#pragma once

#include <QSizeF>

#include "tracking/ITracker.hpp"

class PlaceholderTracker final : public ITracker
{
public:
    explicit PlaceholderTracker(QSizeF boxSize = {120.0, 80.0});

    void initialize(FramePoint point, QSize frameSize) override;
    std::optional<FrameRect> update(const cv::Mat& frame) override;
    void reset() override;
    std::optional<FrameRect> currentBox() const override;

private:
    QSizeF boxSize_;
    std::optional<FrameRect> box_;
};
