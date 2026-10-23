#include "metrics/RuntimeTelemetry.hpp"

#include <QString>

#include <algorithm>

namespace {
quint64 nonDecreasingDelta(quint64 current, quint64 previous) noexcept
{
    return current >= previous ? current - previous : 0;
}

double ratePerSecond(quint64 count, qint64 milliseconds) noexcept
{
    return milliseconds > 0 ? static_cast<double>(count) * 1000.0 / milliseconds : 0.0;
}

QString formattedMilliseconds(double milliseconds)
{
    return QString::number(qRound64(std::max(0.0, milliseconds)));
}
} // namespace

RuntimeRates RuntimeTelemetrySampler::sample(
    const RuntimeTotals& totals,
    qint64 monotonicMilliseconds)
{
    if (!hasBaseline_) {
        previousTotals_ = totals;
        previousMilliseconds_ = monotonicMilliseconds;
        hasBaseline_ = true;
        return {};
    }

    const qint64 elapsedMilliseconds = monotonicMilliseconds - previousMilliseconds_;
    if (elapsedMilliseconds <= 0) {
        return {};
    }

    const quint64 cameraFrames = nonDecreasingDelta(totals.cameraFrames, previousTotals_.cameraFrames);
    const quint64 displayedFrames = nonDecreasingDelta(
        totals.displayedFrames, previousTotals_.displayedFrames);
    const quint64 completedUpdates = nonDecreasingDelta(
        totals.completedUpdates, previousTotals_.completedUpdates);
    const quint64 skippedFrames = nonDecreasingDelta(
        totals.skippedFrames, previousTotals_.skippedFrames);

    previousTotals_ = totals;
    previousMilliseconds_ = monotonicMilliseconds;

    RuntimeRates rates;
    rates.cameraFps = ratePerSecond(cameraFrames, elapsedMilliseconds);
    rates.viewFps = ratePerSecond(displayedFrames, elapsedMilliseconds);
    rates.trackingFps = ratePerSecond(completedUpdates, elapsedMilliseconds);
    const quint64 trackingFrames = completedUpdates + skippedFrames;
    rates.skipPercent = trackingFrames == 0
        ? 0.0
        : static_cast<double>(skippedFrames) * 100.0 / trackingFrames;
    rates.lastUpdateMilliseconds = totals.lastUpdateMilliseconds;
    rates.lastResultAgeMilliseconds = totals.lastResultAgeMilliseconds;
    rates.trackingActive = totals.trackingActive;
    return rates;
}

void RuntimeTelemetrySampler::reset() noexcept
{
    previousTotals_ = {};
    previousMilliseconds_ = 0;
    hasBaseline_ = false;
}

QString formatRuntimeTelemetry(const RuntimeRates& rates)
{
    const QString camera = QString::number(rates.cameraFps, 'f', 1);
    const QString view = QString::number(rates.viewFps, 'f', 1);
    if (!rates.trackingActive) {
        return QStringLiteral("CAM %1 | VIEW %2 | TRACK -- | CSRT -- | AGE -- | SKIP --")
            .arg(camera, view);
    }

    return QStringLiteral("CAM %1 | VIEW %2 | TRACK %3 | CSRT %4 ms | AGE %5 ms | SKIP %6%")
        .arg(camera)
        .arg(view)
        .arg(QString::number(rates.trackingFps, 'f', 1))
        .arg(formattedMilliseconds(rates.lastUpdateMilliseconds))
        .arg(formattedMilliseconds(rates.lastResultAgeMilliseconds))
        .arg(QString::number(qRound64(std::clamp(rates.skipPercent, 0.0, 100.0))));
}
