#pragma once

#include <QtGlobal>

class QString;

struct RuntimeTotals
{
    quint64 cameraFrames{};
    quint64 displayedFrames{};
    quint64 completedUpdates{};
    quint64 skippedFrames{};
    double lastUpdateMilliseconds{};
    double lastResultAgeMilliseconds{};
    bool trackingActive{};
};

struct RuntimeRates
{
    double cameraFps{};
    double viewFps{};
    double trackingFps{};
    double skipPercent{};
    double lastUpdateMilliseconds{};
    double lastResultAgeMilliseconds{};
    bool trackingActive{};
};

class RuntimeTelemetrySampler final
{
public:
    [[nodiscard]] RuntimeRates sample(const RuntimeTotals& totals, qint64 monotonicMilliseconds);
    void reset() noexcept;

private:
    RuntimeTotals previousTotals_;
    qint64 previousMilliseconds_{};
    bool hasBaseline_{false};
};

[[nodiscard]] QString formatRuntimeTelemetry(const RuntimeRates& rates);
