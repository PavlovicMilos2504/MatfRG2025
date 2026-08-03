#ifndef APP_GUI_CONTROLLER_HPP
#define APP_GUI_CONTROLLER_HPP

#include <engine/core/Controller.hpp>

namespace app {
/**
 * @brief Draws an ImGui panel used to tweak the scene lighting at runtime (toggle F2).
 */
class GUIController final : public engine::core::Controller {
public:
    std::string_view name() const override {
        return "app::GUIController";
    }

private:
    void initialize() override;

    void poll_events() override;

    void draw() override;
};
}// namespace app

#endif//APP_GUI_CONTROLLER_HPP
