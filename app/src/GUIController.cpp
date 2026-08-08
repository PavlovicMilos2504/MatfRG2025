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
    ImGui::SetNextWindowSize(ImVec2(420, 380), ImGuiCond_FirstUseEver);
    ImGui::Begin("Osvetljenje");

    ImGui::SeparatorText("Direkciono svetlo (mesecina)");
    ImGui::Checkbox("Ukljuceno##dir", &lighting.directional.enabled);
    ImGui::ColorEdit3("Diffuse##dir", &lighting.directional.diffuse.x);
    ImGui::SliderFloat3("Pravac##dir", &lighting.directional.direction.x, -1.0f, 1.0f);

    ImGui::SeparatorText("Lampa iznad stola (tackasto svetlo)");
    ImGui::Checkbox("Ukljucena##lamp", &lighting.lamp.enabled);
    ImGui::ColorEdit3("Diffuse##lamp", &lighting.lamp.diffuse.x);
    ImGui::SliderFloat3("Pozicija##lamp", &lighting.lamp.position.x, -8.0f, 8.0f);
    ImGui::SliderFloat3("Ambient##lamp", &lighting.lamp.ambient.x, 0.0f, 2.0f);
    ImGui::SliderFloat("Slabljenje (linear)##lamp", &lighting.lamp.linear, 0.0f, 0.5f);
    ImGui::SliderFloat("Slabljenje (quadratic)##lamp", &lighting.lamp.quadratic, 0.0f, 1.0f);

    ImGui::SeparatorText("Bloom (HDR sjaj)");
    ImGui::SliderFloat("Prag sjaja##bloom", &engine::core::Controller::get<MainController>()->bloom_threshold(),
                       0.1f, 5.0f);
    ImGui::SliderFloat("Ekspozicija##bloom", &engine::core::Controller::get<MainController>()->bloom_exposure(),
                       0.1f, 5.0f);
    ImGui::SliderInt("Broj blur prolaza##bloom", &engine::core::Controller::get<MainController>()->bloom_blur_passes(),
                     1, 30);
    ImGui::SliderFloat("Radijus rasipanja##bloom", &engine::core::Controller::get<MainController>()->bloom_blur_stride(),
                       1.0f, 8.0f);

    ImGui::SeparatorText("Senke (Point Shadows)");
    ImGui::Checkbox("Ukljucene##shadows", &engine::core::Controller::get<MainController>()->shadows_enabled());
    ImGui::SliderFloat("Bias##shadows", &engine::core::Controller::get<MainController>()->shadow_bias(),
                       0.01f, 0.5f);

    ImGui::SeparatorText("Model stola");
    ImGui::SliderFloat("Skala", &engine::core::Controller::get<MainController>()->table_scale(), 0.1f, 3.0f);

    ImGui::End();
    graphics->end_gui();
}
}// namespace app
