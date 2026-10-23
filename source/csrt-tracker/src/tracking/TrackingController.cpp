#include "tracking/TrackingController.hpp"

#include <chrono>
#include <exception>
#include <utility>

#include <QMetaObject>

TrackingController::TrackingController(
    std::unique_ptr<ITrackingEngine> engine,
    QObject* parent)
    : QObject(parent)
    , engine_(std::move(engine))
{
    qRegisterMetaType<FrameRect>();
    qRegisterMetaType<TrackingState>();
    qRegisterMetaType<TrackingStats>();
    worker_ = std::thread([this] { workerLoop(); });
}

TrackingController::~TrackingController()
{
    {
        std::lock_guard lock(commandMutex_);
        stopping_ = true;
        command_.reset();
        pendingSelection_.reset();
    }
    commandReady_.notify_one();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void TrackingController::offerFrame(GstBuffer* buffer, const GstVideoInfo& info) noexcept
{
    TrackingFramePtr frame;
    try {
        frame = TrackingFrame::capture(buffer, info);
    } catch (...) {
        return;
    }
    if (!frame) {
        return;
    }

    {
        std::lock_guard lock(latestFrameMutex_);
        latestFrame_ = frame;
    }

    const auto admission = lifecycle_.admitUpdate([this] {
        skippedFrames_.fetch_add(1, std::memory_order_relaxed);
    });
    if (!admission) {
        return;
    }

    {
        std::lock_guard lock(commandMutex_);
        command_ = Command{CommandKind::Update, admission->generation, std::move(frame), {}};
    }
    commandReady_.notify_one();
}

void TrackingController::selectTarget(FramePoint point)
{
    const quint64 generation = lifecycle_.beginSelection();
    setState(TrackingState::Detecting);
    clearBox();

    TrackingFramePtr frame;
    {
        std::lock_guard lock(latestFrameMutex_);
        frame = latestFrame_;
    }
    if (!frame) {
        setState(TrackingState::Ready);
        return;
    }

    Command selection{CommandKind::Select, generation, std::move(frame), point};
    if (lifecycle_.tryBeginCommand()) {
        {
            std::lock_guard lock(commandMutex_);
            command_ = std::move(selection);
        }
        commandReady_.notify_one();
    } else {
        std::lock_guard lock(commandMutex_);
        pendingSelection_ = std::move(selection);
    }
}

void TrackingController::reset()
{
    bool stateDidChange = false;
    lifecycle_.reset([this, &stateDidChange] {
        completedUpdates_.store(0, std::memory_order_relaxed);
        skippedFrames_.store(0, std::memory_order_relaxed);
        lastUpdateMilliseconds_.store(0.0, std::memory_order_relaxed);
        lastResultAgeMilliseconds_.store(0.0, std::memory_order_relaxed);
        stateDidChange = state_.exchange(TrackingState::Ready) != TrackingState::Ready;
    });
    {
        std::lock_guard lock(commandMutex_);
        pendingSelection_.reset();
    }
    {
        std::lock_guard lock(latestFrameMutex_);
        latestFrame_.reset();
    }
    if (stateDidChange) {
        emit stateChanged(TrackingState::Ready);
    }
    clearBox();
}

TrackingState TrackingController::state() const noexcept
{
    return state_.load(std::memory_order_acquire);
}

TrackingStats TrackingController::stats() const noexcept
{
    return {completedUpdates_.load(std::memory_order_relaxed),
        skippedFrames_.load(std::memory_order_relaxed),
        lastUpdateMilliseconds_.load(std::memory_order_relaxed),
        lastResultAgeMilliseconds_.load(std::memory_order_relaxed)};
}

std::optional<FrameRect> TrackingController::currentBox() const
{
    std::lock_guard lock(boxMutex_);
    return box_;
}

void TrackingController::workerLoop()
{
    for (;;) {
        Command command;
        {
            std::unique_lock lock(commandMutex_);
            commandReady_.wait(lock, [this] { return stopping_ || command_.has_value(); });
            if (stopping_) {
                return;
            }
            command = std::move(*command_);
            command_.reset();
        }

        Result result = execute(command);
        QMetaObject::invokeMethod(
            this,
            [this, result = std::move(result)]() mutable { handleResult(std::move(result)); },
            Qt::QueuedConnection);
    }
}

TrackingController::Result TrackingController::execute(const Command& command)
{
    try {
        const auto mapped = command.frame->mapBgr();
        if (!mapped) {
            return {ResultKind::Error, command.generation, std::nullopt,
                    QStringLiteral("Could not map the tracking frame.")};
        }

        if (command.kind == CommandKind::Select) {
            engine_->reset();
            const auto selection = engine_->select(mapped->mat, command.point);
            if (!selection) {
                return {ResultKind::SelectionFailed, command.generation};
            }
            if (!engine_->initialize(mapped->mat, selection->box)) {
                return {ResultKind::SelectionFailed, command.generation};
            }
            return {ResultKind::Tracking, command.generation, selection->box};
        }

        const auto updateStarted = std::chrono::steady_clock::now();
        const auto box = engine_->update(mapped->mat);
        const auto updateFinished = std::chrono::steady_clock::now();
        const double updateMilliseconds =
            std::chrono::duration<double, std::milli>(updateFinished - updateStarted).count();
        const double resultAgeMilliseconds =
            std::chrono::duration<double, std::milli>(updateFinished - command.frame->capturedAt()).count();
        return box ? Result{ResultKind::Updated, command.generation, box, {},
                         updateMilliseconds, resultAgeMilliseconds}
                   : Result{ResultKind::Lost, command.generation, std::nullopt, {},
                         updateMilliseconds, resultAgeMilliseconds};
    } catch (const cv::Exception& error) {
        return {ResultKind::Error, command.generation, std::nullopt,
                QString::fromUtf8(error.what())};
    } catch (const std::exception& error) {
        return {ResultKind::Error, command.generation, std::nullopt,
                QString::fromUtf8(error.what())};
    } catch (...) {
        return {ResultKind::Error, command.generation, std::nullopt,
                QStringLiteral("Unknown tracking error.")};
    }
}

void TrackingController::handleResult(Result result)
{
    const bool trackingAfterCompletion =
        result.kind == ResultKind::Tracking || result.kind == ResultKind::Updated;
    if (!lifecycle_.completeCommand(
            result.generation, trackingAfterCompletion, [this, &result] {
            if (result.kind == ResultKind::Updated || result.kind == ResultKind::Lost) {
                lastUpdateMilliseconds_.store(result.updateMilliseconds, std::memory_order_relaxed);
                lastResultAgeMilliseconds_.store(result.resultAgeMilliseconds,
                    std::memory_order_relaxed);
                if (result.kind == ResultKind::Updated) {
                    completedUpdates_.fetch_add(1, std::memory_order_relaxed);
                }
            }
        })) {
        schedulePendingSelection();
        return;
    }

    switch (result.kind) {
    case ResultKind::Tracking:
    case ResultKind::Updated:
        {
            std::lock_guard lock(boxMutex_);
            box_ = result.box;
        }
        if (result.kind == ResultKind::Tracking) {
            emit selectionCandidate(*result.box);
        }
        emit targetBoxChanged(*result.box);
        setState(TrackingState::Tracking);
        break;
    case ResultKind::Lost:
        clearBox();
        setState(TrackingState::Lost);
        break;
    case ResultKind::SelectionFailed:
        clearBox();
        setState(TrackingState::Ready);
        break;
    case ResultKind::Error:
        clearBox();
        setState(TrackingState::Error);
        emit errorOccurred(result.error);
        break;
    }
    emit statsChanged(stats());
    schedulePendingSelection();
}

void TrackingController::schedulePendingSelection()
{
    std::optional<Command> selection;
    {
        std::lock_guard lock(commandMutex_);
        if (pendingSelection_) {
            selection = std::move(pendingSelection_);
            pendingSelection_.reset();
        }
    }
    if (!selection || !lifecycle_.tryBeginCommand()) {
        if (selection) {
            std::lock_guard lock(commandMutex_);
            pendingSelection_ = std::move(selection);
        }
        return;
    }
    {
        std::lock_guard lock(commandMutex_);
        command_ = std::move(selection);
    }
    commandReady_.notify_one();
}

void TrackingController::setState(TrackingState next)
{
    if (state_.exchange(next) != next) {
        emit stateChanged(next);
    }
}

void TrackingController::clearBox()
{
    bool hadBox = false;
    {
        std::lock_guard lock(boxMutex_);
        hadBox = box_.has_value();
        box_.reset();
    }
    if (hadBox) {
        emit targetCleared();
    }
}
