#pragma once

#include <optional>

#include <opencv2/core.hpp>

#include "tracking/TrackingTypes.hpp"

class IObjectSelector
{
public:
    virtual ~IObjectSelector() = default;
    virtual std::optional<SelectionResult> select(
        const cv::Mat& bgrFrame,
        const FramePoint& click) const = 0;
};
