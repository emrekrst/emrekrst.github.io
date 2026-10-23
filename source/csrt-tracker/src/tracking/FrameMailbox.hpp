#pragma once

#include <condition_variable>
#include <mutex>

#include <gst/gst.h>

class FrameMailbox
{
public:
    FrameMailbox() = default;
    ~FrameMailbox();

    FrameMailbox(const FrameMailbox&) = delete;
    FrameMailbox& operator=(const FrameMailbox&) = delete;

    void push(GstBuffer* buffer);
    [[nodiscard]] GstBuffer* waitAndTake();
    void stop();

private:
    std::mutex mutex_;
    std::condition_variable ready_;
    GstBuffer* buffer_{nullptr};
    bool stopped_{false};
};
