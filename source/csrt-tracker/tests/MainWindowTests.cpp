#include <QtTest>

#include <QPushButton>
#include <QFile>
#include <QLabel>
#include <QSlider>
#include <QTemporaryDir>

#include "ui/MainWindow.hpp"
#include "ui/VideoCanvas.hpp"

class MainWindowTests final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void recordedDemoTracksMovingTarget();
    void cameraAvailabilityMatchesPlatform();
    void transportStartsDisabled();
    void videoCanvasIsTheVisibleFileSurface();
    void cameraUsesSharedOneClickCanvas();
    void invalidCameraConfigDoesNotDisableFileControlsForever();
    void validConfigIsRetainedForCameraStart();
    void runtimeTelemetryStartsUnavailable();
    void changingVideoSourceResetsRuntimeTelemetry();

private:
    QTemporaryDir fixtureDirectory_;
    QString firstFixturePath_;
    QString secondFixturePath_;
};

void MainWindowTests::initTestCase()
{
    gst_init(nullptr, nullptr);
    QVERIFY(fixtureDirectory_.isValid());

    const auto createFixture = [this](const QString& name, int pattern) -> QString {
        const QString path = fixtureDirectory_.filePath(name);
        const QString launch = QStringLiteral(
            "videotestsrc num-buffers=80 pattern=%1 ! "
            "video/x-raw,width=64,height=64,framerate=10/1 ! "
            "jpegenc ! avimux ! filesink location=\"%2\"")
                                   .arg(pattern)
                                   .arg(path);
        GError* error = nullptr;
        GstElement* generator = gst_parse_launch(launch.toUtf8().constData(), &error);
        g_clear_error(&error);
        if (generator == nullptr) {
            return {};
        }

        GstBus* bus = gst_element_get_bus(generator);
        const bool started =
            gst_element_set_state(generator, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE;
        GstMessage* message = gst_bus_timed_pop_filtered(
            bus, 10 * GST_SECOND, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
        const bool reachedEnd = message != nullptr && GST_MESSAGE_TYPE(message) == GST_MESSAGE_EOS;
        if (message != nullptr) {
            gst_message_unref(message);
        }
        gst_element_set_state(generator, GST_STATE_NULL);
        gst_object_unref(bus);
        gst_object_unref(generator);
        return started && reachedEnd ? path : QString{};
    };

    firstFixturePath_ = createFixture(QStringLiteral("first.avi"), 0);
    secondFixturePath_ = createFixture(QStringLiteral("second.avi"), 1);
    QVERIFY(!firstFixturePath_.isEmpty());
    QVERIFY(!secondFixturePath_.isEmpty());
}

void MainWindowTests::cameraAvailabilityMatchesPlatform()
{
    MainWindow window;
    auto* cameraButton = window.findChild<QPushButton*>("cameraButton");
    QVERIFY(cameraButton != nullptr);

#ifdef Q_OS_MACOS
    QVERIFY(!cameraButton->isEnabled());
    QVERIFY(cameraButton->toolTip().contains("Raspberry Pi"));
#else
    QCOMPARE(cameraButton->isEnabled(), CameraPipeline::isAvailable());
#endif
}

void MainWindowTests::videoCanvasIsTheVisibleFileSurface()
{
    MainWindow window;
    window.show();
    QCoreApplication::processEvents();

    auto* canvas = window.findChild<VideoCanvas*>("videoCanvas");
    QVERIFY(canvas != nullptr);
    QVERIFY(canvas->isVisible());
    QVERIFY(window.findChild<QWidget*>("videoSurface") == nullptr);
}

void MainWindowTests::transportStartsDisabled()
{
    MainWindow window;

    auto* playPause = window.findChild<QPushButton*>("playPauseButton");
    auto* slider = window.findChild<QSlider*>("positionSlider");
    QVERIFY(playPause != nullptr);
    QVERIFY(slider != nullptr);
    QVERIFY(!playPause->isEnabled());
    QVERIFY(!slider->isEnabled());
}

void MainWindowTests::cameraUsesSharedOneClickCanvas()
{
    MainWindow window;
    QCOMPARE(window.findChildren<VideoCanvas*>().size(), 1);
    QVERIFY(window.findChild<QWidget*>("cameraOverlay") == nullptr);
}

void MainWindowTests::invalidCameraConfigDoesNotDisableFileControlsForever()
{
    MainWindow window({std::nullopt,
        "Invalid camera setting sensor_depth=9; expected 8, 10, or 12."});
    auto* camera = window.findChild<QPushButton*>("cameraButton");
    auto* openFile = window.findChild<QPushButton*>("openFileButton");
    QVERIFY(camera != nullptr);
    QVERIFY(openFile != nullptr);
    QVERIFY(!camera->isEnabled());
    QVERIFY(camera->toolTip().contains("sensor_depth=9"));
    QVERIFY(openFile->isEnabled());
}

void MainWindowTests::validConfigIsRetainedForCameraStart()
{
    CameraConfig config;
    config.fps = 74;
    MainWindow window({config, {}});
    QCOMPARE(window.cameraConfigForTest().fps, 74);
}

void MainWindowTests::runtimeTelemetryStartsUnavailable()
{
    MainWindow window;

    auto* telemetry = window.findChild<QLabel*>("runtimeTelemetry");
    QVERIFY(telemetry != nullptr);
    QCOMPARE(telemetry->text(),
        QString("CAM -- | VIEW -- | TRACK -- | CSRT -- | AGE -- | SKIP --"));
}

void MainWindowTests::changingVideoSourceResetsRuntimeTelemetry()
{
    MainWindow window;
    window.telemetryTimer_.stop();
    auto* telemetry = window.findChild<QLabel*>("runtimeTelemetry");
    QVERIFY(telemetry != nullptr);

    QVERIFY(window.openVideoFile(firstFixturePath_));
    QTRY_VERIFY_WITH_TIMEOUT(window.displayedFrames_ >= quint64{3}, 1500);
    QVERIFY(QMetaObject::invokeMethod(&window, "updateRuntimeTelemetry", Qt::DirectConnection));
    QTest::qWait(300);
    const quint64 previousSourceDisplayedAtSample = window.displayedFrames_;
    QVERIFY(QMetaObject::invokeMethod(&window, "updateRuntimeTelemetry", Qt::DirectConnection));
    QVERIFY(!telemetry->text().startsWith(QStringLiteral("CAM --")));

    QVERIFY(window.openVideoFile(secondFixturePath_));
    QCOMPARE(window.displayedFrames_, quint64{0});
    QCOMPARE(telemetry->text(),
        QString("CAM -- | VIEW -- | TRACK -- | CSRT -- | AGE -- | SKIP --"));

    QTRY_VERIFY_WITH_TIMEOUT(window.displayedFrames_ > previousSourceDisplayedAtSample, 1500);
    QCOMPARE(telemetry->text(),
        QString("CAM -- | VIEW -- | TRACK -- | CSRT -- | AGE -- | SKIP --"));
    QVERIFY(QMetaObject::invokeMethod(&window, "updateRuntimeTelemetry", Qt::DirectConnection));
    QCOMPARE(telemetry->text(),
        QString("CAM 0.0 | VIEW 0.0 | TRACK -- | CSRT -- | AGE -- | SKIP --"));
}

void MainWindowTests::recordedDemoTracksMovingTarget()
{
    const QString sample = QStringLiteral(CSRT_SAMPLE_PATH);
    QVERIFY2(QFile::exists(sample), "generate samples/moving-target.avi first");
    MainWindow window;
    window.show();
    QVERIFY(window.openVideoFile(sample));
    QTRY_VERIFY_WITH_TIMEOUT(window.displayedFrames_ > 0, 2000);
    QVERIFY(window.filePipeline_.setPaused(true));
    QVERIFY(window.filePipeline_.seek(0.0));
    QTest::qWait(150);
    window.selectTarget({104, 134});
    QTRY_COMPARE_WITH_TIMEOUT(window.tracker_.state(), TrackingState::Tracking, 3000);
    QVERIFY(window.filePipeline_.setPaused(false));
    QTRY_VERIFY_WITH_TIMEOUT(window.tracker_.stats().completedUpdates >= 15, 5000);
    QVERIFY(window.tracker_.currentBox().has_value());
    const double seconds = window.filePipeline_.positionMilliseconds() / 1000.0;
    const FrameRect truth{80.0 + 20.0 * seconds, 120.0, 48.0, 28.0};
    QVERIFY2(intersectionOverUnion(*window.tracker_.currentBox(), truth) > 0.4,
             "recorded-video tracking drifted away from the known moving object");
    const QString screenshot = qEnvironmentVariable("CSRT_DEMO_SCREENSHOT");
    if (!screenshot.isEmpty()) QVERIFY(window.grab().save(screenshot));
}

QTEST_MAIN(MainWindowTests)
#include "MainWindowTests.moc"
