#include <QtTest>

#include <opencv2/core/mat.hpp>

#include "tracking/PlaceholderTracker.hpp"

class PlaceholderTrackerTests final : public QObject
{
    Q_OBJECT

private slots:
    void selectionCreatesClampedStaticBox();
    void updateKeepsTheSelectionStatic();
    void resetClearsTheSelection();
};

void PlaceholderTrackerTests::selectionCreatesClampedStaticBox()
{
    PlaceholderTracker tracker({120.0, 80.0});

    tracker.initialize({10.0, 10.0}, {1920, 1080});

    QCOMPARE(tracker.currentBox(), std::optional<FrameRect>({0.0, 0.0, 120.0, 80.0}));
}

void PlaceholderTrackerTests::updateKeepsTheSelectionStatic()
{
    PlaceholderTracker tracker({120.0, 80.0});
    tracker.initialize({960.0, 540.0}, {1920, 1080});

    const auto updated = tracker.update(cv::Mat(1080, 1920, CV_8UC4));

    QCOMPARE(updated, std::optional<FrameRect>({900.0, 500.0, 120.0, 80.0}));
}

void PlaceholderTrackerTests::resetClearsTheSelection()
{
    PlaceholderTracker tracker;
    tracker.initialize({320.0, 180.0}, {640, 360});

    tracker.reset();

    QVERIFY(!tracker.currentBox());
}

QTEST_MAIN(PlaceholderTrackerTests)
#include "PlaceholderTrackerTests.moc"
