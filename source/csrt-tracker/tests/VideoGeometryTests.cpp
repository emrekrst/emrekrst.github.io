#include <QtTest>

#include "ui/VideoGeometry.hpp"

class VideoGeometryTests final : public QObject
{
    Q_OBJECT

private slots:
    void centerClickMapsToFrameCenter();
    void letterboxClickIsRejected();
    void frameBoxMapsIntoDisplayedVideo();
};

void VideoGeometryTests::centerClickMapsToFrameCenter()
{
    const auto mapped = mapWidgetPointToFrame({500.0, 250.0}, {1000.0, 500.0}, {1920, 1080});

    QVERIFY(mapped.has_value());
    QCOMPARE(mapped->x, 960.0);
    QCOMPARE(mapped->y, 540.0);
}

void VideoGeometryTests::letterboxClickIsRejected()
{
    QVERIFY(!mapWidgetPointToFrame({500.0, 10.0}, {1000.0, 1000.0}, {1920, 1080}));
}

void VideoGeometryTests::frameBoxMapsIntoDisplayedVideo()
{
    const QRectF mapped = mapFrameRectToWidget(
        {860.0, 490.0, 200.0, 100.0}, {1000.0, 1000.0}, {1920, 1080});

    QCOMPARE(mapped.center(), QPointF(500.0, 500.0));
    QCOMPARE(mapped.width(), 200.0 * (1000.0 / 1920.0));
    QCOMPARE(mapped.height(), 100.0 * (1000.0 / 1920.0));
}

QTEST_MAIN(VideoGeometryTests)
#include "VideoGeometryTests.moc"
