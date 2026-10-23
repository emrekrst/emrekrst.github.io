#include "pipeline/FilePipeline.hpp"

#include <algorithm>

#include <QFileInfo>
#include <QMetaObject>

#include <gst/app/gstappsink.h>
#include <gst/video/video.h>

QSize normalizedTrackingSize(QSize source) noexcept
{
    if (source.width() <= 0 || source.height() <= 0 || source.height() <= 1080) {
        return source;
    }
    return {qRound(static_cast<double>(source.width()) * 1080.0 / source.height()), 1080};
}

FilePipeline::FilePipeline(QObject* parent)
    : AbstractPipeline(parent)
{
}

FilePipeline::~FilePipeline()
{
    // Stop appsink callbacks while the frame mutex and pending image still
    // exist. The base destructor runs after derived members are destroyed.
    stop();
}

void FilePipeline::setDisplaySize(QSize size)
{
    if (size.width() > 0 && size.height() > 0) {
        displayWidth_.store(size.width());
        displayHeight_.store(size.height());
    }
}

bool FilePipeline::open(const QString& filePath)
{
    stop();
    const QFileInfo file(filePath);
    if (!file.exists() || !file.isFile()) {
        reportError(QStringLiteral("The selected video file does not exist."));
        return false;
    }

    GstElement* pipeline = gst_pipeline_new("file_pipeline");
    GstElement* source = gst_element_factory_make("filesrc", "source");
    GstElement* decoder = gst_element_factory_make("decodebin", "decoder");
    GstElement* converter = gst_element_factory_make("videoconvert", "converter");
    GstElement* scaler = gst_element_factory_make("videoscale", "tracking_scaler");
    GstElement* capsFilter = gst_element_factory_make("capsfilter", "bgrx_caps");
    GstElement* queue = gst_element_factory_make("queue", "tracker_queue");
    GstElement* sink = gst_element_factory_make("appsink", "video_sink");

    if (pipeline == nullptr || source == nullptr || decoder == nullptr
        || converter == nullptr || scaler == nullptr || capsFilter == nullptr || queue == nullptr
        || sink == nullptr) {
        if (pipeline != nullptr) {
            gst_object_unref(pipeline);
        }
        reportError("A required GStreamer playback element is unavailable.");
        return false;
    }

    g_object_set(source, "location", file.absoluteFilePath().toUtf8().constData(), nullptr);
    g_object_set(queue,
        "max-size-buffers", 1,
        "max-size-bytes", 0,
        "max-size-time", static_cast<guint64>(0),
        "leaky", 0 /* no frame loss; backpressure at the media clock */,
        nullptr);

    // sync=true plays at the media's own rate; drop+max-buffers keep only
    // the newest frame so a slow UI can never stall the pipeline.
    g_object_set(sink,
        "emit-signals", TRUE,
        "sync", TRUE,
        "max-buffers", 1,
        "drop", TRUE,
        nullptr);

    GstCaps* caps = gst_caps_from_string(
        "video/x-raw,format=BGRx,width=(int)[1,1920],height=(int)[1,1080],pixel-aspect-ratio=1/1");
    g_object_set(capsFilter, "caps", caps, nullptr);
    g_object_set(sink, "caps", caps, nullptr);
    gst_caps_unref(caps);

    gst_bin_add_many(
        GST_BIN(pipeline), source, decoder, converter, scaler, capsFilter, queue, sink, nullptr);
    if (!gst_element_link(source, decoder)
        || !gst_element_link_many(converter, scaler, capsFilter, queue, sink, nullptr)) {
        gst_object_unref(pipeline);
        reportError("GStreamer could not link the file playback elements.");
        return false;
    }

    g_signal_connect(decoder, "pad-added", G_CALLBACK(&FilePipeline::padAdded), converter);
    g_signal_connect(sink, "new-sample", G_CALLBACK(&FilePipeline::newSample), this);
    g_signal_connect(sink, "new-preroll", G_CALLBACK(&FilePipeline::newPreroll), this);

    // No window handle: nothing is embedded, Qt paints the frames.
    return adoptAndPlay(pipeline, sink, queue, 0);
}

