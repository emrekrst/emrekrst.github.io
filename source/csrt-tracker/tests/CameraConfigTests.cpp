#include <QtTest>
#include <QFile>
#include <QTemporaryDir>

#include "config/CameraConfig.hpp"

class CameraConfigTests final : public QObject
{
    Q_OBJECT
private slots:
    void loadsKnownGoodDefaults();
    void retainsCsrtDefaultsWithoutTrackingGroup();
    void rejectsInvalidDepthWithKeyAndValue();
    void rejectsManualGainBelowOne();
    void rejectsInvalidScalar_data();
    void rejectsInvalidScalar();
    void rejectsInvalidTrackingSetting_data();
    void rejectsInvalidTrackingSetting();
    void reportsExactFloatTrackingRange_data();
    void reportsExactFloatTrackingRange();
    void rejectsUnknownGainMode();
    void rejectsMissingSetting();
    void reportsMissingAbsolutePath();
};

static QString writeConfig(QTemporaryDir& directory, const QByteArray& text)
{
    const QString path = directory.filePath("camera-tracker.ini");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(text) != text.size())
        return {};
    return path;
}

void CameraConfigTests::loadsKnownGoodDefaults()
{
    QTemporaryDir directory;
    const QString path = writeConfig(directory,
        "[camera]\nwidth=2028\nheight=1080\nfps=50\n"
        "sensor_depth=12\nexposure_us=5000\n"
        "gain_mode=auto\nanalogue_gain=1.0\n"
        "\n[tracking]\n"
        "use_hog=true\nuse_color_names=false\nuse_gray=true\nuse_rgb=false\n"
        "use_channel_weights=true\nuse_segmentation=false\n"
        "template_size=200\npadding=3\nnumber_of_scales=33\n"
        "scale_step=1.02\nadmm_iterations=4\npsr_threshold=0.035\n");
    const CameraConfigResult result = loadCameraConfig(path);
    QVERIFY2(result.config.has_value(), qPrintable(result.error));
    QCOMPARE(result.config->width, 2028);
    QCOMPARE(result.config->height, 1080);
    QCOMPARE(result.config->fps, 50);
    QCOMPARE(result.config->sensorDepth, 12);
    QCOMPARE(result.config->exposureMicroseconds, 5000);
    QCOMPARE(result.config->gainMode, CameraGainMode::Auto);
    QCOMPARE(result.config->analogueGain, 1.0);
    QCOMPARE(result.config->tracking.useHog, true);
    QCOMPARE(result.config->tracking.useColorNames, false);
    QCOMPARE(result.config->tracking.useGray, true);
    QCOMPARE(result.config->tracking.useRgb, false);
    QCOMPARE(result.config->tracking.useChannelWeights, true);
    QCOMPARE(result.config->tracking.useSegmentation, false);
    QCOMPARE(result.config->tracking.templateSize, 200.0F);
    QCOMPARE(result.config->tracking.padding, 3.0F);
    QCOMPARE(result.config->tracking.numberOfScales, 33);
    QCOMPARE(result.config->tracking.scaleStep, 1.02F);
    QCOMPARE(result.config->tracking.admmIterations, 4);
    QCOMPARE(result.config->tracking.psrThreshold, 0.035F);
}

void CameraConfigTests::retainsCsrtDefaultsWithoutTrackingGroup()
{
    QTemporaryDir directory;
    const QString path = writeConfig(directory,
        "[camera]\nwidth=2028\nheight=1080\nfps=50\n"
        "sensor_depth=12\nexposure_us=5000\n"
        "gain_mode=auto\nanalogue_gain=1.0\n");
    const CameraConfigResult result = loadCameraConfig(path);
    QVERIFY2(result.config.has_value(), qPrintable(result.error));
    QCOMPARE(result.config->tracking.useHog, true);
    QCOMPARE(result.config->tracking.useColorNames, true);
    QCOMPARE(result.config->tracking.useGray, true);
    QCOMPARE(result.config->tracking.useRgb, false);
    QCOMPARE(result.config->tracking.useChannelWeights, true);
    QCOMPARE(result.config->tracking.useSegmentation, true);
    QCOMPARE(result.config->tracking.templateSize, 200.0F);
    QCOMPARE(result.config->tracking.padding, 3.0F);
    QCOMPARE(result.config->tracking.numberOfScales, 33);
    QCOMPARE(result.config->tracking.scaleStep, 1.02F);
    QCOMPARE(result.config->tracking.admmIterations, 4);
    QCOMPARE(result.config->tracking.psrThreshold, 0.035F);
}

void CameraConfigTests::rejectsInvalidDepthWithKeyAndValue()
{
    QTemporaryDir directory;
    const QString path = writeConfig(directory,
        "[camera]\nwidth=2028\nheight=1080\nfps=50\n"
        "sensor_depth=9\nexposure_us=5000\n"
        "gain_mode=auto\nanalogue_gain=1.0\n");
    const CameraConfigResult result = loadCameraConfig(path);
    QVERIFY(!result.config);
    QVERIFY(result.error.contains("sensor_depth"));
    QVERIFY(result.error.contains("9"));
}

