#include "config/CameraConfig.hpp"

#include <cmath>

#include <QFileInfo>
#include <QSettings>

namespace {

CameraConfigResult invalidSetting(const QString& key, const QString& value,
                                  const QString& expected)
{
    return {std::nullopt,
            QStringLiteral("Invalid camera setting %1=%2; expected %3.")
                .arg(key, value, expected)};
}

bool readPositiveInteger(QSettings& settings, const QString& key, int& output,
                         CameraConfigResult& error)
{
    if (!settings.contains(key)) {
        error = invalidSetting(key, QStringLiteral("<missing>"),
                               QStringLiteral("a positive integer"));
        return false;
    }

    const QString value = settings.value(key).toString();
    bool ok = false;
    const int parsed = value.toInt(&ok);
    if (!ok || parsed <= 0) {
        error = invalidSetting(key, value, QStringLiteral("a positive integer"));
        return false;
    }
    output = parsed;
    return true;
}

bool readBoolean(QSettings& settings, const QString& key, bool& output,
                 CameraConfigResult& error)
{
    if (!settings.contains(key)) {
        error = invalidSetting(key, QStringLiteral("<missing>"),
                               QStringLiteral("true or false"));
        return false;
    }

    const QString value = settings.value(key).toString();
    if (value == QStringLiteral("true")) {
        output = true;
        return true;
    }
    if (value == QStringLiteral("false")) {
        output = false;
        return true;
    }
    error = invalidSetting(key, value, QStringLiteral("true or false"));
    return false;
}

bool readIntegerInRange(QSettings& settings, const QString& key, int minimum,
                        int maximum, int& output, CameraConfigResult& error)
{
    if (!settings.contains(key)) {
        error = invalidSetting(key, QStringLiteral("<missing>"),
                               QStringLiteral("an integer in [%1, %2]")
                                   .arg(minimum)
                                   .arg(maximum));
        return false;
    }

    const QString value = settings.value(key).toString();
    bool ok = false;
    const int parsed = value.toInt(&ok);
    if (!ok || parsed < minimum || parsed > maximum) {
        error = invalidSetting(key, value,
                               QStringLiteral("an integer in [%1, %2]")
                                   .arg(minimum)
                                   .arg(maximum));
        return false;
    }
    output = parsed;
    return true;
}

bool readFloatInRange(QSettings& settings, const QString& key, float minimum,
                      float maximum, bool minimumInclusive, const QString& acceptedRange,
                      float& output,
                      CameraConfigResult& error)
{
    if (!settings.contains(key)) {
        error = invalidSetting(key, QStringLiteral("<missing>"),
                               QStringLiteral("a number in %1").arg(acceptedRange));
        return false;
    }

    const QString value = settings.value(key).toString();
    bool ok = false;
    const float parsed = value.toFloat(&ok);
    if (!ok || !std::isfinite(parsed)
        || (minimumInclusive ? parsed < minimum : parsed <= minimum)
        || parsed > maximum) {
        error = invalidSetting(key, value,
                               QStringLiteral("a number in %1").arg(acceptedRange));
        return false;
    }
    output = parsed;
    return true;
}

bool readTrackingConfig(QSettings& settings, CsrtConfig& config,
                        CameraConfigResult& error)
{
    if (!settings.childGroups().contains(QStringLiteral("tracking"))) {
        return true;
    }

    settings.beginGroup(QStringLiteral("tracking"));
    const bool valid =
        readBoolean(settings, QStringLiteral("use_hog"), config.useHog, error)
        && readBoolean(settings, QStringLiteral("use_color_names"), config.useColorNames, error)
        && readBoolean(settings, QStringLiteral("use_gray"), config.useGray, error)
        && readBoolean(settings, QStringLiteral("use_rgb"), config.useRgb, error)
        && readBoolean(settings, QStringLiteral("use_channel_weights"), config.useChannelWeights, error)
        && readBoolean(settings, QStringLiteral("use_segmentation"), config.useSegmentation, error)
        && readFloatInRange(settings, QStringLiteral("template_size"), 16.0F, 2048.0F,
                            true, QStringLiteral("[16, 2048]"), config.templateSize, error)
        && readFloatInRange(settings, QStringLiteral("padding"), 0.0F, 10.0F,
                            false, QStringLiteral("(0, 10]"), config.padding, error)
        && readIntegerInRange(settings, QStringLiteral("number_of_scales"), 1, 101,
                              config.numberOfScales, error)
        && readFloatInRange(settings, QStringLiteral("scale_step"), 1.0F, 1.5F,
                            false, QStringLiteral("(1, 1.5]"), config.scaleStep, error)
        && readIntegerInRange(settings, QStringLiteral("admm_iterations"), 1, 20,
                              config.admmIterations, error)
        && readFloatInRange(settings, QStringLiteral("psr_threshold"), 0.0F, 1.0F,
                            true, QStringLiteral("[0, 1]"), config.psrThreshold, error);
    settings.endGroup();
    return valid;
}

} // namespace

