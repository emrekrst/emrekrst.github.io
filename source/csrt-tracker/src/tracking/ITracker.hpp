#pragma once

#include <optional>

#include <QSize>

#include <opencv2/core/mat.hpp>

#include "tracking/TrackingTypes.hpp"

class ITracker
{
public:
    virtual ~ITracker() = default;

    virtual void initialize(FramePoint point, QSize frameSize) = 0;
    virtual std::optional<FrameRect> update(const cv::Mat& frame) = 0;
    virtual void reset() = 0;
    virtual std::optional<FrameRect> currentBox() const = 0;
};
