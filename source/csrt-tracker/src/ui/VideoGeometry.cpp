#include "ui/VideoGeometry.hpp"

#include <algorithm>

namespace {
struct DisplayGeometry
{
    double scale{};
    double offsetX{};
    double offsetY{};
    double width{};
    double height{};
};

std::optional<DisplayGeometry> displayGeometry(QSizeF widgetSize, QSize frameSize)
{
    if (widgetSize.width() <= 0.0 || widgetSize.height() <= 0.0
        || frameSize.width() <= 0 || frameSize.height() <= 0) {
        return std::nullopt;
    }

    const double scale = std::min(
        widgetSize.width() / frameSize.width(),
        widgetSize.height() / frameSize.height());
    const double width = frameSize.width() * scale;
    const double height = frameSize.height() * scale;

    return DisplayGeometry{
        scale,
        (widgetSize.width() - width) / 2.0,
        (widgetSize.height() - height) / 2.0,
        width,
        height,
    };
}
} // namespace

QRectF videoDisplayRect(QSizeF widgetSize, QSize frameSize)
{
    const auto display = displayGeometry(widgetSize, frameSize);
    if (!display) {
        return {};
    }
    return {display->offsetX, display->offsetY, display->width, display->height};
}

std::optional<FramePoint> mapWidgetPointToFrame(
    QPointF click,
    QSizeF widgetSize,
    QSize frameSize)
{
    const auto display = displayGeometry(widgetSize, frameSize);
    if (!display) {
        return std::nullopt;
    }

    const double localX = click.x() - display->offsetX;
    const double localY = click.y() - display->offsetY;
    if (localX < 0.0 || localY < 0.0
        || localX > display->width || localY > display->height) {
        return std::nullopt;
    }

    return FramePoint{
        std::clamp(localX / display->scale, 0.0, static_cast<double>(frameSize.width())),
        std::clamp(localY / display->scale, 0.0, static_cast<double>(frameSize.height())),
    };
}

QRectF mapFrameRectToWidget(
    FrameRect frameRect,
    QSizeF widgetSize,
    QSize frameSize)
{
    const auto display = displayGeometry(widgetSize, frameSize);
    if (!display) {
        return {};
    }

    return {
        display->offsetX + frameRect.x * display->scale,
        display->offsetY + frameRect.y * display->scale,
        frameRect.width * display->scale,
        frameRect.height * display->scale,
    };
}
