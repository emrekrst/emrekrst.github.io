#pragma once

#include "tracking/IObjectSelector.hpp"

class SeededObjectSelector final : public IObjectSelector
{
public:
    std::optional<SelectionResult> select(
        const cv::Mat& bgrFrame,
        const FramePoint& click) const override;
};