GstFlowReturn FilePipeline::newSample(GstElement* sink, gpointer userData)
{
    auto* self = static_cast<FilePipeline*>(userData);
    return self->deliverSample(gst_app_sink_pull_sample(GST_APP_SINK(sink)));
}

GstFlowReturn FilePipeline::newPreroll(GstElement* sink, gpointer userData)
{
    auto* self = static_cast<FilePipeline*>(userData);
    return self->deliverSample(gst_app_sink_pull_preroll(GST_APP_SINK(sink)));
}

GstFlowReturn FilePipeline::deliverSample(GstSample* sample)
{
    if (sample == nullptr) {
        return GST_FLOW_OK;
    }

    GstCaps* caps = gst_sample_get_caps(sample);
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstVideoInfo info;

    if (caps == nullptr || buffer == nullptr || !gst_video_info_from_caps(&info, caps)) {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    GstVideoFrame videoFrame;
    if (!gst_video_frame_map(&videoFrame, &info, buffer, GST_MAP_READ)) {
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    const int width = GST_VIDEO_FRAME_WIDTH(&videoFrame);
    const int height = GST_VIDEO_FRAME_HEIGHT(&videoFrame);
    const int stride = GST_VIDEO_FRAME_PLANE_STRIDE(&videoFrame, 0);

    // BGRx byte order matches QImage::Format_RGB32 on little-endian, so
    // this wraps the mapped memory without copying it.
    const QImage wrapped(
        static_cast<const uchar*>(GST_VIDEO_FRAME_PLANE_DATA(&videoFrame, 0)),
        width, height, stride, QImage::Format_RGB32);

    // Scale here, on the streaming thread, so the UI thread never handles
    // a full-resolution frame. The result owns its own memory.
    const QSize target = QSize(width, height).scaled(
        std::max(1, displayWidth_.load()),
        std::max(1, displayHeight_.load()),
        Qt::KeepAspectRatio);
    QImage scaled = wrapped.scaled(target, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if (scaled.constBits() == wrapped.constBits()) {
        scaled = wrapped.copy();
    }

    gst_video_frame_unmap(&videoFrame);
    gst_sample_unref(sample);

    bool needsPost = false;
    {
        std::lock_guard lock(frameMutex_);
        pendingFrame_ = scaled;
        needsPost = !deliveryPosted_;
        deliveryPosted_ = true;
    }

    // Coalesce: only one delivery is ever queued, so a slow UI thread
    // drops intermediate frames instead of accumulating a backlog.
    if (needsPost) {
        QMetaObject::invokeMethod(
            this, [this] { deliverPendingFrame(); }, Qt::QueuedConnection);
    }

    return GST_FLOW_OK;
}

void FilePipeline::deliverPendingFrame()
{
    QImage frame;
    {
        std::lock_guard lock(frameMutex_);
        frame = pendingFrame_;
        pendingFrame_ = QImage();
        deliveryPosted_ = false;
    }

    if (!frame.isNull()) {
        emit frameReady(frame);
    }
}

void FilePipeline::padAdded(GstElement*, GstPad* newPad, gpointer userData)
{
    auto* converter = GST_ELEMENT(userData);
    GstPad* sinkPad = gst_element_get_static_pad(converter, "sink");
    if (gst_pad_is_linked(sinkPad)) {
        gst_object_unref(sinkPad);
        return;
    }

    GstCaps* caps = gst_pad_get_current_caps(newPad);
    if (caps == nullptr) {
        caps = gst_pad_query_caps(newPad, nullptr);
    }

    const GstStructure* structure = caps != nullptr && !gst_caps_is_empty(caps)
        ? gst_caps_get_structure(caps, 0)
        : nullptr;
    const char* mediaType = structure != nullptr ? gst_structure_get_name(structure) : "";
    if (g_str_has_prefix(mediaType, "video/")) {
        gst_pad_link(newPad, sinkPad);
    }
    if (caps != nullptr) {
        gst_caps_unref(caps);
    }
    gst_object_unref(sinkPad);
}
