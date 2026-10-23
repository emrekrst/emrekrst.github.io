#include "tracking/CsrtTracker.hpp"

#include <algorithm>
#include <cmath>

namespace {

std::optional<cv::Rect> clampedRect(const FrameRect& box, const cv::Size& size)
{
    if (!std::isfinite(box.x) || !std::isfinite(box.y)
        || !std::isfinite(box.width) || !std::isfinite(box.height)
        || box.width < 2.0 || box.height < 2.0) {
        return std::nullopt;
    }

    const int left = std::clamp(cvRound(box.x), 0, size.width);
    const int top = std::clamp(cvRound(box.y), 0, size.height);
    const int right = std::clamp(cvRound(box.x + box.width), 0, size.width);
    const int bottom = std::clamp(cvRound(box.y + box.height), 0, size.height);
    if (right - left < 2 || bottom - top < 2) {
        return std::nullopt;
    }
    return cv::Rect(left, top, right - left, bottom - top);
}

FrameRect toFrameRect(const cv::Rect& box)
{
    return {static_cast<double>(box.x), static_cast<double>(box.y),
            static_cast<double>(box.width), static_cast<double>(box.height)};
}

} // namespace

cv::TrackerCSRT::Params buildOpenCvCsrtParams(const CsrtConfig& config)
{
    cv::TrackerCSRT::Params params;
    params.use_hog = config.useHog;
    params.use_color_names = config.useColorNames;
    params.use_gray = config.useGray;
    params.use_rgb = config.useRgb;
    params.use_channel_weights = config.useChannelWeights;
    params.use_segmentation = config.useSegmentation;
    params.template_size = config.templateSize;
    params.padding = config.padding;
    params.number_of_scales = config.numberOfScales;
    params.scale_step = config.scaleStep;
    params.admm_iterations = config.admmIterations;
    params.psr_threshold = config.psrThreshold;
    return params;
}

CsrtTracker::CsrtTracker(CsrtConfig config)
    : config_(config)
{
}

bool CsrtTracker::initialize(const cv::Mat& bgrFrame, const FrameRect& box)
{
    reset();
    if (bgrFrame.empty()) {
        return false;
    }
    const auto roi = clampedRect(box, bgrFrame.size());
    if (!roi) {
        return false;
    }

    try {
        tracker_ = cv::TrackerCSRT::create(buildOpenCvCsrtParams(config_));
        tracker_->init(bgrFrame, *roi);
        box_ = toFrameRect(*roi);
        return true;
    } catch (const cv::Exception&) {
        reset();
        return false;
    }
}

std::optional<FrameRect> CsrtTracker::update(const cv::Mat& bgrFrame)
{
    if (!tracker_ || bgrFrame.empty()) {
        return std::nullopt;
    }

    try {
        cv::Rect updated;
        if (!tracker_->update(bgrFrame, updated)) {
            box_.reset();
            return std::nullopt;
        }
        const auto valid = clampedRect(toFrameRect(updated), bgrFrame.size());
        if (!valid) {
            box_.reset();
            return std::nullopt;
        }
        box_ = toFrameRect(*valid);
        return box_;
    } catch (const cv::Exception&) {
        box_.reset();
        return std::nullopt;
    }
}

void CsrtTracker::reset()
{
    tracker_.release();
    box_.reset();
}

std::optional<FrameRect> CsrtTracker::currentBox() const
{
    return box_;
}
