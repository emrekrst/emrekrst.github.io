#include <QtTest>

#include <atomic>
#include <barrier>
#include <cmath>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include <gst/video/video.h>

#include "tracking/TrackingController.hpp"
#include "tracking/TrackingLifecycle.hpp"

struct BlockingState
{
    std::mutex mutex;
    std::condition_variable released;
    std::atomic_bool updateEntered{false};
    bool release{false};
};

class BlockingEngine final : public ITrackingEngine
{
public:
    explicit BlockingEngine(std::shared_ptr<BlockingState> state)
        : state_(std::move(state)) {}

    std::optional<SelectionResult> select(const cv::Mat&, const FramePoint&) override
    {
        return SelectionResult{{20, 20, 30, 18}, 0.9};
    }
    bool initialize(const cv::Mat&, const FrameRect&) override { return true; }
    std::optional<FrameRect> update(const cv::Mat&) override
    {
        state_->updateEntered = true;
        std::unique_lock lock(state_->mutex);
        state_->released.wait(lock, [this] { return state_->release; });
        return FrameRect{24, 22, 30, 18};
    }
    void reset() override {}

private:
    std::shared_ptr<BlockingState> state_;
};

class LosingEngine final : public ITrackingEngine
{
public:
    std::optional<SelectionResult> select(const cv::Mat&, const FramePoint&) override
    {
        return SelectionResult{{20, 20, 30, 18}, 0.9};
    }
    bool initialize(const cv::Mat&, const FrameRect&) override { return true; }
    std::optional<FrameRect> update(const cv::Mat&) override { return std::nullopt; }
    void reset() override {}
};

namespace {

struct TestFrame
{
    GstVideoInfo info{};
    GstBuffer* buffer{};

    TestFrame()
    {
        gst_video_info_set_format(&info, GST_VIDEO_FORMAT_BGRx, 96, 64);
        buffer = gst_buffer_new_allocate(nullptr, info.size, nullptr);
    }
    ~TestFrame() { gst_buffer_unref(buffer); }
};

} // namespace

class TrackingControllerTests final : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void updateAdmissionIsAtomicWithReset();
    void completionClosesTrackingBeforeAdmittingNextOffer_data();
    void completionClosesTrackingBeforeAdmittingNextOffer();
    void busyFramesAreDiscardedAndKeepLastBox();
    void resetDiscardsBlockedUpdateMetrics();
    void completedFailureEntersLost();
};

void TrackingControllerTests::initTestCase()
{
    gst_init(nullptr, nullptr);
}

void TrackingControllerTests::completionClosesTrackingBeforeAdmittingNextOffer_data()
{
    QTest::addColumn<QString>("resultKind");

    QTest::newRow("lost completion") << QStringLiteral("Lost");
    QTest::newRow("error completion") << QStringLiteral("Error");
}

void TrackingControllerTests::completionClosesTrackingBeforeAdmittingNextOffer()
{
    QFETCH(QString, resultKind);
    TrackingLifecycle lifecycle;
    const quint64 generation = lifecycle.beginSelection();
    QVERIFY(lifecycle.tryBeginCommand());
    QVERIFY(lifecycle.completeCommand(generation, true, [] {}));
    QVERIFY(lifecycle.admitUpdate([] {}));

    std::mutex gateMutex;
    std::condition_variable gateChanged;
    bool publicationEntered = false;
    bool releaseCompletion = false;
    bool completionWasCurrent = false;
    std::thread completion([&] {
        completionWasCurrent = lifecycle.completeCommand(generation, false, [&] {
            std::unique_lock lock(gateMutex);
            publicationEntered = true;
            gateChanged.notify_one();
            gateChanged.wait(lock, [&] { return releaseCompletion; });
        });
    });

    {
        std::unique_lock lock(gateMutex);
        gateChanged.wait(lock, [&] { return publicationEntered; });
    }

    std::optional<TrackingLifecycle::UpdateAdmission> racingAdmission;
    std::atomic_bool offerStarted{false};
    std::thread offer([&] {
        offerStarted = true;
        racingAdmission = lifecycle.admitUpdate([] {});
    });
    QTRY_VERIFY(offerStarted.load());

    {
        std::lock_guard lock(gateMutex);
        releaseCompletion = true;
    }
    gateChanged.notify_one();
    completion.join();
    offer.join();

    QVERIFY2(completionWasCurrent, qPrintable(resultKind));
    QVERIFY2(!racingAdmission, qPrintable(resultKind));
}

