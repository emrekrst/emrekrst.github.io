#include "ui/VideoCanvas.hpp"

#include <algorithm>

#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>

#include "ui/VideoGeometry.hpp"

VideoCanvas::VideoCanvas(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void VideoCanvas::setFrame(const QImage& frame)
{
    frame_ = frame;
    if (!frameSize_.isValid()) {
        frameSize_ = frame.size();
    }
    update();
}

void VideoCanvas::setFrameSize(QSize frameSize)
{
    frameSize_ = frameSize;
    update();
}

QSize VideoCanvas::frameSize() const
{
    return frameSize_;
}

void VideoCanvas::setTargetBox(std::optional<FrameRect> box)
{
    targetBox_ = box;
    update();
}

void VideoCanvas::setTrackingState(TrackingState state)
{
    trackingState_ = state;
    if (state == TrackingState::Tracking || state == TrackingState::Lost
        || state == TrackingState::Ready || state == TrackingState::Error) {
        pendingPoint_.reset();
    }
    update();
}

void VideoCanvas::clear()
{
    frame_ = QImage();
    frameSize_ = QSize();
    targetBox_.reset();
    pendingPoint_.reset();
    trackingState_ = TrackingState::Ready;
    update();
}

void VideoCanvas::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    emit displaySizeChanged(event->size());
}

void VideoCanvas::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(2, 4, 6));

    if (frame_.isNull() || !frameSize_.isValid()) {
        painter.setPen(QColor(255, 255, 255, 110));
        painter.drawText(rect(), Qt::AlignCenter,
            QStringLiteral("OPEN A VIDEO TO BEGIN"));
        return;
    }

    const QRectF target = videoDisplayRect(size(), frameSize_);
    painter.drawImage(target, frame_);

    if (!targetBox_) {
        painter.setPen(QColor(255, 255, 255, 110));
        const QString prompt = trackingState_ == TrackingState::Detecting
            ? QStringLiteral("DETECTING TARGET…")
            : trackingState_ == TrackingState::Lost
                ? QStringLiteral("TARGET LOST · CLICK TO REACQUIRE")
                : QStringLiteral("CLICK VIDEO TO SELECT A TARGET");
        painter.drawText(rect().adjusted(0, 0, 0, -18),
            Qt::AlignHCenter | Qt::AlignBottom,
            prompt);
        if (pendingPoint_) {
            const FrameRect marker{pendingPoint_->x - 5.0, pendingPoint_->y - 5.0, 10.0, 10.0};
            const QRectF mapped = mapFrameRectToWidget(marker, size(), frameSize_);
            painter.setPen(QPen(QColor(255, 196, 66), 2.0));
            painter.drawEllipse(mapped);
        }
        return;
    }

    painter.setRenderHint(QPainter::Antialiasing);
    const QRectF box = mapFrameRectToWidget(*targetBox_, size(), frameSize_);
    const QColor cyan(55, 224, 255);
    painter.setPen(QPen(cyan, 2.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(box);

    const QPointF center = box.center();
    painter.drawLine(QPointF(center.x() - 11.0, center.y()),
        QPointF(center.x() + 11.0, center.y()));
    painter.drawLine(QPointF(center.x(), center.y() - 11.0),
        QPointF(center.x(), center.y() + 11.0));

    const QString badgeText = QStringLiteral("TARGET LOCKED · CSRT");
    const double badgeWidth =
        painter.fontMetrics().horizontalAdvance(badgeText) + 20.0;
    const QRectF badge(box.left(), std::max(8.0, box.top() - 27.0), badgeWidth, 22.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(8, 18, 25, 220));
    painter.drawRect(badge);
    painter.setPen(cyan);
    painter.drawText(badge, Qt::AlignCenter, badgeText);
}

void VideoCanvas::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        const auto point = mapWidgetPointToFrame(event->position(), size(), frameSize_);
        if (point) {
            pendingPoint_ = *point;
            trackingState_ = TrackingState::Detecting;
            targetBox_.reset();
            update();
            emit targetSelected(*point);
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}