void CameraConfigTests::rejectsManualGainBelowOne()
{
    QTemporaryDir directory;
    const QString path = writeConfig(directory,
        "[camera]\nwidth=2028\nheight=1080\nfps=50\n"
        "sensor_depth=12\nexposure_us=5000\n"
        "gain_mode=manual\nanalogue_gain=0.5\n");
    const CameraConfigResult result = loadCameraConfig(path);
    QVERIFY(!result.config);
    QVERIFY(result.error.contains("analogue_gain"));
    QVERIFY(result.error.contains("0.5"));
}

void CameraConfigTests::rejectsInvalidScalar_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");

    QTest::newRow("zero width") << QByteArray("width") << QByteArray("0");
    QTest::newRow("negative height") << QByteArray("height") << QByteArray("-1");
    QTest::newRow("zero fps") << QByteArray("fps") << QByteArray("0");
    QTest::newRow("zero exposure") << QByteArray("exposure_us") << QByteArray("0");
    QTest::newRow("nonnumeric fps") << QByteArray("fps") << QByteArray("fast");
}

void CameraConfigTests::rejectsInvalidScalar()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);
    QTemporaryDir directory;
    QByteArray text =
        "[camera]\nwidth=2028\nheight=1080\nfps=50\n"
        "sensor_depth=12\nexposure_us=5000\n"
        "gain_mode=auto\nanalogue_gain=1.0\n";
    text.replace(key + "=" + (key == "height" ? "1080" :
                              key == "width" ? "2028" :
                              key == "fps" ? "50" : "5000"),
                 key + "=" + value);

    const CameraConfigResult result = loadCameraConfig(writeConfig(directory, text));
    QVERIFY(!result.config);
    QVERIFY(result.error.contains(QString::fromUtf8(key)));
    QVERIFY(result.error.contains(QString::fromUtf8(value)));
}

void CameraConfigTests::rejectsInvalidTrackingSetting_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("value");

    QTest::newRow("invalid HOG boolean") << QByteArray("use_hog") << QByteArray("maybe");
    QTest::newRow("invalid color names boolean") << QByteArray("use_color_names") << QByteArray("maybe");
    QTest::newRow("invalid gray boolean") << QByteArray("use_gray") << QByteArray("maybe");
    QTest::newRow("invalid RGB boolean") << QByteArray("use_rgb") << QByteArray("maybe");
    QTest::newRow("invalid channel weights boolean") << QByteArray("use_channel_weights") << QByteArray("maybe");
    QTest::newRow("invalid segmentation boolean") << QByteArray("use_segmentation") << QByteArray("maybe");
    QTest::newRow("zero template size") << QByteArray("template_size") << QByteArray("0");
    QTest::newRow("zero padding") << QByteArray("padding") << QByteArray("0");
    QTest::newRow("zero scale count") << QByteArray("number_of_scales") << QByteArray("0");
    QTest::newRow("unity scale step") << QByteArray("scale_step") << QByteArray("1");
    QTest::newRow("zero ADMM iterations") << QByteArray("admm_iterations") << QByteArray("0");
    QTest::newRow("negative PSR threshold") << QByteArray("psr_threshold") << QByteArray("-0.1");
}

void CameraConfigTests::rejectsInvalidTrackingSetting()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, value);
    QTemporaryDir directory;
    QByteArray text =
        "[camera]\nwidth=2028\nheight=1080\nfps=50\n"
        "sensor_depth=12\nexposure_us=5000\n"
        "gain_mode=auto\nanalogue_gain=1.0\n"
        "\n[tracking]\n"
        "use_hog=true\nuse_color_names=false\nuse_gray=true\nuse_rgb=false\n"
        "use_channel_weights=true\nuse_segmentation=false\n"
        "template_size=200\npadding=3\nnumber_of_scales=33\n"
        "scale_step=1.02\nadmm_iterations=4\npsr_threshold=0.035\n";
    text.replace(key + "=" + (key == "use_hog" || key == "use_gray"
                                  || key == "use_channel_weights" ? "true" :
                              key == "use_color_names" || key == "use_rgb"
                                  || key == "use_segmentation" ? "false" :
                              key == "template_size" ? "200" :
                              key == "padding" ? "3" :
                              key == "number_of_scales" ? "33" :
                              key == "scale_step" ? "1.02" :
                              key == "admm_iterations" ? "4" : "0.035"),
                 key + "=" + value);

    const CameraConfigResult result = loadCameraConfig(writeConfig(directory, text));
    QVERIFY(!result.config);
    QVERIFY(result.error.contains(QString::fromUtf8(key)));
    QVERIFY(result.error.contains(QString::fromUtf8(value)));
}

