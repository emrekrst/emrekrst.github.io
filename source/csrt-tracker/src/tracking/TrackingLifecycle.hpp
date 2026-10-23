#pragma once

#include <mutex>
#include <optional>
#include <utility>

#include <QtGlobal>

class TrackingLifecycle final
{
public:
    struct UpdateAdmission
    {
        quint64 generation{};
    };

    [[nodiscard]] quint64 beginSelection()
    {
        std::lock_guard lock(mutex_);
        tracking_ = false;
        return ++generation_;
    }

    template <typename OnSkipped>
    [[nodiscard]] std::optional<UpdateAdmission> admitUpdate(OnSkipped&& onSkipped)
    {
        std::lock_guard lock(mutex_);
        if (!tracking_) {
            return std::nullopt;
        }
        if (commandActive_) {
            std::forward<OnSkipped>(onSkipped)();
            return std::nullopt;
        }
        commandActive_ = true;
        return UpdateAdmission{generation_};
    }

    [[nodiscard]] bool tryBeginCommand()
    {
        std::lock_guard lock(mutex_);
        if (commandActive_) {
            return false;
        }
        commandActive_ = true;
        return true;
    }

    template <typename OnCurrent>
    [[nodiscard]] bool completeCommand(
        quint64 generation,
        bool trackingAfterCompletion,
        OnCurrent&& onCurrent)
    {
        std::lock_guard lock(mutex_);
        commandActive_ = false;
        if (generation != generation_) {
            return false;
        }
        std::forward<OnCurrent>(onCurrent)();
        tracking_ = trackingAfterCompletion;
        return true;
    }

    template <typename OnReset>
    void reset(OnReset&& onReset)
    {
        std::lock_guard lock(mutex_);
        ++generation_;
        tracking_ = false;
        std::forward<OnReset>(onReset)();
    }

    [[nodiscard]] bool isCurrent(quint64 generation) const
    {
        std::lock_guard lock(mutex_);
        return generation == generation_;
    }

private:
    mutable std::mutex mutex_;
    quint64 generation_{0};
    bool tracking_{false};
    bool commandActive_{false};
};
