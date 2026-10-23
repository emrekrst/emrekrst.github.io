#include <QtTest>

#include <gst/gst.h>

#include "tracking/FrameMailbox.hpp"

class FrameMailboxTests final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void newFrameOverwritesUnconsumedFrame();
    void stoppedMailboxReturnsNoFrame();
};

void FrameMailboxTests::initTestCase()
{
    gst_init(nullptr, nullptr);
}

void FrameMailboxTests::newFrameOverwritesUnconsumedFrame()
{
    FrameMailbox mailbox;
    GstBuffer* first = gst_buffer_new_allocate(nullptr, 4, nullptr);
    GstBuffer* second = gst_buffer_new_allocate(nullptr, 8, nullptr);

    mailbox.push(first);
    mailbox.push(second);
    gst_buffer_unref(first);
    gst_buffer_unref(second);

    GstBuffer* taken = mailbox.waitAndTake();
    QVERIFY(taken != nullptr);
    QCOMPARE(gst_buffer_get_size(taken), static_cast<gsize>(8));
    gst_buffer_unref(taken);
}

void FrameMailboxTests::stoppedMailboxReturnsNoFrame()
{
    FrameMailbox mailbox;
    mailbox.stop();

    QCOMPARE(mailbox.waitAndTake(), nullptr);
}

QTEST_MAIN(FrameMailboxTests)
#include "FrameMailboxTests.moc"
