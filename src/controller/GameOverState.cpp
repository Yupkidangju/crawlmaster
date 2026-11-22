#include "controller/GameOverState.hpp"
#include "controller/TitleState.hpp"
#include "controller/TownState.hpp"
#include "core/AudioManager.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "view/UiSystem.hpp"
#include "view/UiTheme.hpp"

namespace crawl {

GameOverState::GameOverState(Game& game, bool checkpointRecovered, std::string statusMessage)
    : m_game(game), m_checkpointRecovered(checkpointRecovered), m_statusMessage(std::move(statusMessage)) {
    AudioManager::getInstance().stopBgm();
    AudioManager::getInstance().stopAmbience();
    AudioManager::getInstance().playSfx("sfx_defeat");
}

void GameOverState::activate(int idx) {
    AudioManager::getInstance().playSfx("sfx_ui_click");
    if (idx == 0 && m_checkpointRecovered) {
        m_game.triggerFadeIn(0.28f);
        m_game.getStates().replaceAll(std::make_unique<TownState>(m_game));
    } else {
        m_game.triggerFadeIn(0.28f);
        m_game.getStates().replaceAll(std::make_unique<TitleState>(m_game));
    }
}

void GameOverState::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        const auto pos = m_game.mapPixelToLogical({event.mouseButton.x, event.mouseButton.y});
        for (int i = 0; i < 2; ++i) {
            if (m_buttonRects[static_cast<std::size_t>(i)].contains(pos)) {
                activate(i);
                return;
            }
        }
        return;
    }
    if (event.type != sf::Event::KeyPressed) return;
    if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::Right ||
        event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::Down) {
        m_selectedIndex = 1 - m_selectedIndex;
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    } else if (event.key.code == sf::Keyboard::Enter) {
        activate(m_selectedIndex);
    } else if (event.key.code == sf::Keyboard::Escape) {
        activate(1);
    }
}

void GameOverState::update(sf::Time /*deltaTime*/) {}

void GameOverState::draw(sf::RenderWindow& window) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    const auto& lm = LocalizationManager::getInstance();
    const auto mouse = m_game.getMouseLogicalPosition();

    UiSystem::drawBackdrop(window, assets, "title_bg", 185);
    const sf::FloatRect panel(270.0f, 160.0f, 740.0f, 390.0f);
    UiSystem::drawPanel(window, assets, panel, true, 242);

    UiSystem::drawCenteredText(window, font, lm.get("GAMEOVER_TITLE"),
                               {panel.left, panel.top + 36.0f, panel.width, 54.0f}, 38, UiTheme::hpCrimson(), true);
    UiSystem::drawCenteredText(window, font, lm.get("GAMEOVER_SUBTITLE"),
                               {panel.left + 32.0f, panel.top + 114.0f, panel.width - 64.0f, 44.0f},
                               16, UiTheme::textPrimary(), true);
    if (!m_statusMessage.empty()) {
        UiSystem::drawCenteredText(window, font, m_statusMessage,
                                   {panel.left + 32.0f, panel.top + 178.0f, panel.width - 64.0f, 44.0f},
                                   14, UiTheme::textGold(), true);
    }

    m_buttonRects[0] = {panel.left + 84.0f, panel.top + 276.0f, 260.0f, 52.0f};
    m_buttonRects[1] = {panel.left + 396.0f, panel.top + 276.0f, 260.0f, 52.0f};
    UiSystem::drawButton(window, assets, font, {
        m_buttonRects[0], lm.get("GAMEOVER_RETURN_TOWN"), "Enter",
        m_selectedIndex == 0, m_buttonRects[0].contains(mouse), m_checkpointRecovered, true
    });
    UiSystem::drawButton(window, assets, font, {
        m_buttonRects[1], lm.get("GAMEOVER_RETURN_TITLE"), "Esc",
        m_selectedIndex == 1, m_buttonRects[1].contains(mouse), true, false
    });
}

} // namespace crawl
