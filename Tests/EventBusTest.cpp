#include "minitest.h"
#include "Core/EventBus.h"
#include "Core/Types.h"

TEST(EventBus, SubscribePublish) {
    EventBus<DeviceEvent> bus;
    int count = 0;
    bus.Subscribe([&](const DeviceEvent&) { count++; });
    DeviceEvent ev{ DeviceEvent::Added, {} };
    bus.Publish(ev);
    EXPECT_EQ(count, 1);
}

TEST(EventBus, MultipleSubscribers) {
    EventBus<DeviceEvent> bus;
    int a = 0, b = 0;
    bus.Subscribe([&](const DeviceEvent&) { a++; });
    bus.Subscribe([&](const DeviceEvent&) { b++; });
    bus.Publish({ DeviceEvent::Added, {} });
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 1);
}

TEST(EventBus, Unsubscribe) {
    EventBus<DeviceEvent> bus;
    int count = 0;
    auto tok = bus.Subscribe([&](const DeviceEvent&) { count++; });
    bus.Unsubscribe(tok);
    bus.Publish({ DeviceEvent::Added, {} });
    EXPECT_EQ(count, 0);
}

TEST(EventBus, UnsubscribePartial) {
    EventBus<DeviceEvent> bus;
    int a = 0, b = 0;
    auto tok = bus.Subscribe([&](const DeviceEvent&) { a++; });
    bus.Subscribe([&](const DeviceEvent&) { b++; });
    bus.Unsubscribe(tok);
    bus.Publish({ DeviceEvent::Added, {} });
    EXPECT_EQ(a, 0);
    EXPECT_EQ(b, 1);
}

TEST(EventBus, DeviceEventPublish) {
    DeviceEventBus bus;
    DeviceEvent received{};
    bus.Subscribe([&](const DeviceEvent& e) { received = e; });
    DeviceEvent ev{ DeviceEvent::Added, {} };
    bus.Publish(ev);
    EXPECT_EQ(received.type, DeviceEvent::Added);
}

TEST(EventBus, ErrorEventPublish) {
    ErrorEventBus bus;
    ErrorEvent received{};
    bus.Subscribe([&](const ErrorEvent& e) { received = e; });
    ErrorEvent ev{ ErrorEvent::Input, E_FAIL, L"test" };
    bus.Publish(ev);
    EXPECT_EQ(received.source, ErrorEvent::Input);
    EXPECT_EQ(received.hr, E_FAIL);
}
