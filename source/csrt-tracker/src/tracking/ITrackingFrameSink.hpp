#pragma once

#include <gst/gst.h>
#include <gst/video/video.h>

class ITrackingFrameSink
{
public:
    virtual ~ITrackingFrameSink() = default;
    virtual void offerFrame(GstBuffer* buffer, const GstVideoInfo& info) noexcept = 0;
};
