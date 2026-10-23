#include <QtTest>

#include "config/CameraConfig.hpp"
#include "pipeline/CameraPipeline.hpp"

class CameraPipelineTests final : public QObject
{
    Q_OBJECT

private slots:
    void defaultConfigPreservesLowLatencyBaseline();
    void manualGainIsEncodedExactly();
    void speedExperimentSelectsEightBitMode();
    void cameraAvailabilityMatchesPlatform();
    void observedFrameCountGrowsForShortGstreamerFixture();
};

class ProbePipeline final : public AbstractPipeline
{
public:
    bool startFixture()
    {
        GError* error = nullptr;
        GstElement* pipeline = gst_parse_launch(
            "videotestsrc num-buffers=10 ! "
            "video/x-raw,format=BGRx,width=8,height=8,framerate=60/1 ! "
            "queue name=tracker_queue ! fakesink name=video_sink sync=false",
            &error);
        if (pipeline == nullptr) {
            g_clear_error(&error);
            return false;
        }

        GstElement* sink = gst_bin_get_by_name(GST_BIN(pipeline), "video_sink");
        GstElement* queue = gst_bin_get_by_name(GST_BIN(pipeline), "tracker_queue");
        const bool started = adoptAndPlay(pipeline, sink, queue, 0);
        if (sink != nullptr) {
            gst_object_unref(sink);
        }
        if (queue != nullptr) {
            gst_object_unref(queue);
        }
        return started;
    }
};

void CameraPipelineTests::defaultConfigPreservesLowLatencyBaseline()
{
    const QString pipeline = CameraPipeline::description(CameraConfig{});
    QVERIFY(pipeline.contains(
        "sensor-config=\"sensor/config,width=2028,height=1080,depth=12\""));
    QVERIFY(pipeline.contains(
        "video/x-raw,format=BGRx,width=2028,height=1080,framerate=50/1"));
    QVERIFY(pipeline.contains("exposure-time-mode=manual exposure-time=5000"));
    QVERIFY(pipeline.contains("analogue-gain-mode=auto"));
    QVERIFY(pipeline.contains(
        "queue name=tracker_queue max-size-buffers=1 max-size-bytes=0 "
        "max-size-time=0 leaky=downstream"));
    QVERIFY(pipeline.endsWith(
        "appsink name=video_sink emit-signals=true sync=false "
        "max-buffers=1 drop=true"));
    QVERIFY(!pipeline.contains("videoconvert"));
    QVERIFY(!pipeline.contains("x264"));
}

void CameraPipelineTests::manualGainIsEncodedExactly()
{
    CameraConfig config;
    config.gainMode = CameraGainMode::Manual;
    config.analogueGain = 2.25;
    const QString pipeline = CameraPipeline::description(config);
    QVERIFY(pipeline.contains("analogue-gain-mode=manual analogue-gain=2.25"));
}

void CameraPipelineTests::speedExperimentSelectsEightBitMode()
{
    CameraConfig config;
    config.fps = 90;
    config.sensorDepth = 8;
    const QString pipeline = CameraPipeline::description(config);
    QVERIFY(pipeline.contains("depth=8"));
    QVERIFY(pipeline.contains("framerate=90/1"));
}

void CameraPipelineTests::cameraAvailabilityMatchesPlatform()
{
#ifdef Q_OS_MACOS
    QVERIFY(!CameraPipeline::isAvailable());
    QVERIFY(!CameraPipeline::unavailabilityReason().isEmpty());
#else
    QCOMPARE(CameraPipeline::isAvailable(),
             CameraPipeline::unavailabilityReason().isEmpty());
#endif
}

void CameraPipelineTests::observedFrameCountGrowsForShortGstreamerFixture()
{
    gst_init(nullptr, nullptr);
    ProbePipeline pipeline;

    QVERIFY(pipeline.startFixture());
    QTRY_VERIFY_WITH_TIMEOUT(pipeline.observedFrameCount() > 0, 2000);
}

QTEST_MAIN(CameraPipelineTests)
#include "CameraPipelineTests.moc"
