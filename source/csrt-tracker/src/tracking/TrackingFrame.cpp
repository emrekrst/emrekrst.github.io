#include "tracking/TrackingFrame.hpp"

#include <opencv2/imgproc.hpp>

TrackingFrame::TrackingFrame(GstBuffer* buffer, const GstVideoInfo& info,
    std::chrono::steady_clock::time_point capturedAt)
    : buffer_(gst_buffer_ref(buffer))
    , info_(info)
    , capturedAt_(capturedAt)
{
}

TrackingFrame::~TrackingFrame()
{
    if (buffer_ != nullptr) {
        gst_buffer_unref(buffer_);
    }
}

std::shared_ptr<const TrackingFrame> TrackingFrame::capture(
    GstBuffer* buffer, const GstVideoInfo& info)
{
    if (buffer == nullptr || GST_VIDEO_INFO_FORMAT(&info) != GST_VIDEO_FORMAT_BGRx) {
        return {};
    }
    return std::shared_ptr<const TrackingFrame>(new TrackingFrame(
        buffer, info, std::chrono::steady_clock::now()));
}

std::optional<MappedBgrFrame> TrackingFrame::mapBgr() const
{
    GstVideoFrame mapped;
    if (!gst_video_frame_map(&mapped, &info_, buffer_, GST_MAP_READ)) {
        return std::nullopt;
    }

    const int width = GST_VIDEO_FRAME_WIDTH(&mapped);
    const int height = GST_VIDEO_FRAME_HEIGHT(&mapped);
    const int stride = GST_VIDEO_FRAME_PLANE_STRIDE(&mapped, 0);
    const cv::Mat bgrx(height, width, CV_8UC4,
        GST_VIDEO_FRAME_PLANE_DATA(&mapped, 0), stride);
    cv::Mat bgr;
    cv::cvtColor(bgrx, bgr, cv::COLOR_BGRA2BGR);
    gst_video_frame_unmap(&mapped);

    const GstClockTime pts = GST_BUFFER_PTS(buffer_);
    return MappedBgrFrame{std::move(bgr),
        GST_CLOCK_TIME_IS_VALID(pts) ? static_cast<qint64>(pts) : qint64{0}};
}

QSize TrackingFrame::size() const
{
    return {static_cast<int>(GST_VIDEO_INFO_WIDTH(&info_)),
        static_cast<int>(GST_VIDEO_INFO_HEIGHT(&info_))};
}

std::chrono::steady_clock::time_point TrackingFrame::capturedAt() const noexcept
{
    return capturedAt_;
}
