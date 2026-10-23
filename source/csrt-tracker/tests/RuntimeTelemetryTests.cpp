#include <QtTest>

#include "metrics/RuntimeTelemetry.hpp"

class RuntimeTelemetryTests final : public QObject
{
    Q_OBJECT

private slots:
    void calculatesRatesFromCounterDeltas();
    void formatsInactiveTrackingFieldsAsUnavailable();
    void clampsCounterResetDeltasToZero();
    void resetRebaselinesForANewSource();
};

void RuntimeTelemetryTests::calculatesRatesFromCounterDeltas()
{
    RuntimeTelemetrySampler sampler;
    static_cast<void>(sampler.sample({100, 50, 10, 2, 0.0, 0.0, false}, 1000));

    const RuntimeRates rates = sampler.sample({125, 65, 12, 10, 238.0, 251.0, true}, 1500);

    QCOMPARE(rates.cameraFps, 50.0);
    QCOMPARE(rates.viewFps, 30.0);
    QCOMPARE(rates.trackingFps, 4.0);
    QCOMPARE(rates.skipPercent, 80.0);
    QCOMPARE(formatRuntimeTelemetry(rates),
        QString("CAM 50.0 | VIEW 30.0 | TRACK 4.0 | CSRT 238 ms | AGE 251 ms | SKIP 80%"));
}

void RuntimeTelemetryTests::formatsInactiveTrackingFieldsAsUnavailable()
{
    RuntimeTelemetrySampler sampler;
    static_cast<void>(sampler.sample({0, 0, 0, 0, 0.0, 0.0, false}, 1000));

    const RuntimeRates rates = sampler.sample({25, 15, 2, 8, 238.0, 251.0, false}, 1500);

    QCOMPARE(formatRuntimeTelemetry(rates),
        QString("CAM 50.0 | VIEW 30.0 | TRACK -- | CSRT -- | AGE -- | SKIP --"));
}

void RuntimeTelemetryTests::clampsCounterResetDeltasToZero()
{
    RuntimeTelemetrySampler sampler;
    static_cast<void>(sampler.sample({100, 100, 100, 100, 1.0, 1.0, true}, 1000));

    const RuntimeRates rates = sampler.sample({2, 3, 4, 5, 2.0, 3.0, true}, 1500);

    QCOMPARE(rates.cameraFps, 0.0);
    QCOMPARE(rates.viewFps, 0.0);
    QCOMPARE(rates.trackingFps, 0.0);
    QCOMPARE(rates.skipPercent, 0.0);
}

void RuntimeTelemetryTests::resetRebaselinesForANewSource()
{
    RuntimeTelemetrySampler sampler;
    static_cast<void>(sampler.sample({900, 800, 70, 30, 200.0, 240.0, true}, 1000));

    sampler.reset();
    const RuntimeRates newBaseline =
        sampler.sample({10, 4, 1, 0, 7.0, 9.0, true}, 1500);
    QCOMPARE(newBaseline.cameraFps, 0.0);
    QCOMPARE(newBaseline.viewFps, 0.0);
    QVERIFY(!newBaseline.trackingActive);

    const RuntimeRates newSourceRates =
        sampler.sample({15, 7, 2, 1, 8.0, 10.0, true}, 2000);
    QCOMPARE(newSourceRates.cameraFps, 10.0);
    QCOMPARE(newSourceRates.viewFps, 6.0);
    QCOMPARE(newSourceRates.trackingFps, 2.0);
    QCOMPARE(newSourceRates.skipPercent, 50.0);
}

QTEST_MAIN(RuntimeTelemetryTests)
#include "RuntimeTelemetryTests.moc"
