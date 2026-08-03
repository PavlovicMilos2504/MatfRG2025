#include <app/App.hpp>
#include <app/EventController.hpp>
#include <app/GUIController.hpp>
#include <app/MainController.hpp>

namespace app {
void BilliardApp::app_setup() {
    auto event_controller = register_controller<EventController>();
    auto main_controller = register_controller<MainController>();
    auto gui_controller = register_controller<GUIController>();

    event_controller->after(engine::core::Controller::get<engine::core::EngineControllersEnd>());
    main_controller->after(event_controller);
    gui_controller->after(main_controller);
}
}// namespace app

int main(int argc, char **argv) {
    return std::make_unique<app::BilliardApp>()->run(argc, argv);
}
