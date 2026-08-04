#include <app/GUIController.hpp>
#include <app/MainController.hpp>
#include <engine/core/Engine.hpp>
#include <engine/graphics/GraphicsController.hpp>
#include <imgui.h>

namespace app {
void GUIController::initialize() {
    set_enable(false);
}

void GUIController::poll_events() {
    const auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    if (platform->key(engine::platform::KEY_F2).state() == engine::platform::Key::State::JustPressed) {
        set_enable(!is_enabled());
    }
}

void GUIController::draw() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto &lighting = engine::core::Controller::get<MainController>()->lighting();

    graphics->begin_gui();
    ImGui::Begin("Osvetljenje");

    ImGui::SeparatorText("Direkciono svetlo (mesecina)");
    ImGui::Checkbox("Ukljuceno##dir", &lighting.directional.enabled);
    ImGui::ColorEdit3("Diffuse##dir", &lighting.directional.diffuse.x);
    ImGui::SliderFloat3("Pravac##dir", &lighting.directional.direction.x, -1.0f, 1.0f);

    ImGui::SeparatorText("Lampa iznad stola (tackasto svetlo)");
    ImGui::Checkbox("Ukljucena##lamp", &lighting.lamp.enabled);
    ImGui::ColorEdit3("Diffuse##lamp", &lighting.lamp.diffuse.x);
    ImGui::SliderFloat3("Pozicija##lamp", &lighting.lamp.position.x, -5.0f, 5.0f);
    ImGui::SliderFloat("Slabljenje (linear)##lamp", &lighting.lamp.linear, 0.0f, 0.5f);
    ImGui::SliderFloat("Slabljenje (quadratic)##lamp", &lighting.lamp.quadratic, 0.0f, 1.0f);

    ImGui::SeparatorText("Model stola");
    ImGui::SliderFloat("Skala", &engine::core::Controller::get<MainController>()->table_scale(), 0.1f, 3.0f);

    ImGui::End();
    graphics->end_gui();
}
}// namespace app
