#include "controller/VictoryState.hpp"
#include "controller/TitleState.hpp"
#include "controller/TownState.hpp"
#include "core/AudioManager.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "view/UiSystem.hpp"
#include "view/UiTheme.hpp"

namespace crawl {

VictoryState::VictoryState(Game& game) : m_game(game) {
    AudioManager::getInstance().stopAmbience();
    AudioManager::getInstance().playBgm("bgm_title");
    AudioManager::getInstance().playSfx("sfx_victory");
}

void VictoryState::activate(int idx) {
    AudioManager::getInstance().playSfx("sfx_ui_click");
    m_game.triggerFadeIn(0.3f);
    if (idx == 0) {
        m_game.getStates().replaceAll(std::make_unique<TownState>(m_game));
    } else {
        m_game.getStates().replaceAll(std::make_unique<TitleState>(m_game));
    }
}

void VictoryState::handleInput(const sf::Event& event) {
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
    if (event.key.code == sf::Keyboard::Left || event.key.code == sf::Keyboard::Right) {
        m_selectedIndex = 1 - m_selectedIndex;
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    } else if (event.key.code == sf::Keyboard::Enter) {
        activate(m_selectedIndex);
    } else if (event.key.code == sf::Keyboard::Escape) {
        activate(1);
    }
}

void VictoryState::update(sf::Time deltaTime) { m_time += deltaTime.asSeconds(); }

void VictoryState::draw(sf::RenderWindow& window) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    const auto& lm = LocalizationManager::getInstance();
    const auto mouse = m_game.getMouseLogicalPosition();
    const auto& party = m_game.getParty();

    UiSystem::drawBackdrop(window, assets, "title_bg", 120);
    const sf::FloatRect panel(210.0f, 110.0f, 860.0f, 500.0f);
    UiSystem::drawPanel(window, assets, panel, true, 242);

    UiSystem::drawCenteredText(window, font, lm.get("VICTORY_TITLE"),
                               {panel.left, panel.top + 28.0f, panel.width, 54.0f}, 36, UiTheme::textGold(), true);
    UiSystem::drawCenteredText(window, font, lm.get("VICTORY_SUBTITLE"),
                               {panel.left + 40.0f, panel.top + 92.0f, panel.width - 80.0f, 44.0f},
                               16, UiTheme::textPrimary(), true);

    const std::string summary =
        lm.format("VICTORY_SUMMARY", {
            {"gold", std::to_string(party.getGold())},
            {"turns", std::to_string(party.getTotalTurns())},
            {"defeated", std::to_string(party.getBestiary().totalDefeated())}
        });
    UiSystem::drawCenteredText(window, font, summary,
                               {panel.left + 40.0f, panel.top + 150.0f, panel.width - 80.0f, 36.0f},
                               16, UiTheme::cyanAccent(), true);

    // 승리한 파티 영웅 초상화 진열
    const auto snap = buildPartyHudSnapshot(party);
    const auto& slots = snap.slots();
    for (std::size_t i = 0; i < slots.size() && i < 4; ++i) {
        const float px = panel.left + 32.0f + static_cast<float>(i) * 202.0f;
        UiSystem::drawPortraitCard(window, assets, font, slots[i], {px, panel.top + 214.0f, 192.0f, 134.0f}, true, false, false);
    }

    m_buttonRects[0] = {panel.left + 120.0f, panel.top + 404.0f, 280.0f, 52.0f};
    m_buttonRects[1] = {panel.left + 460.0f, panel.top + 404.0f, 280.0f, 52.0f};
    UiSystem::drawButton(window, assets, font, {
        m_buttonRects[0], lm.get("VICTORY_CONTINUE_TOWN"), "Enter",
        m_selectedIndex == 0, m_buttonRects[0].contains(mouse), true, true
    });
    UiSystem::drawButton(window, assets, font, {
        m_buttonRects[1], lm.get("GAMEOVER_RETURN_TITLE"), "Esc",
        m_selectedIndex == 1, m_buttonRects[1].contains(mouse), true, false
    });
}

} // namespace crawl
