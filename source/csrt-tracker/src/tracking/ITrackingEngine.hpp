#pragma once

#include <optional>

#include <opencv2/core.hpp>

#include "tracking/TrackingTypes.hpp"

class ITrackingEngine
{
public:
    virtual ~ITrackingEngine() = default;
    virtual std::optional<SelectionResult> select(
        const cv::Mat& frame, const FramePoint& point) = 0;
    virtual bool initialize(const cv::Mat& frame, const FrameRect& box) = 0;
    virtual std::optional<FrameRect> update(const cv::Mat& frame) = 0;
    virtual void reset() = 0;
};
