#include "tracking/PlaceholderTracker.hpp"

#include <algorithm>

PlaceholderTracker::PlaceholderTracker(QSizeF boxSize)
    : boxSize_(boxSize)
{
}

void PlaceholderTracker::initialize(FramePoint point, QSize frameSize)
{
    if (frameSize.width() <= 0 || frameSize.height() <= 0) {
        box_.reset();
        return;
    }

    const double width = std::min(boxSize_.width(), static_cast<double>(frameSize.width()));
    const double height = std::min(boxSize_.height(), static_cast<double>(frameSize.height()));
    box_ = FrameRect{
        std::clamp(point.x - width / 2.0, 0.0, frameSize.width() - width),
        std::clamp(point.y - height / 2.0, 0.0, frameSize.height() - height),
        width,
        height,
    };
}

std::optional<FrameRect> PlaceholderTracker::update(const cv::Mat&)
{
    return box_;
}

void PlaceholderTracker::reset()
{
    box_.reset();
}

std::optional<FrameRect> PlaceholderTracker::currentBox() const
{
    return box_;
}
