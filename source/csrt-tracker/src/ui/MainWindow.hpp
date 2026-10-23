#pragma once

#include <QMainWindow>
#include <QElapsedTimer>
#include <QTimer>

#include "config/CameraConfig.hpp"
#include "metrics/RuntimeTelemetry.hpp"
#include "pipeline/CameraPipeline.hpp"
#include "pipeline/FilePipeline.hpp"
#include "tracking/TrackingController.hpp"

class QLabel;
class QPushButton;
class QSlider;
class VideoCanvas;
class QWidget;
class MainWindowTests;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    explicit MainWindow(CameraConfigResult cameraConfig, QWidget* parent = nullptr);
    ~MainWindow() override;

    // Opens a video without going through the file dialog. Used by the
    // --open command-line flag for automated/repeatable testing.
    bool openVideoFile(const QString& path);
    void selectTarget(FramePoint point);
    [[nodiscard]] const CameraConfig& cameraConfigForTest() const;

private slots:
    void chooseFile();
    void startCamera();
    void togglePlayback();
    void seekFromSlider();
    void updateTransport();
    void updateRuntimeTelemetry();
    void showPipelineError(const QString& message);

private:
    friend class MainWindowTests;

    enum class SourceMode { None, File, Camera };

    void buildUi();
    void connectUi();
    void setSourceMode(SourceMode mode);
    static QString formatTime(qint64 milliseconds);

    TrackingController tracker_;
    FilePipeline filePipeline_;
    CameraPipeline cameraPipeline_;
    std::optional<CameraConfig> cameraConfig_;
    QString cameraConfigError_;

    VideoCanvas* videoCanvas_{nullptr};
    QPushButton* openFileButton_{nullptr};
    QPushButton* cameraButton_{nullptr};
    QPushButton* playPauseButton_{nullptr};
    QSlider* positionSlider_{nullptr};
    QLabel* positionLabel_{nullptr};
    QLabel* durationLabel_{nullptr};
    QLabel* sourceLabel_{nullptr};
    QLabel* runtimeTelemetry_{nullptr};
    QTimer transportTimer_;
    QTimer telemetryTimer_;
    QElapsedTimer monotonicTimer_;
    RuntimeTelemetrySampler telemetrySampler_;
    quint64 displayedFrames_{0};
    SourceMode sourceMode_{SourceMode::None};
    bool paused_{false};
    bool sliderDragging_{false};
};