CameraConfigResult loadCameraConfig(const QString& path)
{
    const QFileInfo file(path);
    const QString absolutePath = file.absoluteFilePath();
    if (!file.isFile()) {
        return {std::nullopt,
                QStringLiteral("Camera configuration file not found: %1")
                    .arg(absolutePath)};
    }

    QSettings settings(absolutePath, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("camera"));

    CameraConfig config;
    CameraConfigResult error;
    if (!readPositiveInteger(settings, QStringLiteral("width"), config.width, error)
        || !readPositiveInteger(settings, QStringLiteral("height"), config.height, error)
        || !readPositiveInteger(settings, QStringLiteral("fps"), config.fps, error)) {
        return error;
    }

    const QString depthValue = settings.value(QStringLiteral("sensor_depth")).toString();
    bool depthOk = false;
    const int depth = depthValue.toInt(&depthOk);
    if (!settings.contains(QStringLiteral("sensor_depth")) || !depthOk
        || (depth != 8 && depth != 10 && depth != 12)) {
        return invalidSetting(QStringLiteral("sensor_depth"),
                              settings.contains(QStringLiteral("sensor_depth"))
                                  ? depthValue : QStringLiteral("<missing>"),
                              QStringLiteral("8, 10, or 12"));
    }
    config.sensorDepth = depth;

    if (!readPositiveInteger(settings, QStringLiteral("exposure_us"),
                             config.exposureMicroseconds, error)) {
        return error;
    }

    const QString gainMode = settings.value(QStringLiteral("gain_mode")).toString();
    if (!settings.contains(QStringLiteral("gain_mode"))) {
        return invalidSetting(QStringLiteral("gain_mode"), QStringLiteral("<missing>"),
                              QStringLiteral("auto or manual"));
    }
    if (gainMode == QStringLiteral("auto")) {
        config.gainMode = CameraGainMode::Auto;
    } else if (gainMode == QStringLiteral("manual")) {
        config.gainMode = CameraGainMode::Manual;
    } else {
        return invalidSetting(QStringLiteral("gain_mode"), gainMode,
                              QStringLiteral("auto or manual"));
    }

    const QString gainValue = settings.value(QStringLiteral("analogue_gain")).toString();
    bool gainOk = false;
    const double gain = gainValue.toDouble(&gainOk);
    if (!settings.contains(QStringLiteral("analogue_gain")) || !gainOk || gain < 1.0) {
        return invalidSetting(QStringLiteral("analogue_gain"),
                              settings.contains(QStringLiteral("analogue_gain"))
                                  ? gainValue : QStringLiteral("<missing>"),
                              QStringLiteral("a number greater than or equal to 1.0"));
    }
    config.analogueGain = gain;

    settings.endGroup();
    if (!readTrackingConfig(settings, config.tracking, error)) {
        return error;
    }

    return {config, {}};
}
