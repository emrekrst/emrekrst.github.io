#include "pipeline/CameraPipeline.hpp"

#include <algorithm>
#include <utility>

#include <QMetaObject>
#include <QStringList>

#include <gst/app/gstappsink.h>
#include <gst/video/video.h>

namespace {
bool elementFactoryExists(const char* factoryName)
{
    GstElementFactory* factory = gst_element_factory_find(factoryName);
    if (factory == nullptr) {
        return false;
    }
    gst_object_unref(factory);
    return true;
}
} // namespace

CameraPipeline::CameraPipeline(QObject* parent)
    : AbstractPipeline(parent)
{
}

CameraPipeline::~CameraPipeline()
{
    stop();
}

QString CameraPipeline::description(const CameraConfig& config)
{
    const QString gain = config.gainMode == CameraGainMode::Auto
        ? QStringLiteral("analogue-gain-mode=auto")
        : QStringLiteral("analogue-gain-mode=manual analogue-gain=%1")
              .arg(config.analogueGain, 0, 'g', 15);

    return QStringLiteral(
        "libcamerasrc sensor-config=\"sensor/config,width=%1,height=%2,depth=%3\" "
        "ae-enable=true "
        "exposure-time-mode=manual "
        "exposure-time=%4 "
        "%5 ! "
        "video/x-raw,format=BGRx,width=%1,height=%2,framerate=%6/1 ! "
        "queue name=tracker_queue max-size-buffers=1 max-size-bytes=0 max-size-time=0 "
        "leaky=downstream ! "
        "appsink name=video_sink emit-signals=true sync=false "
        "max-buffers=1 drop=true")
        .arg(config.width)
        .arg(config.height)
        .arg(config.sensorDepth)
        .arg(config.exposureMicroseconds)
        .arg(gain)
        .arg(config.fps);
}

bool CameraPipeline::isAvailable()
{
#if defined(Q_OS_LINUX)
    gst_init(nullptr, nullptr);
    return elementFactoryExists("libcamerasrc")
        && elementFactoryExists("appsink");
#else
    return false;
#endif
}

QString CameraPipeline::unavailabilityReason()
{
#if defined(Q_OS_LINUX)
    gst_init(nullptr, nullptr);
    QStringList missing;
    if (!elementFactoryExists("libcamerasrc")) {
        missing << QStringLiteral("libcamerasrc");
    }
    if (!elementFactoryExists("appsink")) {
        missing << QStringLiteral("appsink");
    }
    if (missing.isEmpty()) {
        return {};
    }
    return QStringLiteral("Missing required GStreamer element(s): %1.")
        .arg(missing.join(QStringLiteral(", ")));
#else
    return QStringLiteral("Camera mode is available only on the Raspberry Pi/Linux build.");
#endif
}

bool CameraPipeline::start(WId windowHandle)
{
    return start(windowHandle, CameraConfig{});
}

bool CameraPipeline::start(WId windowHandle, const CameraConfig& config)
{
    Q_UNUSED(windowHandle)
    if (!isAvailable()) {
        reportError(unavailabilityReason());
        return false;
    }

    GError* parseError = nullptr;
    GstElement* pipeline = gst_parse_launch(description(config).toUtf8().constData(), &parseError);
    if (pipeline == nullptr) {
        const QString message = parseError != nullptr
            ? QString::fromUtf8(parseError->message)
            : QStringLiteral("Could not parse the camera pipeline.");
        g_clear_error(&parseError);
        reportError(message);
        return false;
    }

    GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline), "video_sink");
    GstElement* queue = gst_bin_get_by_name(GST_BIN(pipeline), "tracker_queue");
    if (sink != nullptr) {
        g_signal_connect(sink, "new-sample", G_CALLBACK(&CameraPipeline::newSample), this);
        g_signal_connect(sink, "new-preroll", G_CALLBACK(&CameraPipeline::newPreroll), this);
    }
    const bool started = adoptAndPlay(pipeline, sink, queue, 0);
    if (sink != nullptr) {
        gst_object_unref(sink);
    }
    if (queue != nullptr) {
        gst_object_unref(queue);
    }
    return started;
}

void CameraPipeline::setDisplaySize(QSize size)
{
    if (size.width() > 0 && size.height() > 0) {
        displayWidth_.store(size.width());
        displayHeight_.store(size.height());
    }
}

GstFlowReturn CameraPipeline::newSample(GstElement* sink, gpointer userData)
{
    auto* self = static_cast<CameraPipeline*>(userData);
    return self->deliverSample(gst_app_sink_pull_sample(GST_APP_SINK(sink)));
}

GstFlowReturn CameraPipeline::newPreroll(GstElement* sink, gpointer userData)
{
    auto* self = static_cast<CameraPipeline*>(userData);
    return self->deliverSample(gst_app_sink_pull_preroll(GST_APP_SINK(sink)));
}

GstFlowReturn CameraPipeline::deliverSample(GstSample* sample)
{
    if (sample == nullptr) {
        return GST_FLOW_OK;
    }

    {
        std::lock_guard lock(frameMutex_);
        if (deliveryPosted_) {
            gst_sample_unref(sample);
            return GST_FLOW_OK;
        }
        deliveryPosted_ = true;
    }

    GstCaps* caps = gst_sample_get_caps(sample);
    GstBuffer* buffer = gst_sample_get_buffer(sample);
    GstVideoInfo info;
    if (caps == nullptr || buffer == nullptr || !gst_video_info_from_caps(&info, caps)) {
        gst_sample_unref(sample);
        releaseDeliverySlot();
        return GST_FLOW_OK;
    }

    GstVideoFrame videoFrame;
    if (!gst_video_frame_map(&videoFrame, &info, buffer, GST_MAP_READ)) {
        gst_sample_unref(sample);
        releaseDeliverySlot();
        return GST_FLOW_OK;
    }

    const int width = GST_VIDEO_FRAME_WIDTH(&videoFrame);
    const int height = GST_VIDEO_FRAME_HEIGHT(&videoFrame);
    const int stride = GST_VIDEO_FRAME_PLANE_STRIDE(&videoFrame, 0);
    const QImage wrapped(
        static_cast<const uchar*>(GST_VIDEO_FRAME_PLANE_DATA(&videoFrame, 0)),
        width, height, stride, QImage::Format_RGB32);
    const QSize target = QSize(width, height).scaled(
        std::max(1, displayWidth_.load()),
        std::max(1, displayHeight_.load()),
        Qt::KeepAspectRatio);
    QImage scaled = wrapped.scaled(target, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    if (scaled.constBits() == wrapped.constBits()) {
        scaled = wrapped.copy();
    }

    gst_video_frame_unmap(&videoFrame);
    gst_sample_unref(sample);

    {
        std::lock_guard lock(frameMutex_);
        pendingFrame_ = std::move(scaled);
    }
    QMetaObject::invokeMethod(
        this, [this] { deliverPendingFrame(); }, Qt::QueuedConnection);
    return GST_FLOW_OK;
}

void CameraPipeline::deliverPendingFrame()
{
    QImage frame;
    {
        std::lock_guard lock(frameMutex_);
        frame = std::move(pendingFrame_);
        deliveryPosted_ = false;
    }
    if (!frame.isNull()) {
        emit frameReady(frame);
    }
}

void CameraPipeline::releaseDeliverySlot()
{
    std::lock_guard lock(frameMutex_);
    deliveryPosted_ = false;
}
