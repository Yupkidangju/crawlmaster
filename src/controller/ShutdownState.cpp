#include "controller/ShutdownState.hpp"
#include "core/AudioManager.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "view/UiSystem.hpp"
#include "view/UiTheme.hpp"

namespace crawl {

ShutdownState::ShutdownState(Game& game, PersistenceStatus initialFailure)
    : m_game(game), m_failureStatus(initialFailure) {
    const auto& lm = LocalizationManager::getInstance();
    m_statusMessage = (initialFailure == PersistenceStatus::RecoveryPending)
                          ? lm.get("SHUTDOWN_RECOVERY_PENDING")
                          : lm.get("SHUTDOWN_SAVE_FAILED");
}

void ShutdownState::retryPrimary() {
    const auto& lm = LocalizationManager::getInstance();
    if (m_failureStatus == PersistenceStatus::RecoveryPending || m_game.getParty().isRecoveryPending()) {
        const auto loadRes = m_game.getParty().loadFromFile();
        if (loadRes) {
            m_game.completeShutdown();
            return;
        }
        m_statusMessage = lm.get("SHUTDOWN_RECOVERY_FAILED");
        AudioManager::getInstance().playSfx("sfx_ui_error");
        return;
    }
    const auto res = m_game.getParty().saveToFile();
    if (res.status == PersistenceStatus::Saved) {
        m_game.completeShutdown();
        return;
    }
    m_statusMessage = lm.get("SHUTDOWN_SAVE_FAILED");
    AudioManager::getInstance().playSfx("sfx_ui_error");
}

void ShutdownState::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        const auto pos = m_game.mapPixelToLogical({event.mouseButton.x, event.mouseButton.y});
        if (m_buttonRects[0].contains(pos)) { retryPrimary(); return; }
        if (m_buttonRects[1].contains(pos)) { m_game.completeShutdown(); return; }
        return;
    }
    if (event.type != sf::Event::KeyPressed) return;
    if (event.key.code == sf::Keyboard::Enter) {
        retryPrimary();
    } else if (event.key.code == sf::Keyboard::Escape) {
        m_game.completeShutdown();
    }
}

void ShutdownState::update(sf::Time /*deltaTime*/) {}

void ShutdownState::draw(sf::RenderWindow& window) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    const auto& lm = LocalizationManager::getInstance();
    const auto mouse = m_game.getMouseLogicalPosition();

    sf::RectangleShape dim({Game::LOGICAL_WIDTH, Game::LOGICAL_HEIGHT});
    dim.setFillColor(sf::Color(4, 4, 8, 220));
    window.draw(dim);

    const sf::FloatRect panel(290.0f, 200.0f, 700.0f, 300.0f);
    UiSystem::drawPanel(window, assets, panel, true, 248);
    UiSystem::drawCenteredText(window, font, lm.get("SHUTDOWN_TITLE"),
                               {panel.left, panel.top + 24.0f, panel.width, 38.0f}, 24, UiTheme::hpCrimson(), true);
    UiSystem::drawCenteredText(window, font, m_statusMessage,
                               {panel.left + 28.0f, panel.top + 92.0f, panel.width - 56.0f, 74.0f},
                               15, UiTheme::textPrimary(), true);

    m_buttonRects[0] = {panel.left + 58.0f, panel.top + 210.0f, 270.0f, 50.0f};
    m_buttonRects[1] = {panel.left + 372.0f, panel.top + 210.0f, 270.0f, 50.0f};
    UiSystem::drawButton(window, assets, font, {
        m_buttonRects[0], lm.get("SHUTDOWN_RETRY"), "Enter", true, m_buttonRects[0].contains(mouse), true, true
    });
    UiSystem::drawButton(window, assets, font, {
        m_buttonRects[1], lm.get("SHUTDOWN_EXIT_UNSAVED"), "Esc", false, m_buttonRects[1].contains(mouse), true, false
    });
}

} // namespace crawl
