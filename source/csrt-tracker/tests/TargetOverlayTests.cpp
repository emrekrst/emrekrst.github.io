#include <QtTest>

#include "ui/TargetOverlay.hpp"

class TargetOverlayTests final : public QObject
{
    Q_OBJECT

private slots:
    void clickInsideVideoEmitsFramePoint();
    void clickInLetterboxDoesNotSelectTarget();
};

void TargetOverlayTests::clickInsideVideoEmitsFramePoint()
{
    TargetOverlay overlay;
    overlay.resize(640, 360);
    overlay.setFrameSize({1920, 1080});
    QSignalSpy spy(&overlay, &TargetOverlay::targetSelected);

    QTest::mouseClick(&overlay, Qt::LeftButton, {}, {320, 180});

    QCOMPARE(spy.count(), 1);
    const FramePoint point = spy.first().first().value<FramePoint>();
    QCOMPARE(point, FramePoint({960.0, 540.0}));
}

void TargetOverlayTests::clickInLetterboxDoesNotSelectTarget()
{
    TargetOverlay overlay;
    overlay.resize(640, 640);
    overlay.setFrameSize({1920, 1080});
    QSignalSpy spy(&overlay, &TargetOverlay::targetSelected);

    QTest::mouseClick(&overlay, Qt::LeftButton, {}, {320, 20});

    QCOMPARE(spy.count(), 0);
}

QTEST_MAIN(TargetOverlayTests)
#include "TargetOverlayTests.moc"
