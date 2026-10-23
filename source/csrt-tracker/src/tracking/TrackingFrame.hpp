#pragma once

#include <chrono>
#include <memory>
#include <optional>

#include <QSize>

#include <gst/gst.h>
#include <gst/video/video.h>
#include <opencv2/core/mat.hpp>

struct MappedBgrFrame
{
    cv::Mat mat;
    qint64 ptsNanoseconds{};
};

class TrackingFrame final
{
public:
    static std::shared_ptr<const TrackingFrame> capture(
        GstBuffer* buffer, const GstVideoInfo& info);
    ~TrackingFrame();

    TrackingFrame(const TrackingFrame&) = delete;
    TrackingFrame& operator=(const TrackingFrame&) = delete;

    [[nodiscard]] std::optional<MappedBgrFrame> mapBgr() const;
    [[nodiscard]] QSize size() const;
    [[nodiscard]] std::chrono::steady_clock::time_point capturedAt() const noexcept;

private:
    TrackingFrame(GstBuffer* buffer, const GstVideoInfo& info,
        std::chrono::steady_clock::time_point capturedAt);

    GstBuffer* buffer_{};
    GstVideoInfo info_{};
    std::chrono::steady_clock::time_point capturedAt_;
};

using TrackingFramePtr = std::shared_ptr<const TrackingFrame>;
