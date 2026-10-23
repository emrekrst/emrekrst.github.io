#pragma once

#include <optional>

#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>

#include "tracking/TrackingTypes.hpp"

// The letterboxed rectangle the video occupies inside the widget. Painting
// and hit-testing must agree on this, so both go through here.
QRectF videoDisplayRect(QSizeF widgetSize, QSize frameSize);

std::optional<FramePoint> mapWidgetPointToFrame(
    QPointF click,
    QSizeF widgetSize,
    QSize frameSize);

QRectF mapFrameRectToWidget(
    FrameRect frameRect,
    QSizeF widgetSize,
    QSize frameSize);
