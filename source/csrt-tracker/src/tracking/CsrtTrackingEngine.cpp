#include "tracking/CsrtTrackingEngine.hpp"

#include "tracking/SeededObjectSelector.hpp"

CsrtTrackingEngine::CsrtTrackingEngine()
    : CsrtTrackingEngine(CsrtConfig{})
{
}

CsrtTrackingEngine::CsrtTrackingEngine(CsrtConfig config)
    : CsrtTrackingEngine(
          std::make_unique<SeededObjectSelector>(),
          std::make_unique<CsrtTracker>(config))
{
}

CsrtTrackingEngine::CsrtTrackingEngine(
    std::unique_ptr<IObjectSelector> selector,
    std::unique_ptr<CsrtTracker> tracker)
    : selector_(std::move(selector))
    , tracker_(std::move(tracker))
{
}

std::optional<SelectionResult> CsrtTrackingEngine::select(
    const cv::Mat& frame, const FramePoint& point)
{
    return selector_->select(frame, point);
}

bool CsrtTrackingEngine::initialize(const cv::Mat& frame, const FrameRect& box)
{
    return tracker_->initialize(frame, box);
}

std::optional<FrameRect> CsrtTrackingEngine::update(const cv::Mat& frame)
{
    return tracker_->update(frame);
}

void CsrtTrackingEngine::reset()
{
    tracker_->reset();
}
