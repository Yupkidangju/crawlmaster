#include "controller/QuestJournalState.hpp"
#include "core/AudioManager.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "model/Quest.hpp"
#include "view/UiSystem.hpp"
#include "view/UiTheme.hpp"

namespace crawl {

QuestJournalState::QuestJournalState(Game& game) : m_game(game) {}

void QuestJournalState::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        const auto pos = m_game.mapPixelToLogical({event.mouseButton.x, event.mouseButton.y});
        if (m_closeRect.contains(pos)) {
            AudioManager::getInstance().playSfx("sfx_ui_click");
            m_game.getStates().popState();
            return;
        }
        for (std::size_t i = 0; i < m_rowRects.size(); ++i) {
            if (m_rowRects[i].contains(pos)) {
                m_selectedIndex = static_cast<int>(i);
                AudioManager::getInstance().playSfx("sfx_ui_hover");
                return;
            }
        }
        return;
    }
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
        AudioManager::getInstance().playSfx("sfx_ui_click");
        m_game.getStates().popState();
        return;
    }
    if (event.type != sf::Event::KeyPressed) return;
    if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::J ||
        event.key.code == sf::Keyboard::Q) {
        AudioManager::getInstance().playSfx("sfx_ui_click");
        m_game.getStates().popState();
    } else if (event.key.code == sf::Keyboard::Up) {
        m_selectedIndex = std::max(0, m_selectedIndex - 1);
    } else if (event.key.code == sf::Keyboard::Down) {
        ++m_selectedIndex;
    }
}

void QuestJournalState::update(sf::Time /*deltaTime*/) {}

void QuestJournalState::draw(sf::RenderWindow& window) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    const auto& lm = LocalizationManager::getInstance();
    const auto& party = m_game.getParty();
    const auto mouse = m_game.getMouseLogicalPosition();

    sf::RectangleShape dim({Game::LOGICAL_WIDTH, Game::LOGICAL_HEIGHT});
    dim.setFillColor(sf::Color(5, 6, 10, 212));
    window.draw(dim);

    const sf::FloatRect frame(110.0f, 44.0f, 1060.0f, 632.0f);
    UiSystem::drawPanel(window, assets, frame, true, 244);
    UiSystem::drawText(window, font, lm.get("JOURNAL_TITLE"), {frame.left + 26.0f, frame.top + 18.0f}, 24, UiTheme::textGold(), true);
    m_closeRect = {frame.left + frame.width - 196.0f, frame.top + 16.0f, 172.0f, 38.0f};
    UiSystem::drawButton(window, assets, font, {m_closeRect, lm.get("COMMON_BACK"), "Esc", false, m_closeRect.contains(mouse), true, false});

    m_rowRects.clear();
    const auto& active = party.getActiveQuests();
    float y = frame.top + 74.0f;
    if (active.empty()) {
        UiSystem::drawText(window, font, lm.get("QUEST_NONE_ACTIVE"), {frame.left + 32.0f, y}, 15, UiTheme::textMuted());
    } else {
        for (std::size_t i = 0; i < active.size(); ++i) {
            const auto& q = active[i];
            const sf::FloatRect r(frame.left + 26.0f, y, 660.0f, 64.0f);
            m_rowRects.push_back(r);
            UiSystem::drawPanel(window, assets, r, q->isReadyToReport(), 228);
            UiSystem::drawText(window, font,
                               "[" + std::to_string(q->getTargetFloor()) + "F] " + q->getName(),
                               {r.left + 14.0f, r.top + 10.0f}, 15, UiTheme::textGold(), true, 490.0f);
            UiSystem::drawText(window, font, q->getDescription(),
                               {r.left + 14.0f, r.top + 34.0f}, 12, UiTheme::textPrimary(), false, 520.0f);
            UiSystem::drawBadge(window, font, {r.left + r.width - 110.0f, r.top + 20.0f},
                                q->isReadyToReport() ? lm.get("QUEST_REPORT") : lm.get("QUEST_ACTIVE"),
                                sf::Color(18, 22, 28),
                                q->isReadyToReport() ? UiTheme::statPos() : UiTheme::cyanAccent(), 11);
            y += 72.0f;
        }
    }

    // 우측: 중요품 & 완료 퀘스트 요약
    const sf::FloatRect side(frame.left + 706.0f, frame.top + 74.0f, 328.0f, 528.0f);
    UiSystem::drawPanel(window, assets, side, false, 224);
    UiSystem::drawText(window, font, lm.get("JOURNAL_KEY_ITEMS"), {side.left + 16.0f, side.top + 16.0f}, 16, UiTheme::textGold());
    float ky = side.top + 48.0f;
    if (party.getKeyItems().empty()) {
        UiSystem::drawText(window, font, lm.get("CHAR_INFO_EMPTY"), {side.left + 16.0f, ky}, 13, UiTheme::textMuted());
    } else {
        for (const auto& ki : party.getKeyItems()) {
            UiSystem::drawText(window, font, "• " + lm.getContent("KEYITEM", ki, "NAME"),
                               {side.left + 16.0f, ky}, 14, UiTheme::textPrimary());
            ky += 26.0f;
        }
    }

    UiSystem::drawText(window, font,
                       lm.get("JOURNAL_COMPLETED") + " (" + std::to_string(party.getCompletedQuestIds().size()) + "/8)",
                       {side.left + 16.0f, side.top + 230.0f}, 16, UiTheme::statPos());
    float cy = side.top + 264.0f;
    for (const auto& cid : party.getCompletedQuestIds()) {
        if (auto q = Quest::createCanonical(cid)) {
            UiSystem::drawText(window, font, "✓ " + q->getName(), {side.left + 16.0f, cy}, 13, UiTheme::textMuted(), false, 296.0f);
            cy += 24.0f;
        }
    }
}

} // namespace crawl
