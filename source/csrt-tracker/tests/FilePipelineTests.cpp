#include <QtTest>

#include <QElapsedTimer>
#include <QTemporaryDir>

#include <atomic>
#include <thread>

#include "pipeline/FilePipeline.hpp"
#include "tracking/ITrackingFrameSink.hpp"

class RecordingFrameSink final : public ITrackingFrameSink
{
public:
    void offerFrame(GstBuffer*, const GstVideoInfo& info) noexcept override
    {
        offers.fetch_add(1);
        width.store(GST_VIDEO_INFO_WIDTH(&info));
        height.store(GST_VIDEO_INFO_HEIGHT(&info));
    }

    std::atomic_int offers{0};
    std::atomic_int width{0};
    std::atomic_int height{0};
};

class FilePipelineTests final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void trackingSizeCaps4kWithoutUpscaling();
    void missingFileReportsAnErrorWithoutBuildingPipeline();
    void normalizedSeekRejectsOutOfRangeValues();
    void playbackAdvancesAtMediaClockRate();
    void seekMovesPlaybackToRequestedTime();
    void seekMovesPlaybackToNonKeyframeTime();
    void pausedSeekDeliversTheRequestedFrame();
    void playbackFeedsFramesAtMediaCadence();
    void decodedFrameContainsVideoPixels();

private:
    QTemporaryDir fixtureDirectory_;
    QString fixturePath_;
    QString longGopFixturePath_;
};

void FilePipelineTests::trackingSizeCaps4kWithoutUpscaling()
{
    QCOMPARE(normalizedTrackingSize({3840, 2160}), QSize(1920, 1080));
    QCOMPARE(normalizedTrackingSize({1280, 720}), QSize(1280, 720));
    QCOMPARE(normalizedTrackingSize({1080, 1920}), QSize(608, 1080));
}

