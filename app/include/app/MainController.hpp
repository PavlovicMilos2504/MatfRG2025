#ifndef APP_MAIN_CONTROLLER_HPP
#define APP_MAIN_CONTROLLER_HPP

#include <app/SceneLighting.hpp>
#include <engine/core/Controller.hpp>
#include <engine/platform/PlatformEventObserver.hpp>

namespace app {
/**
 * @brief Reacts to keyboard input and forwards the corresponding events to @ref MainController.
 */
class MainPlatformEventObserver final : public engine::platform::PlatformEventObserver {
public:
    void on_key(engine::platform::Key key) override;
};

/**
 * @brief Drives the billiard room scene: camera movement, lighting and the lamp switch action/event chain.
 */
class MainController final : public engine::core::Controller {
public:
    std::string_view name() const override {
        return "app::MainController";
    }

    /**
     * @brief Starts the lamp switch action/event chain:
     * ACTION (key press) --- after 1s ---> EVENT_A (lamp turns on)
     * --- after 2s ---> EVENT_B (spotlight glow above the table stabilizes).
     */
    void trigger_lamp_switch();

    SceneLighting &lighting() {
        return m_lighting;
    }

private:
    void initialize() override;

    bool loop() override;

    void poll_events() override;

    void update() override;

    void begin_draw() override;

    void draw() override;

    void end_draw() override;

    void draw_table();

    void update_camera();

    SceneLighting m_lighting{};
    bool m_lamp_switch_in_progress{false};
    bool m_cursor_enabled{true};
};
}// namespace app

#endif//APP_MAIN_CONTROLLER_HPP
