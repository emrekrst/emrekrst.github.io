#include <QtTest>

#include <gst/video/video.h>

#include "tracking/TrackingFrame.hpp"

class TrackingFrameTests final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void snapshotSurvivesProducerUnrefAndOwnsBgrPixels();
};

void TrackingFrameTests::initTestCase()
{
    gst_init(nullptr, nullptr);
}

void TrackingFrameTests::snapshotSurvivesProducerUnrefAndOwnsBgrPixels()
{
    GstVideoInfo info;
    gst_video_info_set_format(&info, GST_VIDEO_FORMAT_BGRx, 2, 1);
    GstBuffer* buffer = gst_buffer_new_allocate(nullptr, info.size, nullptr);
    QVERIFY(buffer != nullptr);

    GstMapInfo bytes;
    QVERIFY(gst_buffer_map(buffer, &bytes, GST_MAP_WRITE));
    const guint8 pixels[8] = {10, 20, 30, 0, 40, 50, 60, 0};
    std::copy(std::begin(pixels), std::end(pixels), bytes.data);
    gst_buffer_unmap(buffer, &bytes);

    TrackingFramePtr frame = TrackingFrame::capture(buffer, info);
    gst_buffer_unref(buffer);

    QVERIFY(frame != nullptr);
    QCOMPARE(frame->size(), QSize(2, 1));
    const auto mapped = frame->mapBgr();
    QVERIFY(mapped.has_value());
    QCOMPARE(mapped->mat.at<cv::Vec3b>(0, 0), cv::Vec3b(10, 20, 30));
    QCOMPARE(mapped->mat.at<cv::Vec3b>(0, 1), cv::Vec3b(40, 50, 60));
}

QTEST_MAIN(TrackingFrameTests)
#include "TrackingFrameTests.moc"
