#include <app/EventController.hpp>
#include <app/GUIController.hpp>
#include <app/MainController.hpp>
#include <engine/core/Engine.hpp>
#include <engine/graphics/GraphicsController.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <spdlog/spdlog.h>

namespace app {
void MainPlatformEventObserver::on_key(engine::platform::Key key) {
    if (key.id() == engine::platform::KEY_L && key.state() == engine::platform::Key::State::JustPressed) {
        engine::core::Controller::get<MainController>()->trigger_lamp_switch();
    }
}

void MainController::initialize() {
    engine::graphics::OpenGL::enable_depth_testing();

    auto observer = std::make_unique<MainPlatformEventObserver>();
    engine::core::Controller::get<engine::platform::PlatformController>()->register_platform_event_observer(
            std::move(observer));

    auto camera = engine::core::Controller::get<engine::graphics::GraphicsController>()->camera();
    camera->Position = glm::vec3(0.0f, 1.6f, 3.0f);

    // Room starts dim; only the moonlight-like directional light is on until the lamp is switched on.
    m_lighting.lamp.enabled = false;
}

bool MainController::loop() {
    const auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    if (platform->key(engine::platform::KEY_ESCAPE).state() == engine::platform::Key::State::JustPressed) {
        return false;
    }
    return true;
}

void MainController::poll_events() {
    const auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    if (platform->key(engine::platform::KEY_F1).state() == engine::platform::Key::State::JustPressed) {
        m_cursor_enabled = !m_cursor_enabled;
        platform->set_enable_cursor(m_cursor_enabled);
    }
}

void MainController::update() {
    update_camera();
}

void MainController::begin_draw() {
    engine::graphics::OpenGL::clear_buffers();
}

void MainController::draw() {
    draw_table();
}

void MainController::end_draw() {
    engine::core::Controller::get<engine::platform::PlatformController>()->swap_buffers();
}

void MainController::trigger_lamp_switch() {
    if (m_lamp_switch_in_progress) {
        return;
    }
    m_lamp_switch_in_progress = true;

    auto events = engine::core::Controller::get<EventController>();
    spdlog::info("[MainController]: lamp switch pressed, lamp will turn on shortly...");

    // ACTION (key L pressed) --- after 1s ---> EVENT_A (lamp turns on)
    events->schedule(1.0f, [this, events]() {
        m_lighting.lamp.enabled = true;
        spdlog::info("[MainController]: lamp turned on above the table.");

        // EVENT_A --- after 2s ---> EVENT_B (lamp warms up to full brightness)
        events->schedule(2.0f, [this]() {
            m_lighting.lamp.diffuse = glm::vec3(1.0f, 0.85f, 0.6f);
            m_lighting.lamp.ambient = glm::vec3(0.08f, 0.07f, 0.05f);
            m_lamp_switch_in_progress = false;
            spdlog::info("[MainController]: lamp reached full brightness.");
        });
    });
}

void MainController::draw_table() {
    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto shader = resources->shader("lighting");
    auto table = resources->model("billiard_table");

    shader->use();
    shader->set_mat4("projection", graphics->projection_matrix());
    shader->set_mat4("view", graphics->camera()->view_matrix());
    shader->set_mat4("model", glm::scale(glm::mat4(1.0f), glm::vec3(m_table_scale)));
    shader->set_vec3("viewPos", graphics->camera()->Position);
    shader->set_float("specularStrength", 0.3f);

    const auto &dir = m_lighting.directional;
    shader->set_bool("dirLight.enabled", dir.enabled);
    shader->set_vec3("dirLight.direction", dir.direction);
    shader->set_vec3("dirLight.ambient", dir.ambient);
    shader->set_vec3("dirLight.diffuse", dir.diffuse);
    shader->set_vec3("dirLight.specular", dir.specular);

    const auto &lamp = m_lighting.lamp;
    shader->set_bool("pointLight.enabled", lamp.enabled);
    shader->set_vec3("pointLight.position", lamp.position);
    shader->set_vec3("pointLight.ambient", lamp.ambient);
    shader->set_vec3("pointLight.diffuse", lamp.diffuse);
    shader->set_vec3("pointLight.specular", lamp.specular);
    shader->set_float("pointLight.constant", lamp.constant);
    shader->set_float("pointLight.linear", lamp.linear);
    shader->set_float("pointLight.quadratic", lamp.quadratic);

    table->draw(shader);
}

void MainController::update_camera() {
    auto gui = engine::core::Controller::get<GUIController>();
    if (gui->is_enabled()) {
        return;
    }
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    auto camera = engine::core::Controller::get<engine::graphics::GraphicsController>()->camera();
    float dt = platform->dt();
    if (platform->key(engine::platform::KEY_W).state() == engine::platform::Key::State::Pressed) {
        camera->move_camera(engine::graphics::Camera::Movement::FORWARD, dt);
    }
    if (platform->key(engine::platform::KEY_S).state() == engine::platform::Key::State::Pressed) {
        camera->move_camera(engine::graphics::Camera::Movement::BACKWARD, dt);
    }
    if (platform->key(engine::platform::KEY_A).state() == engine::platform::Key::State::Pressed) {
        camera->move_camera(engine::graphics::Camera::Movement::LEFT, dt);
    }
    if (platform->key(engine::platform::KEY_D).state() == engine::platform::Key::State::Pressed) {
        camera->move_camera(engine::graphics::Camera::Movement::RIGHT, dt);
    }
    auto mouse = platform->mouse();
    camera->rotate_camera(mouse.dx, mouse.dy);
    camera->zoom(mouse.scroll);
}
}// namespace app
