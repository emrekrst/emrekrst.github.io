#pragma once

#include <optional>

#include <opencv2/core.hpp>
#include <opencv2/tracking.hpp>

#include "tracking/CsrtConfig.hpp"
#include "tracking/TrackingTypes.hpp"

cv::TrackerCSRT::Params buildOpenCvCsrtParams(const CsrtConfig& config);

class CsrtTracker final
{
public:
    explicit CsrtTracker(CsrtConfig config = {});

    bool initialize(const cv::Mat& bgrFrame, const FrameRect& box);
    std::optional<FrameRect> update(const cv::Mat& bgrFrame);
    void reset();
    std::optional<FrameRect> currentBox() const;

private:
    CsrtConfig config_;
    cv::Ptr<cv::TrackerCSRT> tracker_;
    std::optional<FrameRect> box_;
};
