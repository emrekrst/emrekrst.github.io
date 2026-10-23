#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

#include <QObject>

#include "tracking/ITrackingEngine.hpp"
#include "tracking/ITrackingFrameSink.hpp"
#include "tracking/TrackingFrame.hpp"
#include "tracking/TrackingLifecycle.hpp"

class TrackingController final : public QObject, public ITrackingFrameSink
{
    Q_OBJECT

public:
    explicit TrackingController(
        std::unique_ptr<ITrackingEngine> engine,
        QObject* parent = nullptr);
    ~TrackingController() override;

    void offerFrame(GstBuffer* buffer, const GstVideoInfo& info) noexcept override;
    void selectTarget(FramePoint point);
    void reset();

    [[nodiscard]] TrackingState state() const noexcept;
    [[nodiscard]] TrackingStats stats() const noexcept;
    [[nodiscard]] std::optional<FrameRect> currentBox() const;

signals:
    void stateChanged(TrackingState state);
    void targetBoxChanged(FrameRect box);
    void targetCleared();
    void selectionCandidate(FrameRect box);
    void statsChanged(TrackingStats stats);
    void errorOccurred(QString message);

private:
    enum class CommandKind { Select, Update };
    struct Command
    {
        CommandKind kind{};
        quint64 generation{};
        TrackingFramePtr frame;
        FramePoint point;
    };
    enum class ResultKind { Tracking, Updated, Lost, SelectionFailed, Error };
    struct Result
    {
        ResultKind kind{};
        quint64 generation{};
        std::optional<FrameRect> box;
        QString error;
        double updateMilliseconds{};
        double resultAgeMilliseconds{};
    };

    void workerLoop();
    Result execute(const Command& command);
    void handleResult(Result result);
    void schedulePendingSelection();
    void setState(TrackingState state);
    void clearBox();

    std::unique_ptr<ITrackingEngine> engine_;
    mutable std::mutex commandMutex_;
    std::condition_variable commandReady_;
    std::optional<Command> command_;
    std::optional<Command> pendingSelection_;
    bool stopping_{false};

    mutable std::mutex latestFrameMutex_;
    TrackingFramePtr latestFrame_;
    mutable std::mutex boxMutex_;
    std::optional<FrameRect> box_;

    std::atomic<TrackingState> state_{TrackingState::Ready};
    TrackingLifecycle lifecycle_;
    std::atomic<quint64> completedUpdates_{0};
    std::atomic<quint64> skippedFrames_{0};
    std::atomic<double> lastUpdateMilliseconds_{0.0};
    std::atomic<double> lastResultAgeMilliseconds_{0.0};
    std::thread worker_;
};
