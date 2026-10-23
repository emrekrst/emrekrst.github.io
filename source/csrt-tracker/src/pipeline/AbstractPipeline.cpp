#include "pipeline/AbstractPipeline.hpp"

#include <QMetaObject>

#include <gst/video/videooverlay.h>

AbstractPipeline::AbstractPipeline(QObject* parent)
    : QObject(parent)
{
    busTimer_.setInterval(50);
    connect(&busTimer_, &QTimer::timeout, this, &AbstractPipeline::pollBus);
}

AbstractPipeline::~AbstractPipeline()
{
    stop();
}

bool AbstractPipeline::adoptAndPlay(
    GstElement* pipeline,
    GstElement* videoSink,
    GstElement* trackerQueue,
    WId windowHandle)
{
    stop();
    if (pipeline == nullptr || videoSink == nullptr) {
        if (pipeline != nullptr) {
            gst_object_unref(pipeline);
        }
        reportError("GStreamer could not create the playback pipeline.");
        return false;
    }

    pipeline_ = pipeline;
    videoSink_ = GST_ELEMENT(gst_object_ref(videoSink));
    bus_ = gst_element_get_bus(pipeline_);

    if (windowHandle != 0 && GST_IS_VIDEO_OVERLAY(videoSink_)) {
        gst_video_overlay_set_window_handle(
            GST_VIDEO_OVERLAY(videoSink_),
            static_cast<guintptr>(windowHandle));
    }

    if (trackerQueue != nullptr) {
        probePad_ = gst_element_get_static_pad(trackerQueue, "src");
        if (probePad_ != nullptr) {
            probeId_ = gst_pad_add_probe(
                probePad_,
                GST_PAD_PROBE_TYPE_BUFFER,
                &AbstractPipeline::bufferProbe,
                this,
                nullptr);
        }
    }

    const GstStateChangeReturn stateResult =
        gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    if (stateResult == GST_STATE_CHANGE_FAILURE) {
        reportError("GStreamer could not start playback.");
        stop();
        return false;
    }

    busTimer_.start();
    return true;
}

void AbstractPipeline::stop()
{
    busTimer_.stop();
    detachProbe();
    if (pipeline_ != nullptr) {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
    }
    if (bus_ != nullptr) {
        gst_object_unref(bus_);
        bus_ = nullptr;
    }
    if (videoSink_ != nullptr) {
        gst_object_unref(videoSink_);
        videoSink_ = nullptr;
    }
    if (pipeline_ != nullptr) {
        gst_object_unref(pipeline_);
        pipeline_ = nullptr;
    }
    observedFrames_.store(0, std::memory_order_relaxed);
    frameSize_ = {};
}

bool AbstractPipeline::setPaused(bool paused)
{
    if (pipeline_ == nullptr) {
        return false;
    }
    return gst_element_set_state(
               pipeline_, paused ? GST_STATE_PAUSED : GST_STATE_PLAYING)
        != GST_STATE_CHANGE_FAILURE;
}

bool AbstractPipeline::seek(double normalizedPosition)
{
    if (pipeline_ == nullptr || normalizedPosition < 0.0 || normalizedPosition > 1.0) {
        return false;
    }
    const qint64 duration = durationMilliseconds();
    if (duration <= 0) {
        return false;
    }
    const gint64 target = static_cast<gint64>(
        normalizedPosition * duration * GST_MSECOND);
    return gst_element_seek_simple(
        pipeline_,
        GST_FORMAT_TIME,
        static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
        target);
}

qint64 AbstractPipeline::positionMilliseconds() const
{
    gint64 value = GST_CLOCK_TIME_NONE;
    if (pipeline_ == nullptr
        || !gst_element_query_position(pipeline_, GST_FORMAT_TIME, &value)) {
        return 0;
    }
    return value / GST_MSECOND;
}

qint64 AbstractPipeline::durationMilliseconds() const
{
    gint64 value = GST_CLOCK_TIME_NONE;
    if (pipeline_ == nullptr
        || !gst_element_query_duration(pipeline_, GST_FORMAT_TIME, &value)) {
        return 0;
    }
    return value / GST_MSECOND;
}

bool AbstractPipeline::isLoaded() const
{
    return pipeline_ != nullptr;
}

quint64 AbstractPipeline::observedFrameCount() const noexcept
{
    return observedFrames_.load(std::memory_order_relaxed);
}

void AbstractPipeline::setTrackingFrameSink(ITrackingFrameSink* sink) noexcept
{
    trackingFrameSink_.store(sink, std::memory_order_release);
}

void AbstractPipeline::reportError(const QString& message)
{
    emit errorOccurred(message);
}

void AbstractPipeline::pollBus()
{
    if (bus_ == nullptr) {
        return;
    }

    while (GstMessage* message = gst_bus_pop_filtered(
               bus_, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS))) {
        if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
            GError* error = nullptr;
            gchar* details = nullptr;
            gst_message_parse_error(message, &error, &details);
            const QString text = error != nullptr
                ? QString::fromUtf8(error->message)
                : QStringLiteral("Unknown GStreamer error");
            g_clear_error(&error);
            g_free(details);
            gst_message_unref(message);
            reportError(text);
            stop();
            return;
        }

        gst_message_unref(message);
        emit endOfStream();
        setPaused(true);
    }
}

GstPadProbeReturn AbstractPipeline::bufferProbe(
    GstPad* pad,
    GstPadProbeInfo* info,
    gpointer userData)
{
    auto* self = static_cast<AbstractPipeline*>(userData);
    self->observedFrames_.fetch_add(1, std::memory_order_relaxed);
    GstCaps* caps = gst_pad_get_current_caps(pad);
    if (caps != nullptr && !gst_caps_is_empty(caps)) {
        if (auto* sink = self->trackingFrameSink_.load(std::memory_order_acquire)) {
            GstVideoInfo videoInfo;
            if (gst_video_info_from_caps(&videoInfo, caps)) {
                sink->offerFrame(gst_pad_probe_info_get_buffer(info), videoInfo);
            }
        }
        const GstStructure* structure = gst_caps_get_structure(caps, 0);
        int width = 0;
        int height = 0;
        if (gst_structure_get_int(structure, "width", &width)
            && gst_structure_get_int(structure, "height", &height)) {
            const QSize nextSize(width, height);
            if (nextSize != self->frameSize_) {
                self->frameSize_ = nextSize;
                QMetaObject::invokeMethod(
                    self,
                    [self, nextSize] { emit self->frameSizeChanged(nextSize); },
                    Qt::QueuedConnection);
            }
        }
        gst_caps_unref(caps);
    }
    return GST_PAD_PROBE_OK;
}

void AbstractPipeline::detachProbe()
{
    if (probePad_ != nullptr) {
        if (probeId_ != 0) {
            gst_pad_remove_probe(probePad_, probeId_);
        }
        gst_object_unref(probePad_);
        probePad_ = nullptr;
        probeId_ = 0;
    }
}
