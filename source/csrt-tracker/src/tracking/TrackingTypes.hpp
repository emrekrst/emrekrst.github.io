#pragma once

#include <QMetaType>
#include <QtGlobal>

#include <algorithm>

struct FramePoint
{
    double x{};
    double y{};

    bool operator==(const FramePoint&) const = default;
};

struct FrameRect
{
    double x{};
    double y{};
    double width{};
    double height{};

    bool operator==(const FrameRect&) const = default;
};

struct SelectionResult
{
    FrameRect box;
    double confidence{};
};

enum class TrackingState
{
    Ready,
    Detecting,
    Tracking,
    Lost,
    Error,
};

struct TrackingStats
{
    quint64 completedUpdates{};
    quint64 skippedFrames{};
    double lastUpdateMilliseconds{};
    double lastResultAgeMilliseconds{};
};

inline double intersectionOverUnion(const FrameRect& lhs, const FrameRect& rhs) noexcept
{
    const double left = std::max(lhs.x, rhs.x);
    const double top = std::max(lhs.y, rhs.y);
    const double right = std::min(lhs.x + lhs.width, rhs.x + rhs.width);
    const double bottom = std::min(lhs.y + lhs.height, rhs.y + rhs.height);
    const double intersection = std::max(0.0, right - left) * std::max(0.0, bottom - top);
    const double unionArea = lhs.width * lhs.height + rhs.width * rhs.height - intersection;
    return unionArea > 0.0 ? intersection / unionArea : 0.0;
}

Q_DECLARE_METATYPE(FramePoint)
Q_DECLARE_METATYPE(FrameRect)
Q_DECLARE_METATYPE(TrackingState)
Q_DECLARE_METATYPE(TrackingStats)
