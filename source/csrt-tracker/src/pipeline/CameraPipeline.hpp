#pragma once

#include <atomic>
#include <mutex>

#include <QImage>
#include <QSize>

#include "config/CameraConfig.hpp"
#include "pipeline/AbstractPipeline.hpp"

class CameraPipeline final : public AbstractPipeline
{
    Q_OBJECT

public:
    explicit CameraPipeline(QObject* parent = nullptr);
    ~CameraPipeline() override;

    static QString description(const CameraConfig& config = CameraConfig{});
    static bool isAvailable();
    static QString unavailabilityReason();
    bool start(WId windowHandle);
    bool start(WId windowHandle, const CameraConfig& config);
    void setDisplaySize(QSize size);

signals:
    void frameReady(const QImage& frame);

private:
    static GstFlowReturn newSample(GstElement* sink, gpointer userData);
    static GstFlowReturn newPreroll(GstElement* sink, gpointer userData);
    GstFlowReturn deliverSample(GstSample* sample);
    void deliverPendingFrame();
    void releaseDeliverySlot();

    std::mutex frameMutex_;
    QImage pendingFrame_;
    bool deliveryPosted_{false};
    std::atomic<int> displayWidth_{960};
    std::atomic<int> displayHeight_{540};
};