void TrackingControllerTests::updateAdmissionIsAtomicWithReset()
{
    TrackingLifecycle lifecycle;
    const quint64 trackingGeneration = lifecycle.beginSelection();
    QVERIFY(lifecycle.tryBeginCommand());
    QVERIFY(lifecycle.completeCommand(trackingGeneration, true, [] {}));

    std::barrier start{3};
    std::optional<TrackingLifecycle::UpdateAdmission> admission;
    std::thread offer([&] {
        start.arrive_and_wait();
        admission = lifecycle.admitUpdate([] {});
    });
    std::thread reset([&] {
        start.arrive_and_wait();
        lifecycle.reset([] {});
    });
    start.arrive_and_wait();
    offer.join();
    reset.join();

    if (admission) {
        QVERIFY(!lifecycle.isCurrent(admission->generation));
    }
    QVERIFY(!lifecycle.admitUpdate([] {}));
}

void TrackingControllerTests::busyFramesAreDiscardedAndKeepLastBox()
{
    auto shared = std::make_shared<BlockingState>();
    TrackingController controller(std::make_unique<BlockingEngine>(shared));
    TestFrame frame;

    controller.offerFrame(frame.buffer, frame.info);
    controller.selectTarget({35, 29});
    QTRY_COMPARE(controller.state(), TrackingState::Tracking);

    controller.offerFrame(frame.buffer, frame.info);
    QTRY_VERIFY(shared->updateEntered.load());
    controller.offerFrame(frame.buffer, frame.info);
    controller.offerFrame(frame.buffer, frame.info);
    QCOMPARE(controller.stats().skippedFrames, quint64{2});
    QCOMPARE(controller.stats().completedUpdates, quint64{0});
    QCOMPARE(controller.currentBox(), std::optional<FrameRect>({20, 20, 30, 18}));

    {
        std::lock_guard lock(shared->mutex);
        shared->release = true;
    }
    shared->released.notify_one();
    QTRY_COMPARE(controller.currentBox(), std::optional<FrameRect>({24, 22, 30, 18}));

    const TrackingStats stats = controller.stats();
    QCOMPARE(stats.completedUpdates, quint64{1});
    QCOMPARE(stats.skippedFrames, quint64{2});
    QVERIFY(std::isfinite(stats.lastUpdateMilliseconds));
    QVERIFY(stats.lastUpdateMilliseconds >= 0.0);
    QVERIFY(std::isfinite(stats.lastResultAgeMilliseconds));
    QVERIFY(stats.lastResultAgeMilliseconds >= stats.lastUpdateMilliseconds);

    controller.reset();
    const TrackingStats resetStats = controller.stats();
    QCOMPARE(resetStats.completedUpdates, quint64{0});
    QCOMPARE(resetStats.skippedFrames, quint64{0});
}

void TrackingControllerTests::resetDiscardsBlockedUpdateMetrics()
{
    auto shared = std::make_shared<BlockingState>();
    TrackingController controller(std::make_unique<BlockingEngine>(shared));
    TestFrame frame;

    controller.offerFrame(frame.buffer, frame.info);
    controller.selectTarget({35, 29});
    QTRY_COMPARE(controller.state(), TrackingState::Tracking);

    controller.offerFrame(frame.buffer, frame.info);
    QTRY_VERIFY(shared->updateEntered.load());

    controller.reset();
    const TrackingStats resetStats = controller.stats();
    QCOMPARE(resetStats.completedUpdates, quint64{0});
    QCOMPARE(resetStats.skippedFrames, quint64{0});
    QCOMPARE(resetStats.lastUpdateMilliseconds, 0.0);
    QCOMPARE(resetStats.lastResultAgeMilliseconds, 0.0);

    {
        std::lock_guard lock(shared->mutex);
        shared->release = true;
    }
    shared->released.notify_one();

    controller.offerFrame(frame.buffer, frame.info);
    controller.selectTarget({35, 29});
    QTRY_COMPARE(controller.state(), TrackingState::Tracking);

    const TrackingStats staleResultStats = controller.stats();
    QCOMPARE(staleResultStats.completedUpdates, quint64{0});
    QCOMPARE(staleResultStats.skippedFrames, quint64{0});
    QCOMPARE(staleResultStats.lastUpdateMilliseconds, 0.0);
    QCOMPARE(staleResultStats.lastResultAgeMilliseconds, 0.0);
}

void TrackingControllerTests::completedFailureEntersLost()
{
    TrackingController controller(std::make_unique<LosingEngine>());
    TestFrame frame;
    controller.offerFrame(frame.buffer, frame.info);
    controller.selectTarget({35, 29});
    QTRY_COMPARE(controller.state(), TrackingState::Tracking);

    controller.offerFrame(frame.buffer, frame.info);
    QTRY_COMPARE(controller.state(), TrackingState::Lost);
    QVERIFY(!controller.currentBox());
}

QTEST_MAIN(TrackingControllerTests)
#include "TrackingControllerTests.moc"
