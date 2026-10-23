#pragma once

#include <optional>
#include <QString>

#include "tracking/CsrtConfig.hpp"

enum class CameraGainMode { Auto, Manual };

struct CameraConfig
{
    int width{2028};
    int height{1080};
    int fps{50};
    int sensorDepth{12};
    int exposureMicroseconds{5000};
    CameraGainMode gainMode{CameraGainMode::Auto};
    double analogueGain{1.0};
    CsrtConfig tracking;
};

struct CameraConfigResult
{
    std::optional<CameraConfig> config;
    QString error;
};

CameraConfigResult loadCameraConfig(const QString& path);
