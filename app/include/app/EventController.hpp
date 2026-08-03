#ifndef APP_EVENT_CONTROLLER_HPP
#define APP_EVENT_CONTROLLER_HPP

#include <engine/core/Controller.hpp>
#include <functional>
#include <vector>

namespace app {
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
class EventController final : public engine::core::Controller {
public:
    std::string_view name() const override {
        return "app::EventController";
    }

    /**
     * @brief Schedules `callback` to be invoked once `delay_seconds` have passed.
     */
    void schedule(float delay_seconds, std::function<void()> callback);

private:
    void update() override;

    std::vector<ScheduledEvent> m_events;
};
}// namespace app

#endif//APP_EVENT_CONTROLLER_HPP
