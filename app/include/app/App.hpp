#ifndef APP_APP_HPP
#define APP_APP_HPP

#include <engine/core/Engine.hpp>

namespace app {
/**
 * @brief Entry point application class. Registers all the custom controllers used for the billiard room scene.
 */
class BilliardApp final : public engine::core::App {
    void app_setup() override;
};
}// namespace app

#endif//APP_APP_HPP
