#include <algorithm>
#include <engine/core/EventController.hpp>
#include <engine/platform/PlatformController.hpp>

namespace engine::core {
void EventController::initialize() {
    m_events.reserve(kInitialCapacity);
    m_ready.reserve(kInitialCapacity);
}

void EventController::schedule(float delay_seconds, std::function<void()> callback) {
    m_events.push_back(ScheduledEvent{delay_seconds, std::move(callback)});
}

void EventController::update() {
    const auto platform = Controller::get<engine::platform::PlatformController>();
    const float dt = platform->dt();

    for (auto &event: m_events) {
        event.remaining_seconds -= dt;
    }

    // Fire and remove all the events whose delay has elapsed. New events scheduled from within
    // a callback are appended to m_events and will be processed on a subsequent frame.
    auto fire_from = std::stable_partition(m_events.begin(), m_events.end(), [](const ScheduledEvent &event) {
        return event.remaining_seconds > 0.0f;
    });
    m_ready.clear();
    m_ready.insert(m_ready.end(), std::make_move_iterator(fire_from), std::make_move_iterator(m_events.end()));
    m_events.erase(fire_from, m_events.end());

    for (auto &event: m_ready) {
        event.callback();
    }
}
}// namespace engine::core