void CameraConfigTests::reportsExactFloatTrackingRange_data()
{
    QTest::addColumn<QByteArray>("key");
    QTest::addColumn<QByteArray>("validValue");
    QTest::addColumn<QByteArray>("invalidValue");
    QTest::addColumn<QString>("acceptedRange");

    QTest::newRow("template size lower boundary")
        << QByteArray("template_size") << QByteArray("16") << QByteArray("15.9")
        << QStringLiteral("[16, 2048]");
    QTest::newRow("template size upper boundary")
        << QByteArray("template_size") << QByteArray("2048") << QByteArray("2048.1")
        << QStringLiteral("[16, 2048]");
    QTest::newRow("padding lower boundary")
        << QByteArray("padding") << QByteArray("0.01") << QByteArray("0")
        << QStringLiteral("(0, 10]");
    QTest::newRow("padding upper boundary")
        << QByteArray("padding") << QByteArray("10") << QByteArray("10.1")
        << QStringLiteral("(0, 10]");
    QTest::newRow("scale step lower boundary")
        << QByteArray("scale_step") << QByteArray("1.01") << QByteArray("1")
        << QStringLiteral("(1, 1.5]");
    QTest::newRow("scale step upper boundary")
        << QByteArray("scale_step") << QByteArray("1.5") << QByteArray("1.51")
        << QStringLiteral("(1, 1.5]");
    QTest::newRow("PSR threshold lower boundary")
        << QByteArray("psr_threshold") << QByteArray("0") << QByteArray("-0.1")
        << QStringLiteral("[0, 1]");
    QTest::newRow("PSR threshold upper boundary")
        << QByteArray("psr_threshold") << QByteArray("1") << QByteArray("1.1")
        << QStringLiteral("[0, 1]");
}

void CameraConfigTests::reportsExactFloatTrackingRange()
{
    QFETCH(QByteArray, key);
    QFETCH(QByteArray, validValue);
    QFETCH(QByteArray, invalidValue);
    QFETCH(QString, acceptedRange);
    QTemporaryDir directory;
    QByteArray text =
        "[camera]\nwidth=2028\nheight=1080\nfps=50\n"
        "sensor_depth=12\nexposure_us=5000\n"
        "gain_mode=auto\nanalogue_gain=1.0\n"
        "\n[tracking]\n"
        "use_hog=true\nuse_color_names=false\nuse_gray=true\nuse_rgb=false\n"
        "use_channel_weights=true\nuse_segmentation=false\n"
        "template_size=200\npadding=3\nnumber_of_scales=33\n"
        "scale_step=1.02\nadmm_iterations=4\npsr_threshold=0.035\n";
    const QByteArray originalValue = key == "template_size" ? QByteArray("200")
        : key == "padding" ? QByteArray("3")
        : key == "scale_step" ? QByteArray("1.02") : QByteArray("0.035");
    text.replace(key + "=" + originalValue, key + "=" + validValue);
    QVERIFY2(loadCameraConfig(writeConfig(directory, text)).config.has_value(),
        qPrintable(QStringLiteral("accepted boundary rejected for %1").arg(QString::fromUtf8(key))));

    text.replace(key + "=" + validValue, key + "=" + invalidValue);
    const CameraConfigResult result = loadCameraConfig(writeConfig(directory, text));
    QVERIFY(!result.config);
    QVERIFY2(result.error.contains(acceptedRange), qPrintable(result.error));
}

void CameraConfigTests::rejectsUnknownGainMode()
{
    QTemporaryDir directory;
    const QString path = writeConfig(directory,
        "[camera]\nwidth=2028\nheight=1080\nfps=50\n"
        "sensor_depth=12\nexposure_us=5000\n"
        "gain_mode=Auto\nanalogue_gain=1.0\n");
    const CameraConfigResult result = loadCameraConfig(path);
    QVERIFY(!result.config);
    QVERIFY(result.error.contains("gain_mode"));
    QVERIFY(result.error.contains("Auto"));
}

void CameraConfigTests::rejectsMissingSetting()
{
    QTemporaryDir directory;
    const QString path = writeConfig(directory,
        "[camera]\nwidth=2028\nheight=1080\nfps=50\n"
        "sensor_depth=12\nexposure_us=5000\n"
        "gain_mode=auto\n");
    const CameraConfigResult result = loadCameraConfig(path);
    QVERIFY(!result.config);
    QVERIFY(result.error.contains("analogue_gain"));
}

void CameraConfigTests::reportsMissingAbsolutePath()
{
    const QString path = "/definitely/missing/camera-tracker.ini";
    const CameraConfigResult result = loadCameraConfig(path);
    QVERIFY(!result.config);
    QVERIFY(result.error.contains(path));
}

QTEST_MAIN(CameraConfigTests)
#include "CameraConfigTests.moc"
