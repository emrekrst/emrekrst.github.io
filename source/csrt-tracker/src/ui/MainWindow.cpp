#include "ui/MainWindow.hpp"

#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QStatusBar>
#include <QVBoxLayout>

#include <utility>

#include "ui/VideoCanvas.hpp"
#include "tracking/CsrtTrackingEngine.hpp"

MainWindow::MainWindow(QWidget* parent)
    : MainWindow(CameraConfigResult{CameraConfig{}, {}}, parent)
{
}

MainWindow::MainWindow(CameraConfigResult cameraConfig, QWidget* parent)
    : QMainWindow(parent)
    , tracker_(std::make_unique<CsrtTrackingEngine>(
          cameraConfig.config ? cameraConfig.config->tracking : CsrtConfig{}))
    , cameraConfig_(std::move(cameraConfig.config))
    , cameraConfigError_(std::move(cameraConfig.error))
{
    buildUi();
    connectUi();

    filePipeline_.setTrackingFrameSink(&tracker_);
    cameraPipeline_.setTrackingFrameSink(&tracker_);
    transportTimer_.setInterval(100);
    transportTimer_.start();
    monotonicTimer_.start();
    telemetryTimer_.setInterval(500);
    telemetryTimer_.start();

    resize(1120, 760);
    setWindowTitle(QStringLiteral("Camera Tracker · V1"));
    if (cameraConfig_) {
        statusBar()->showMessage(QStringLiteral("Ready · Open a video or start the Pi camera"));
    } else {
        statusBar()->showMessage(cameraConfigError_);
    }
}

MainWindow::~MainWindow()
{
    filePipeline_.stop();
    cameraPipeline_.stop();
}