void FilePipelineTests::initTestCase()
{
    gst_init(nullptr, nullptr);
    QVERIFY(fixtureDirectory_.isValid());
    fixturePath_ = fixtureDirectory_.filePath("clock-fixture.avi");

    const QString launch = QStringLiteral(
        "videotestsrc num-buffers=40 pattern=ball ! "
        "video/x-raw,width=64,height=64,framerate=10/1 ! "
        "jpegenc ! avimux ! filesink location=\"%1\"")
                               .arg(fixturePath_);
    GError* error = nullptr;
    GstElement* generator = gst_parse_launch(launch.toUtf8().constData(), &error);
    QVERIFY2(generator != nullptr, error != nullptr ? error->message : "fixture pipeline failed");
    g_clear_error(&error);

    GstBus* bus = gst_element_get_bus(generator);
    QVERIFY(gst_element_set_state(generator, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    GstMessage* message = gst_bus_timed_pop_filtered(
        bus, 10 * GST_SECOND, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    QVERIFY(message != nullptr);
    QCOMPARE(GST_MESSAGE_TYPE(message), GST_MESSAGE_EOS);
    gst_message_unref(message);
    gst_element_set_state(generator, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(generator);

    longGopFixturePath_ = fixtureDirectory_.filePath("long-gop-fixture.mp4");
    const QString longGopLaunch = QStringLiteral(
        "videotestsrc num-buffers=170 pattern=ball ! "
        "video/x-raw,width=64,height=64,framerate=10/1 ! "
        "x264enc key-int-max=80 speed-preset=ultrafast tune=zerolatency ! "
        "h264parse ! mp4mux ! filesink location=\"%1\"")
                                      .arg(longGopFixturePath_);
    error = nullptr;
    generator = gst_parse_launch(longGopLaunch.toUtf8().constData(), &error);
    QVERIFY2(generator != nullptr, error != nullptr ? error->message : "long-GOP fixture pipeline failed");
    g_clear_error(&error);

    bus = gst_element_get_bus(generator);
    QVERIFY(gst_element_set_state(generator, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    message = gst_bus_timed_pop_filtered(
        bus, 10 * GST_SECOND, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    QVERIFY(message != nullptr);
    QCOMPARE(GST_MESSAGE_TYPE(message), GST_MESSAGE_EOS);
    gst_message_unref(message);
    gst_element_set_state(generator, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(generator);
}

void FilePipelineTests::missingFileReportsAnErrorWithoutBuildingPipeline()
{
    FilePipeline pipeline;
    QSignalSpy spy(&pipeline, &FilePipeline::errorOccurred);

    QVERIFY(!pipeline.open("/definitely/missing/video.mp4"));

    QCOMPARE(spy.count(), 1);
    QVERIFY(spy.first().first().toString().contains("does not exist"));
}

void FilePipelineTests::normalizedSeekRejectsOutOfRangeValues()
{
    FilePipeline pipeline;

    QVERIFY(!pipeline.seek(-0.01));
    QVERIFY(!pipeline.seek(1.01));
}

void FilePipelineTests::playbackAdvancesAtMediaClockRate()
{
    FilePipeline pipeline;
    QVERIFY(pipeline.open(fixturePath_));

    QElapsedTimer wallClock;
    wallClock.start();
    QTest::qWait(500);

    const qint64 position = pipeline.positionMilliseconds();
    QVERIFY2(position >= 250, qPrintable(QStringLiteral("position was only %1 ms").arg(position)));
    QVERIFY2(position <= 1000,
        qPrintable(QStringLiteral("500 ms of wall time advanced media to %1 ms").arg(position)));
    QVERIFY(wallClock.elapsed() < 1500);
}

void FilePipelineTests::seekMovesPlaybackToRequestedTime()
{
    FilePipeline pipeline;
    QVERIFY(pipeline.open(fixturePath_));
    QTest::qWait(200);

    QVERIFY(pipeline.seek(0.5));
    QTest::qWait(150);

    const qint64 position = pipeline.positionMilliseconds();
    QVERIFY2(position >= 1900 && position <= 2600,
        qPrintable(QStringLiteral("50% seek landed at %1 ms for a 4000 ms file").arg(position)));
}

void FilePipelineTests::seekMovesPlaybackToNonKeyframeTime()
{
    FilePipeline pipeline;
    QVERIFY(pipeline.open(longGopFixturePath_));
    QTest::qWait(100);
    QVERIFY(pipeline.setPaused(true));
    QTest::qWait(50);

    const qint64 duration = pipeline.durationMilliseconds();
    QVERIFY2(duration >= 16000, qPrintable(QStringLiteral("fixture duration was %1 ms").arg(duration)));

    for (const qint64 requested : {qint64{3000}, qint64{12000}}) {
        QVERIFY(pipeline.seek(static_cast<double>(requested) / duration));
        QTest::qWait(150);

        const qint64 actual = pipeline.positionMilliseconds();
        QVERIFY2(std::abs(actual - requested) <= 150,
            qPrintable(QStringLiteral("seek to %1 ms landed at keyframe %2 ms")
                           .arg(requested)
                           .arg(actual)));
    }
}

void FilePipelineTests::pausedSeekDeliversTheRequestedFrame()
{
    FilePipeline pipeline;
    pipeline.setDisplaySize({320, 180});
    QSignalSpy frames(&pipeline, &FilePipeline::frameReady);

    QVERIFY(pipeline.open(longGopFixturePath_));
    QTRY_VERIFY_WITH_TIMEOUT(frames.count() > 0, 1500);
    QVERIFY(pipeline.setPaused(true));
    QTest::qWait(100);
    frames.clear();

    const qint64 duration = pipeline.durationMilliseconds();
    QVERIFY(duration > 0);
    QVERIFY(pipeline.seek(12000.0 / duration));

    QTRY_VERIFY_WITH_TIMEOUT(frames.count() > 0, 1500);
    const QImage soughtFrame = qvariant_cast<QImage>(frames.last().first());
    QVERIFY(!soughtFrame.isNull());
}

void FilePipelineTests::playbackFeedsFramesAtMediaCadence()
{
    RecordingFrameSink sink;
    FilePipeline pipeline;
    pipeline.setTrackingFrameSink(&sink);

    QVERIFY(pipeline.open(fixturePath_));
    QTest::qWait(1200);
    pipeline.stop();

    QVERIFY2(sink.offers >= 8,
        qPrintable(QStringLiteral("10 fps playback delivered only %1 frames in 1.2 s").arg(sink.offers.load())));
    QVERIFY2(sink.offers <= 16,
        qPrintable(QStringLiteral("10 fps playback delivered %1 frames in 1.2 s").arg(sink.offers.load())));
    QCOMPARE(sink.width.load(), 64);
    QCOMPARE(sink.height.load(), 64);
}

void FilePipelineTests::decodedFrameContainsVideoPixels()
{
    FilePipeline pipeline;
    pipeline.setDisplaySize({320, 180});
    QSignalSpy frames(&pipeline, &FilePipeline::frameReady);

    QVERIFY(pipeline.open(fixturePath_));
    QTRY_VERIFY_WITH_TIMEOUT(frames.count() > 0, 1500);

    const QImage image = qvariant_cast<QImage>(frames.first().first());
    QVERIFY(!image.isNull());
    QVERIFY(image.width() > 0);
    QVERIFY(image.height() > 0);

    const QRgb first = image.pixel(0, 0);
    bool foundDifferentPixel = false;
    for (int y = 0; y < image.height() && !foundDifferentPixel; y += 8) {
        for (int x = 0; x < image.width(); x += 8) {
            if (image.pixel(x, y) != first) {
                foundDifferentPixel = true;
                break;
            }
        }
    }
    QVERIFY2(foundDifferentPixel, "decoded frame was a single flat colour");
}

QTEST_MAIN(FilePipelineTests)
#include "FilePipelineTests.moc"
