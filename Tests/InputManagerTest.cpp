#include "minitest.h"
#include "Core/InputManager.h"

TEST(InputManager, InitialState) {
    InputManager im;
    EXPECT_EQ(im.GetState(0).dx, 0);
    EXPECT_EQ(im.GetState(0).dy, 0);
    EXPECT_FALSE(im.GetState(0).absolute);
}

TEST(InputManager, LookupIndexReturnsNewIndex) {
    InputManager im;
    HANDLE h1 = (HANDLE)(UINT_PTR)0x1001;
    UINT idx = im.LookupIndex(h1);
    EXPECT_LT(idx, kMaxDevices);
    EXPECT_EQ(im.GetState(idx).dx, 0);
}

TEST(InputManager, LookupIndexReturnsSameForSameDevice) {
    InputManager im;
    HANDLE h = (HANDLE)(UINT_PTR)0x2001;
    UINT i1 = im.LookupIndex(h);
    UINT i2 = im.LookupIndex(h);
    EXPECT_EQ(i1, i2);
}

TEST(InputManager, ProcessInputSetsDelta) {
    InputManager im;
    HANDLE h = (HANDLE)(UINT_PTR)0x3001;
    UINT idx = im.LookupIndex(h);

    RAWINPUT raw = {};
    raw.header.dwType = RIM_TYPEMOUSE;
    raw.header.hDevice = h;
    raw.data.mouse.lLastX = 10;
    raw.data.mouse.lLastY = -5;

    im.ProcessInput(h, raw);
    EXPECT_EQ(im.GetState(idx).dx, 10);
    EXPECT_EQ(im.GetState(idx).dy, -5);
}

TEST(InputManager, RingBufferPublishConsume) {
    InputManager im;
    HANDLE h = (HANDLE)(UINT_PTR)0x4001;
    im.LookupIndex(h);

    RAWINPUT raw = {};
    raw.header.dwType = RIM_TYPEMOUSE;
    raw.header.hDevice = h;
    raw.data.mouse.lLastX = 7;
    raw.data.mouse.lLastY = 3;
    im.ProcessInput(h, raw);

    im.PublishToRingBuffer();

    RawInputFrame frame;
    EXPECT_TRUE(im.TryConsume(frame));
    EXPECT_EQ(frame.hDevice, h);
    EXPECT_EQ(frame.state.dx, 7);
    EXPECT_EQ(frame.state.dy, 3);
}

TEST(InputManager, RingBufferEmptyWhenNoData) {
    InputManager im;
    RawInputFrame frame;
    EXPECT_FALSE(im.TryConsume(frame));
}

TEST(InputManager, RingBufferFlushClearsState) {
    InputManager im;
    HANDLE h = (HANDLE)(UINT_PTR)0x5001;
    UINT idx = im.LookupIndex(h);

    RAWINPUT raw = {};
    raw.header.dwType = RIM_TYPEMOUSE;
    raw.header.hDevice = h;
    raw.data.mouse.lLastX = 5;
    im.ProcessInput(h, raw);

    im.Flush();
    EXPECT_EQ(im.GetState(idx).dx, 0);
    EXPECT_EQ(im.GetState(idx).dy, 0);
}

TEST(InputManager, ResetDevice) {
    InputManager im;
    HANDLE h = (HANDLE)(UINT_PTR)0x6001;
    UINT idx = im.LookupIndex(h);
    EXPECT_EQ(im.LookupIndex(h), idx);

    im.ResetDevice(h);
    // After reset, a new LookupIndex may return the same slot
    UINT newIdx = im.LookupIndex(h);
    EXPECT_TRUE(newIdx == idx || newIdx < kMaxDevices);
    EXPECT_EQ(im.GetState(newIdx).dx, 0);
}

TEST(InputManager, MultipleDevices) {
    InputManager im;
    HANDLE h1 = (HANDLE)(UINT_PTR)0x7001;
    HANDLE h2 = (HANDLE)(UINT_PTR)0x7002;

    UINT i1 = im.LookupIndex(h1);
    UINT i2 = im.LookupIndex(h2);
    EXPECT_NE(i1, i2);

    RAWINPUT r1 = {}, r2 = {};
    r1.header.dwType = RIM_TYPEMOUSE; r1.header.hDevice = h1; r1.data.mouse.lLastX = 1;
    r2.header.dwType = RIM_TYPEMOUSE; r2.header.hDevice = h2; r2.data.mouse.lLastX = 2;

    im.ProcessInput(h1, r1);
    im.ProcessInput(h2, r2);

    EXPECT_EQ(im.GetState(i1).dx, 1);
    EXPECT_EQ(im.GetState(i2).dx, 2);
}

TEST(InputManager, RingBufferNoOverflow) {
    InputManager im;
    HANDLE h = (HANDLE)(UINT_PTR)0x8001;
    im.LookupIndex(h);

    // Fill the ring buffer
    for (int i = 0; i < kRingBufferSize * 2; i++) {
        RAWINPUT raw = {};
        raw.header.dwType = RIM_TYPEMOUSE;
        raw.header.hDevice = h;
        raw.data.mouse.lLastX = i;
        im.ProcessInput(h, raw);
        im.PublishToRingBuffer();

        // Consume to prevent overflow
        RawInputFrame frame;
        while (im.TryConsume(frame)) {}
    }

    // Verify no crash and state is recent
    EXPECT_EQ(im.GetState(im.LookupIndex(h)).dx, 0); // After flush
}
