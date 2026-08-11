#ifndef MATF_RG_PROJECT_EVENT_CONTROLLER_HPP
#define MATF_RG_PROJECT_EVENT_CONTROLLER_HPP

#include <engine/core/Controller.hpp>
#include <functional>
#include <vector>

namespace engine::core {
/**
 * @brief Represents a single scheduled event that fires a callback once a delay has elapsed.
 *
 * Events can be chained: the callback of an event is free to schedule another @ref ScheduledEvent,
 * which allows building sequences such as:
 * `{ACTION} --- after M seconds ---> {EVENT_A} --- after N seconds ---> {EVENT_B}`.
 */
struct ScheduledEvent {
    float remaining_seconds;
    std::function<void()> callback;
};

/**
 * @brief Manages a queue of delayed, one-shot events used to implement action/event chains on the scene
 * (for example: switching on the lamp above the table triggers, after a delay, the balls' glow effect).
 */
class EventController final : public Controller {
public:
    std::string_view name() const override {
        return "engine::core::EventController";
    }

    /**
     * @brief Schedules `callback` to be invoked once `delay_seconds` have passed.
     */
    void schedule(float delay_seconds, std::function<void()> callback);

private:
    void initialize() override;

    void update() override;

    // Reserve capacity upfront for both vectors so the common case (a handful of active events)
    // never triggers a heap allocation from within update(), which runs every frame.
    static constexpr size_t kInitialCapacity = 128;

    std::vector<ScheduledEvent> m_events;
    // Reused across frames (cleared, not reallocated) to hold the events firing this frame.
    std::vector<ScheduledEvent> m_ready;
};
}// namespace engine::core

#endif//MATF_RG_PROJECT_EVENT_CONTROLLER_HPP
