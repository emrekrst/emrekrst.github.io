#include <QtTest>

#include <opencv2/imgproc.hpp>

#include "tracking/CsrtTracker.hpp"
#include "tracking/CsrtTrackingEngine.hpp"

namespace {

cv::Mat makeFrame(const cv::Rect& object)
{
    cv::Mat frame(360, 640, CV_8UC3, cv::Scalar(45, 48, 52));
    cv::Mat texture(object.size(), CV_8UC3);
    cv::RNG random(0x5eed);
    random.fill(texture, cv::RNG::UNIFORM, cv::Scalar(20, 40, 80), cv::Scalar(235, 225, 215));
    texture.copyTo(frame(object));
    cv::rectangle(frame, object, cv::Scalar(230, 220, 40), 2);
    return frame;
}

cv::Mat makeSelectableFrame(const cv::Rect& object)
{
    cv::Mat frame(360, 640, CV_8UC3, cv::Scalar(130, 145, 160));
    cv::rectangle(frame, object, cv::Scalar(35, 75, 205), cv::FILLED);
    cv::rectangle(frame, object, cv::Scalar(10, 20, 40), 2);
    cv::circle(frame, {object.x + object.width / 2, object.y + object.height / 2},
               4, cv::Scalar(50, 90, 220), cv::FILLED);
    return frame;
}

} // namespace

class CsrtTrackerTests final : public QObject
{
    Q_OBJECT
private slots:
    void mapsBooleanCsrtConfigFieldsIndependently_data();
    void mapsBooleanCsrtConfigFieldsIndependently();
    void mapsNumericCsrtConfigFieldsToOpenCvParams();
    void configuredPsrThresholdControlsInitializedTracker();
    void tracksTranslatedTexture();
    void reportsLossAfterFullOcclusion();
    void engineSelectsThenTracksFromOneClick();
};

void CsrtTrackerTests::mapsBooleanCsrtConfigFieldsIndependently_data()
{
    QTest::addColumn<bool>("useHog");
    QTest::addColumn<bool>("useColorNames");
    QTest::addColumn<bool>("useGray");
    QTest::addColumn<bool>("useRgb");
    QTest::addColumn<bool>("useChannelWeights");
    QTest::addColumn<bool>("useSegmentation");

    QTest::newRow("HOG") << true << false << false << false << false << false;
    QTest::newRow("color names") << false << true << false << false << false << false;
    QTest::newRow("gray") << false << false << true << false << false << false;
    QTest::newRow("RGB") << false << false << false << true << false << false;
    QTest::newRow("channel weights") << false << false << false << false << true << false;
    QTest::newRow("segmentation") << false << false << false << false << false << true;
}

void CsrtTrackerTests::mapsBooleanCsrtConfigFieldsIndependently()
{
    QFETCH(bool, useHog);
    QFETCH(bool, useColorNames);
    QFETCH(bool, useGray);
    QFETCH(bool, useRgb);
    QFETCH(bool, useChannelWeights);
    QFETCH(bool, useSegmentation);
    const CsrtConfig config{
        .useHog = useHog,
        .useColorNames = useColorNames,
        .useGray = useGray,
        .useRgb = useRgb,
        .useChannelWeights = useChannelWeights,
        .useSegmentation = useSegmentation,
    };

    const cv::TrackerCSRT::Params params = buildOpenCvCsrtParams(config);

    QCOMPARE(params.use_hog, useHog);
    QCOMPARE(params.use_color_names, useColorNames);
    QCOMPARE(params.use_gray, useGray);
    QCOMPARE(params.use_rgb, useRgb);
    QCOMPARE(params.use_channel_weights, useChannelWeights);
    QCOMPARE(params.use_segmentation, useSegmentation);
}

void CsrtTrackerTests::mapsNumericCsrtConfigFieldsToOpenCvParams()
{
    const CsrtConfig config{
        .templateSize = 512.0F,
        .padding = 4.5F,
        .numberOfScales = 17,
        .scaleStep = 1.3F,
        .admmIterations = 7,
        .psrThreshold = 0.6F,
    };

    const cv::TrackerCSRT::Params params = buildOpenCvCsrtParams(config);

    QCOMPARE(params.template_size, 512.0F);
    QCOMPARE(params.padding, 4.5F);
    QCOMPARE(params.number_of_scales, 17);
    QCOMPARE(params.scale_step, 1.3F);
    QCOMPARE(params.admm_iterations, 7);
    QCOMPARE(params.psr_threshold, 0.6F);
}

void CsrtTrackerTests::configuredPsrThresholdControlsInitializedTracker()
{
    CsrtConfig config;
    config.psrThreshold = 1.0F;
    CsrtTracker tracker(config);
    QVERIFY(tracker.initialize(makeFrame({80, 120, 48, 28}), {80, 120, 48, 28}));

    const auto next = tracker.update(makeFrame({85, 122, 48, 28}));
    QVERIFY(!next.has_value());
}

void CsrtTrackerTests::tracksTranslatedTexture()
{
    CsrtTracker tracker;
    QVERIFY(tracker.initialize(makeFrame({80, 120, 48, 28}), {80, 120, 48, 28}));

    for (int i = 1; i <= 8; ++i) {
        const FrameRect truth{80.0 + 5 * i, 120.0 + 2 * i, 48.0, 28.0};
        const auto box = tracker.update(makeFrame({80 + 5 * i, 120 + 2 * i, 48, 28}));
        QVERIFY(box.has_value());
        QVERIFY2(intersectionOverUnion(*box, truth) >= 0.45, "CSRT drifted off the translated object");
    }
}

void CsrtTrackerTests::reportsLossAfterFullOcclusion()
{
    CsrtTracker tracker;
    QVERIFY(tracker.initialize(makeFrame({80, 120, 48, 28}), {80, 120, 48, 28}));

    const cv::Mat blank(360, 640, CV_8UC3, cv::Scalar(45, 48, 52));
    bool lost = false;
    for (int i = 0; i < 10; ++i) {
        if (!tracker.update(blank)) {
            lost = true;
            break;
        }
    }
    QVERIFY(lost);
}

void CsrtTrackerTests::engineSelectsThenTracksFromOneClick()
{
    CsrtTrackingEngine engine;
    const cv::Mat first = makeSelectableFrame({80, 120, 48, 28});
    const auto selection = engine.select(first, {104, 134});
    QVERIFY(selection.has_value());
    QVERIFY(engine.initialize(first, selection->box));

    const auto next = engine.update(makeSelectableFrame({85, 122, 48, 28}));
    QVERIFY(next.has_value());
    QVERIFY(intersectionOverUnion(*next, {85, 122, 48, 28}) >= 0.45);
}

QTEST_MAIN(CsrtTrackerTests)
#include "CsrtTrackerTests.moc"