void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(24, 20, 24, 18);
    root->setSpacing(14);

    auto* header = new QHBoxLayout;
    auto* title = new QLabel(QStringLiteral("CAMERA TRACKER"), central);
    title->setObjectName("appTitle");
    auto* version = new QLabel(QStringLiteral("V1 · MAC PROTOTYPE"), central);
    version->setObjectName("versionBadge");
    header->addWidget(title);
    header->addWidget(version);
    header->addStretch();

    openFileButton_ = new QPushButton(QStringLiteral("Open Video…"), central);
    openFileButton_->setObjectName("openFileButton");
    cameraButton_ = new QPushButton(QStringLiteral("Pi Camera"), central);
    cameraButton_->setObjectName("cameraButton");
    const bool cameraAvailable = cameraConfig_.has_value() && CameraPipeline::isAvailable();
    cameraButton_->setEnabled(cameraAvailable);
    if (!cameraConfig_) {
        cameraButton_->setToolTip(cameraConfigError_);
    } else if (!CameraPipeline::isAvailable()) {
        cameraButton_->setToolTip(CameraPipeline::unavailabilityReason());
    }
    header->addWidget(openFileButton_);
    header->addWidget(cameraButton_);
    root->addLayout(header);

    auto* sourceRow = new QHBoxLayout;
    auto* sourceCaption = new QLabel(QStringLiteral("SOURCE"), central);
    sourceCaption->setObjectName("metaCaption");
    sourceLabel_ = new QLabel(QStringLiteral("No video selected"), central);
    sourceLabel_->setObjectName("sourceLabel");
    sourceRow->addWidget(sourceCaption);
    sourceRow->addWidget(sourceLabel_);
    sourceRow->addStretch();
    runtimeTelemetry_ = new QLabel(
        QStringLiteral("CAM -- | VIEW -- | TRACK -- | CSRT -- | AGE -- | SKIP --"), central);
    runtimeTelemetry_->setObjectName("runtimeTelemetry");
    sourceRow->addWidget(runtimeTelemetry_);
    root->addLayout(sourceRow);

    auto* videoFrame = new QFrame(central);
    videoFrame->setObjectName("videoFrame");
    videoFrame->setMinimumSize(640, 360);
    auto* videoLayout = new QVBoxLayout(videoFrame);
    videoLayout->setContentsMargins(1, 1, 1, 1);
    videoCanvas_ = new VideoCanvas(videoFrame);
    videoCanvas_->setObjectName("videoCanvas");
    videoLayout->addWidget(videoCanvas_);
    root->addWidget(videoFrame, 1);

    auto* transport = new QHBoxLayout;
    playPauseButton_ = new QPushButton(QStringLiteral("Pause"), central);
    playPauseButton_->setObjectName("playPauseButton");
    playPauseButton_->setEnabled(false);
    positionLabel_ = new QLabel(QStringLiteral("00:00"), central);
    positionSlider_ = new QSlider(Qt::Horizontal, central);
    positionSlider_->setObjectName("positionSlider");
    positionSlider_->setRange(0, 1000);
    positionSlider_->setEnabled(false);
    durationLabel_ = new QLabel(QStringLiteral("00:00"), central);
    transport->addWidget(playPauseButton_);
    transport->addWidget(positionLabel_);
    transport->addWidget(positionSlider_, 1);
    transport->addWidget(durationLabel_);
    root->addLayout(transport);

    setCentralWidget(central);
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget { background: #0b0f14; color: #dce7ef; }
        QLabel#appTitle { font-size: 20px; font-weight: 700; color: #f4f9fc; }
        QLabel#versionBadge { color: #37e0ff; background: #102832; border: 1px solid #24515e;
            padding: 4px 8px; font-size: 10px; font-weight: 700; }
        QLabel#metaCaption { color: #667783; font-size: 10px; font-weight: 700; }
        QLabel#sourceLabel { color: #a7b7c2; }
        QLabel#runtimeTelemetry { color: #7f919d; font-size: 11px; }
        QFrame#videoFrame { background: #020406; border: 1px solid #26313a; }
        QWidget#videoSurface { background: #020406; }
        QPushButton { background: #18212a; border: 1px solid #33414d;
            padding: 8px 14px; font-weight: 600; }
        QPushButton:disabled { color: #52606a; background: #11171d; border-color: #202a31; }
        QSlider::groove:horizontal { height: 4px; background: #26323c; }
        QSlider::sub-page:horizontal { background: #37e0ff; }
        QSlider::handle:horizontal { width: 14px; margin: -5px 0; background: #e9fbff; }
        QStatusBar { color: #7f919d; border-top: 1px solid #1d2730; }
    )"));
}

void MainWindow::connectUi()
{
    connect(openFileButton_, &QPushButton::clicked, this, &MainWindow::chooseFile);
    connect(cameraButton_, &QPushButton::clicked, this, &MainWindow::startCamera);
    connect(playPauseButton_, &QPushButton::clicked, this, &MainWindow::togglePlayback);
    connect(positionSlider_, &QSlider::sliderPressed, this, [this] { sliderDragging_ = true; });
    connect(positionSlider_, &QSlider::sliderReleased, this, &MainWindow::seekFromSlider);
    connect(videoCanvas_, &VideoCanvas::targetSelected, this, &MainWindow::selectTarget);
    connect(videoCanvas_, &VideoCanvas::displaySizeChanged,
        &filePipeline_, &FilePipeline::setDisplaySize);
    connect(videoCanvas_, &VideoCanvas::displaySizeChanged,
        &cameraPipeline_, &CameraPipeline::setDisplaySize);
    const auto displayFrame = [this](const QImage& frame) {
        ++displayedFrames_;
        videoCanvas_->setFrame(frame);
    };
    connect(&filePipeline_, &FilePipeline::frameReady, this, displayFrame);
    connect(&cameraPipeline_, &CameraPipeline::frameReady, this, displayFrame);
    connect(&filePipeline_, &AbstractPipeline::frameSizeChanged,
        videoCanvas_, &VideoCanvas::setFrameSize);
    connect(&cameraPipeline_, &AbstractPipeline::frameSizeChanged,
        videoCanvas_, &VideoCanvas::setFrameSize);
    connect(&tracker_, &TrackingController::targetBoxChanged,
        videoCanvas_, [this](FrameRect box) {
            videoCanvas_->setTargetBox(box);
        });
    connect(&tracker_, &TrackingController::targetCleared,
        videoCanvas_, [this] {
            videoCanvas_->setTargetBox(std::nullopt);
        });
    connect(&tracker_, &TrackingController::stateChanged,
        videoCanvas_, &VideoCanvas::setTrackingState);
    connect(&tracker_, &TrackingController::stateChanged, this, [this](TrackingState state) {
        switch (state) {
        case TrackingState::Ready:
            statusBar()->showMessage(QStringLiteral("Ready · click the target once"));
            break;
        case TrackingState::Detecting:
            statusBar()->showMessage(QStringLiteral("Detecting clicked object…"));
            break;
        case TrackingState::Tracking:
            statusBar()->showMessage(QStringLiteral("Target locked · CPU CSRT tracking"));
            break;
        case TrackingState::Lost:
            statusBar()->showMessage(QStringLiteral("Target lost · click once to reacquire"));
            break;
        case TrackingState::Error:
            statusBar()->showMessage(QStringLiteral("Tracking error · click once to retry"));
            break;
        }
    });
    connect(&transportTimer_, &QTimer::timeout, this, &MainWindow::updateTransport);
    connect(&telemetryTimer_, &QTimer::timeout, this, &MainWindow::updateRuntimeTelemetry);

    for (AbstractPipeline* pipeline : {static_cast<AbstractPipeline*>(&filePipeline_),
             static_cast<AbstractPipeline*>(&cameraPipeline_)}) {
        connect(pipeline, &AbstractPipeline::errorOccurred, this, &MainWindow::showPipelineError);
        connect(pipeline, &AbstractPipeline::endOfStream, this, [this] {
            paused_ = true;
            playPauseButton_->setText(QStringLiteral("Play"));
            statusBar()->showMessage(QStringLiteral("Playback reached the end"));
        });
    }
}

void MainWindow::chooseFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Open video"),
        {},
        QStringLiteral("Video files (*.mp4 *.mov *.mkv *.avi *.m4v);;All files (*)"));
    if (path.isEmpty()) {
        return;
    }

    openVideoFile(path);
}

bool MainWindow::openVideoFile(const QString& path)
{
    cameraPipeline_.stop();
    tracker_.reset();
    videoCanvas_->clear();
    filePipeline_.setDisplaySize(videoCanvas_->size());
    if (!filePipeline_.open(path)) {
        return false;
    }

    sourceLabel_->setText(QFileInfo(path).fileName());
    setSourceMode(SourceMode::File);
    statusBar()->showMessage(QStringLiteral("Playing · click the target once to start CSRT"));

    return true;
}

const CameraConfig& MainWindow::cameraConfigForTest() const
{
    return cameraConfig_.value();
}

void MainWindow::startCamera()
{
    if (!cameraConfig_) {
        statusBar()->showMessage(cameraConfigError_);
        return;
    }
    filePipeline_.stop();
    tracker_.reset();
    videoCanvas_->clear();
    cameraPipeline_.setDisplaySize(videoCanvas_->size());
    if (cameraPipeline_.start(0, *cameraConfig_)) {
        sourceLabel_->setText(QStringLiteral("Raspberry Pi camera"));
        setSourceMode(SourceMode::Camera);
        statusBar()->showMessage(QStringLiteral("Live camera preview"));
    }
}

void MainWindow::togglePlayback()
{
    if (sourceMode_ != SourceMode::File) {
        return;
    }
    paused_ = !paused_;
    if (filePipeline_.setPaused(paused_)) {
        playPauseButton_->setText(paused_ ? QStringLiteral("Play") : QStringLiteral("Pause"));
    }
}

void MainWindow::seekFromSlider()
{
    if (sourceMode_ == SourceMode::File) {
        tracker_.reset();
        videoCanvas_->setTargetBox(std::nullopt);
        filePipeline_.seek(positionSlider_->value() / 1000.0);
    }
    sliderDragging_ = false;
}

void MainWindow::updateTransport()
{
    if (sourceMode_ != SourceMode::File || !filePipeline_.isLoaded()) {
        return;
    }
    const qint64 position = filePipeline_.positionMilliseconds();
    const qint64 duration = filePipeline_.durationMilliseconds();
    positionLabel_->setText(formatTime(position));
    durationLabel_->setText(formatTime(duration));
    if (!sliderDragging_ && duration > 0) {
        positionSlider_->setValue(static_cast<int>((position * 1000) / duration));
    }
}

void MainWindow::updateRuntimeTelemetry()
{
    AbstractPipeline* pipeline = nullptr;
    if (sourceMode_ == SourceMode::File) {
        pipeline = &filePipeline_;
    } else if (sourceMode_ == SourceMode::Camera) {
        pipeline = &cameraPipeline_;
    }
    if (pipeline == nullptr) {
        return;
    }

    const TrackingStats stats = tracker_.stats();
    const bool trackingActive = tracker_.state() == TrackingState::Tracking;
    const RuntimeRates rates = telemetrySampler_.sample(
        {pipeline->observedFrameCount(),
            displayedFrames_,
            stats.completedUpdates,
            stats.skippedFrames,
            stats.lastUpdateMilliseconds,
            stats.lastResultAgeMilliseconds,
            trackingActive},
        monotonicTimer_.elapsed());
    runtimeTelemetry_->setText(formatRuntimeTelemetry(rates));
}

void MainWindow::selectTarget(FramePoint point)
{
    tracker_.selectTarget(point);
}

void MainWindow::showPipelineError(const QString& message)
{
    setSourceMode(SourceMode::None);
    statusBar()->showMessage(message);
    QMessageBox::warning(this, QStringLiteral("Playback error"), message);
}

void MainWindow::setSourceMode(SourceMode mode)
{
    sourceMode_ = mode;
    displayedFrames_ = 0;
    telemetrySampler_.reset();
    runtimeTelemetry_->setText(
        QStringLiteral("CAM -- | VIEW -- | TRACK -- | CSRT -- | AGE -- | SKIP --"));
    paused_ = false;
    const bool fileMode = mode == SourceMode::File;
    playPauseButton_->setEnabled(fileMode);
    positionSlider_->setEnabled(fileMode);
    playPauseButton_->setText(QStringLiteral("Pause"));
    if (!fileMode) {
        positionSlider_->setValue(0);
        positionLabel_->setText(QStringLiteral("00:00"));
        durationLabel_->setText(QStringLiteral("00:00"));
    }
}

QString MainWindow::formatTime(qint64 milliseconds)
{
    const qint64 seconds = std::max<qint64>(0, milliseconds / 1000);
    return QStringLiteral("%1:%2")
        .arg(seconds / 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}
