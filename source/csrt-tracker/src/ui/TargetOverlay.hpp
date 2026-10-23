#pragma once

#include <optional>

#include <QSize>
#include <QWidget>

#include "tracking/TrackingTypes.hpp"

class TargetOverlay final : public QWidget
{
    Q_OBJECT

public:
    explicit TargetOverlay(QWidget* parent = nullptr);

    void setFrameSize(QSize frameSize);
    void setTargetBox(std::optional<FrameRect> box);
    [[nodiscard]] QSize frameSize() const;

signals:
    void targetSelected(FramePoint point);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    QSize frameSize_;
    std::optional<FrameRect> targetBox_;
};
