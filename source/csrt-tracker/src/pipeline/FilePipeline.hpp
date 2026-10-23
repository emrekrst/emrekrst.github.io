#pragma once

#include <atomic>
#include <mutex>

#include <QImage>
#include <QSize>

#include "pipeline/AbstractPipeline.hpp"

QSize normalizedTrackingSize(QSize source) noexcept;

// Video-file playback. Unlike the camera pipeline, this one is ours to
// shape freely: latency is not critical for file playback, so frames are
// pulled into the application with appsink and painted by Qt. That avoids
// embedding a native video surface, which on macOS cannot coexist with a
// Qt overlay widget (Qt promotes the overlay to a native view stacked
// above the video, hiding it).
class FilePipeline final : public AbstractPipeline
{
    Q_OBJECT

public:
    explicit FilePipeline(QObject* parent = nullptr);
    ~FilePipeline() override;

    bool open(const QString& filePath);

    // Frames are scaled to this size on the streaming thread, so the UI
    // thread never scales a full-resolution image.
    void setDisplaySize(QSize size);

signals:
    void frameReady(const QImage& frame);

private:
    static void padAdded(GstElement* decoder, GstPad* newPad, gpointer userData);
    static GstFlowReturn newSample(GstElement* sink, gpointer userData);
    static GstFlowReturn newPreroll(GstElement* sink, gpointer userData);
    GstFlowReturn deliverSample(GstSample* sample);
    void deliverPendingFrame();

    std::mutex frameMutex_;
    QImage pendingFrame_;
    bool deliveryPosted_{false};
    std::atomic<int> displayWidth_{960};
    std::atomic<int> displayHeight_{540};
};
