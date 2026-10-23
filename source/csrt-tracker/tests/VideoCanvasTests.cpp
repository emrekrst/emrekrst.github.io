#include <QtTest>

#include "ui/VideoCanvas.hpp"

class VideoCanvasTests final : public QObject
{
    Q_OBJECT

private slots:
    void paintsVideoPixelsInsteadOfAnOpaqueOverlay();
    void singleClickMapsToTrackingFrameCoordinates();
};

void VideoCanvasTests::paintsVideoPixelsInsteadOfAnOpaqueOverlay()
{
    VideoCanvas canvas;
    canvas.resize(640, 360);

    QImage redFrame(640, 360, QImage::Format_RGB32);
    redFrame.fill(QColor(220, 30, 40));
    canvas.setFrame(redFrame);
    QCOMPARE(canvas.frameSize(), redFrame.size());
    canvas.show();
    QCoreApplication::processEvents();

    const QImage rendered = canvas.grab().toImage();
    const QColor center = rendered.pixelColor(320, 180);
    QVERIFY(center.red() > 180);
    QVERIFY(center.green() < 80);
    QVERIFY(center.blue() < 80);
}

void VideoCanvasTests::singleClickMapsToTrackingFrameCoordinates()
{
    VideoCanvas canvas;
    canvas.resize(640, 360);
    canvas.setFrameSize({1920, 1080});
    QImage displayFrame(640, 360, QImage::Format_RGB32);
    displayFrame.fill(Qt::red);
    canvas.setFrame(displayFrame);
    QCOMPARE(canvas.frameSize(), QSize(1920, 1080));

    QSignalSpy selected(&canvas, &VideoCanvas::targetSelected);
    QTest::mouseClick(&canvas, Qt::LeftButton, {}, QPoint(320, 180));

    QCOMPARE(selected.count(), 1);
    const auto point = qvariant_cast<FramePoint>(selected.first().first());
    QVERIFY(std::abs(point.x - 960.0) < 2.0);
    QVERIFY(std::abs(point.y - 540.0) < 2.0);
}

QTEST_MAIN(VideoCanvasTests)
#include "VideoCanvasTests.moc"
