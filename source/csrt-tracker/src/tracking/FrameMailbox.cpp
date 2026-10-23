#include "tracking/FrameMailbox.hpp"

FrameMailbox::~FrameMailbox()
{
    stop();
}

void FrameMailbox::push(GstBuffer* buffer)
{
    if (buffer == nullptr) {
        return;
    }

    GstBuffer* replacement = gst_buffer_ref(buffer);
    {
        std::lock_guard lock(mutex_);
        if (stopped_) {
            gst_buffer_unref(replacement);
            return;
        }
        if (buffer_ != nullptr) {
            gst_buffer_unref(buffer_);
        }
        buffer_ = replacement;
    }
    ready_.notify_one();
}

GstBuffer* FrameMailbox::waitAndTake()
{
    std::unique_lock lock(mutex_);
    ready_.wait(lock, [this] { return stopped_ || buffer_ != nullptr; });
    if (buffer_ == nullptr) {
        return nullptr;
    }

    GstBuffer* result = buffer_;
    buffer_ = nullptr;
    return result;
}

void FrameMailbox::stop()
{
    {
        std::lock_guard lock(mutex_);
        stopped_ = true;
        if (buffer_ != nullptr) {
            gst_buffer_unref(buffer_);
            buffer_ = nullptr;
        }
    }
    ready_.notify_all();
}
