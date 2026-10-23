#pragma once

#include <optional>

#include <QImage>
#include <QSize>
#include <QWidget>

#include "tracking/TrackingTypes.hpp"

// Paints the video frame and the tracking overlay in a single widget.
//
// Deliberately NOT a native video surface with a separate overlay widget
// stacked on top: on macOS, Qt promotes such an overlay to its own native
// NSView and composites it above the video surface, hiding the video
// entirely. Painting frame and box together removes that whole class of
// problem, and keeps the result reachable via QWidget::grab() so playback
// can be verified without a display.
class VideoCanvas final : public QWidget
{
    Q_OBJECT

public:
    explicit VideoCanvas(QWidget* parent = nullptr);

    // The already-display-scaled frame to paint.
    void setFrame(const QImage& frame);

    // Full-resolution source dimensions. Target coordinates are expressed
    // in this space, independent of how the frame was scaled for display.
    void setFrameSize(QSize frameSize);
    [[nodiscard]] QSize frameSize() const;

    void setTargetBox(std::optional<FrameRect> box);
    void setTrackingState(TrackingState state);
    void clear();

signals:
    void targetSelected(FramePoint point);
    void displaySizeChanged(QSize size);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QImage frame_;
    QSize frameSize_;
    std::optional<FrameRect> targetBox_;
    std::optional<FramePoint> pendingPoint_;
    TrackingState trackingState_{TrackingState::Ready};
};
