#pragma once

#include <QSize>
#include <QString>
#include <QTimer>
#include <QtGui/qwindowdefs.h>

#include <atomic>

#include <gst/gst.h>

#include "tracking/ITrackingFrameSink.hpp"

class AbstractPipeline : public QObject
{
    Q_OBJECT

public:
    explicit AbstractPipeline(QObject* parent = nullptr);
    ~AbstractPipeline() override;

    AbstractPipeline(const AbstractPipeline&) = delete;
    AbstractPipeline& operator=(const AbstractPipeline&) = delete;

    void stop();
    bool setPaused(bool paused);
    bool seek(double normalizedPosition);
    [[nodiscard]] qint64 positionMilliseconds() const;
    [[nodiscard]] qint64 durationMilliseconds() const;
    [[nodiscard]] bool isLoaded() const;
    [[nodiscard]] quint64 observedFrameCount() const noexcept;
    void setTrackingFrameSink(ITrackingFrameSink* sink) noexcept;

signals:
    void errorOccurred(const QString& message);
    void endOfStream();
    void frameSizeChanged(QSize size);

protected:
    bool adoptAndPlay(
        GstElement* pipeline,
        GstElement* videoSink,
        GstElement* trackerQueue,
        WId windowHandle);
    void reportError(const QString& message);

private slots:
    void pollBus();

private:
    static GstPadProbeReturn bufferProbe(
        GstPad* pad,
        GstPadProbeInfo* info,
        gpointer userData);
    void detachProbe();

    GstElement* pipeline_{nullptr};
    GstElement* videoSink_{nullptr};
    GstBus* bus_{nullptr};
    GstPad* probePad_{nullptr};
    gulong probeId_{0};
    std::atomic<ITrackingFrameSink*> trackingFrameSink_{nullptr};
    std::atomic<quint64> observedFrames_{0};
    QSize frameSize_;
    QTimer busTimer_;
};
