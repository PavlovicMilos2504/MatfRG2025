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
    // Positioned in the room's open corner (x close to the max bound, x:[-4.46,3.64], which has no
    // wall geometry), looking diagonally across the room at the billiard table (approx. (-0.4, 0.75, 4.0)).
    camera->Position = glm::vec3(3.0f, 1.7f, 2.0f);
    camera->Yaw = 149.5f;
    camera->Pitch = -5.0f;
    // rotate_camera(0, 0) forces Front/Right/Up to be recomputed from the Yaw/Pitch set above,
    // since update_camera_vectors() is private and only invoked from the constructor or rotate_camera().
    camera->rotate_camera(0.0f, 0.0f);

    // Room starts dim; only the moonlight-like directional light is on until the lamp is switched on.
    m_lighting.lamp.enabled = false;
    m_lighting.lamp.position = glm::vec3(-0.315f, 1.61f, 4.1f);

    // Lock the cursor immediately so mouse movement rotates the camera from the very first frame.
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    m_cursor_enabled = false;
    platform->set_enable_cursor(m_cursor_enabled);
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
    // Shadow pass must run before the bloom HDR capture begins (both use framebuffer 0 when done).
    draw_shadow_pass();
    engine::core::Controller::get<engine::graphics::GraphicsController>()->bloom()->begin_capture();
}

void MainController::draw() {
    draw_table();
    draw_lamp_bulb();

    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    graphics->bloom()->end_capture();

    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    graphics->bloom()->apply(resources->shader("blur"), resources->shader("bloom_composite"), m_bloom_exposure,
                             m_bloom_blur_passes, m_bloom_blur_stride);
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

void MainController::draw_shadow_pass() {
    if (!m_lighting.lamp.enabled) {
        return;
    }

    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    auto point_shadow = graphics->point_shadow();
    auto shader = resources->shader("point_shadow_depth");
    auto table = resources->model("billiard_table");

    auto light_space_matrices = point_shadow->light_space_matrices(m_lighting.lamp.position);
    shader->use();
    for (size_t i = 0; i < light_space_matrices.size(); ++i) {
        shader->set_mat4("shadowMatrices[" + std::to_string(i) + "]", light_space_matrices[i]);
    }
    shader->set_vec3("lightPos", m_lighting.lamp.position);
    shader->set_float("far_plane", point_shadow->far_plane());
    shader->set_mat4("model", glm::scale(glm::mat4(1.0f), glm::vec3(m_table_scale)));

    point_shadow->begin_capture();
    table->draw(shader);
    point_shadow->end_capture(platform->window()->width(), platform->window()->height());
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
    shader->set_float("bloomThreshold", m_bloom_threshold);

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

    // Unit 10 avoids colliding with the mesh's own textures (Mesh::draw binds from unit 0).
    graphics->point_shadow()->bind(shader, "shadowMap", 10);
    shader->set_float("farPlane", graphics->point_shadow()->far_plane());
    shader->set_float("shadowBias", m_shadow_bias);
    shader->set_bool("shadowsEnabled", m_shadows_enabled);

    table->draw(shader);
}

void MainController::draw_lamp_bulb() {
    if (!m_lighting.lamp.enabled) {
        return;
    }
    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto shader = resources->shader("emissive");
    const auto &lamp = m_lighting.lamp;

    glm::mat4 model = glm::translate(glm::mat4(1.0f), lamp.position);
    model = glm::scale(model, glm::vec3(0.04f));

    shader->use();
    shader->set_mat4("model", model);
    shader->set_mat4("view", graphics->camera()->view_matrix());
    shader->set_mat4("projection", graphics->projection_matrix());
    // Boosted beyond [0,1] so the bulb reliably exceeds bloomThreshold and blooms.
    shader->set_vec3("emissiveColor", lamp.diffuse * 3.0f);
    shader->set_float("bloomThreshold", m_bloom_threshold);

    graphics->draw_unit_cube(shader);
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
