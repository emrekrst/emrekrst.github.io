#include "ui/TargetOverlay.hpp"

#include <QMouseEvent>
#include <QPainter>

#include "ui/VideoGeometry.hpp"

TargetOverlay::TargetOverlay(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

void TargetOverlay::setFrameSize(QSize frameSize)
{
    frameSize_ = frameSize;
    update();
}

void TargetOverlay::setTargetBox(std::optional<FrameRect> box)
{
    targetBox_ = box;
    update();
}

QSize TargetOverlay::frameSize() const
{
    return frameSize_;
}

void TargetOverlay::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        const auto point = mapWidgetPointToFrame(event->position(), size(), frameSize_);
        if (point) {
            emit targetSelected(*point);
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void TargetOverlay::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!targetBox_) {
        painter.setPen(QColor(255, 255, 255, 110));
        painter.drawText(rect().adjusted(0, 0, 0, -18), Qt::AlignHCenter | Qt::AlignBottom,
            frameSize_.isValid()
                ? QStringLiteral("CLICK VIDEO TO SELECT A TARGET")
                : QStringLiteral("OPEN A VIDEO TO BEGIN"));
        return;
    }

    const QRectF box = mapFrameRectToWidget(*targetBox_, size(), frameSize_);
    const QColor cyan(55, 224, 255);
    painter.setPen(QPen(cyan, 2.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(box, 3.0, 3.0);

    const QPointF center = box.center();
    painter.drawLine(QPointF(center.x() - 11.0, center.y()), QPointF(center.x() + 11.0, center.y()));
    painter.drawLine(QPointF(center.x(), center.y() - 11.0), QPointF(center.x(), center.y() + 11.0));

    const QString badgeText = QStringLiteral("TARGET LOCKED · CSRT");
    const double badgeWidth = painter.fontMetrics().horizontalAdvance(badgeText) + 20.0;
    const QRectF badge(box.left(), std::max(8.0, box.top() - 27.0), badgeWidth, 22.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(8, 18, 25, 220));
    painter.drawRoundedRect(badge, 4.0, 4.0);
    painter.setPen(cyan);
    painter.drawText(badge, Qt::AlignCenter, badgeText);
}
