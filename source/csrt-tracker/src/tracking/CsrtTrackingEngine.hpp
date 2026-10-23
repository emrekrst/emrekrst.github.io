#pragma once

#include <memory>

#include "tracking/CsrtTracker.hpp"
#include "tracking/IObjectSelector.hpp"
#include "tracking/ITrackingEngine.hpp"

class CsrtTrackingEngine final : public ITrackingEngine
{
public:
    CsrtTrackingEngine();
    explicit CsrtTrackingEngine(CsrtConfig config);
    CsrtTrackingEngine(
        std::unique_ptr<IObjectSelector> selector,
        std::unique_ptr<CsrtTracker> tracker);

    std::optional<SelectionResult> select(
        const cv::Mat& frame, const FramePoint& point) override;
    bool initialize(const cv::Mat& frame, const FrameRect& box) override;
    std::optional<FrameRect> update(const cv::Mat& frame) override;
    void reset() override;

private:
    std::unique_ptr<IObjectSelector> selector_;
    std::unique_ptr<CsrtTracker> tracker_;
};
