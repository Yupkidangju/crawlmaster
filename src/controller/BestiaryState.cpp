#include "controller/BestiaryState.hpp"
#include "core/AudioManager.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "model/ItemFactory.hpp"
#include "model/MonsterFactory.hpp"
#include "view/UiSystem.hpp"
#include "view/UiTheme.hpp"

#include <algorithm>

namespace crawl {

BestiaryState::BestiaryState(Game& game)
    : m_game(game), m_monsterIds(MonsterFactory::getRegisteredIds()) {}

void BestiaryState::handleInput(const sf::Event& event) {
    const int n = static_cast<int>(m_monsterIds.size());
    if (event.type == sf::Event::MouseWheelScrolled && n > 0) {
        m_selectedIndex = std::clamp(m_selectedIndex - static_cast<int>(event.mouseWheelScroll.delta), 0, n - 1);
        return;
    }
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        const auto pos = m_game.mapPixelToLogical({event.mouseButton.x, event.mouseButton.y});
        if (m_closeRect.contains(pos)) {
            AudioManager::getInstance().playSfx("sfx_ui_click");
            m_game.getStates().popState();
            return;
        }
        for (std::size_t i = 0; i < m_rowRects.size(); ++i) {
            if (m_rowRects[i].contains(pos)) {
                m_selectedIndex = m_scrollOffset + static_cast<int>(i);
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
    if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::B) {
        AudioManager::getInstance().playSfx("sfx_ui_click");
        m_game.getStates().popState();
    } else if (event.key.code == sf::Keyboard::Up || event.key.code == sf::Keyboard::W) {
        if (n > 0) m_selectedIndex = (m_selectedIndex + n - 1) % n;
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    } else if (event.key.code == sf::Keyboard::Down || event.key.code == sf::Keyboard::S) {
        if (n > 0) m_selectedIndex = (m_selectedIndex + 1) % n;
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    }
}

void BestiaryState::update(sf::Time /*deltaTime*/) {}

void BestiaryState::draw(sf::RenderWindow& window) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    const auto& lm = LocalizationManager::getInstance();
    const auto& bestiary = m_game.getParty().getBestiary();
    const auto mouse = m_game.getMouseLogicalPosition();

    sf::RectangleShape dim({Game::LOGICAL_WIDTH, Game::LOGICAL_HEIGHT});
    dim.setFillColor(sf::Color(5, 6, 10, 215));
    window.draw(dim);

    const sf::FloatRect frame(56.0f, 30.0f, 1168.0f, 660.0f);
    UiSystem::drawPanel(window, assets, frame, true, 244);
    UiSystem::drawText(window, font,
                       lm.get("BESTIARY_TITLE") + " (" + std::to_string(bestiary.discoveredCount()) + "/" +
                           std::to_string(m_monsterIds.size()) + ")",
                       {frame.left + 24.0f, frame.top + 16.0f}, 22, UiTheme::textGold(), true);
    m_closeRect = {frame.left + frame.width - 196.0f, frame.top + 14.0f, 172.0f, 38.0f};
    UiSystem::drawButton(window, assets, font, {m_closeRect, lm.get("COMMON_BACK"), "Esc", false, m_closeRect.contains(mouse), true, false});

    // 좌측 몬스터 리스트 (14개 가시 스크롤)
    constexpr int MAX_VIS = 14;
    if (m_selectedIndex < m_scrollOffset) m_scrollOffset = m_selectedIndex;
    if (m_selectedIndex >= m_scrollOffset + MAX_VIS) m_scrollOffset = m_selectedIndex - MAX_VIS + 1;

    m_rowRects.clear();
    for (int vis = 0; vis < MAX_VIS; ++vis) {
        const int idx = m_scrollOffset + vis;
        if (idx >= static_cast<int>(m_monsterIds.size())) break;
        const std::string& id = m_monsterIds[static_cast<std::size_t>(idx)];
        const auto mon = MonsterFactory::createMonster(id);
        const auto entry = bestiary.getEntry(id);
        const bool seen = entry.encountered > 0;
        const sf::FloatRect r(frame.left + 24.0f, frame.top + 68.0f + static_cast<float>(vis) * 40.0f, 400.0f, 35.0f);
        m_rowRects.push_back(r);

        const bool sel = (idx == m_selectedIndex) || r.contains(mouse);
        sf::RectangleShape bg({r.width, r.height});
        bg.setPosition(r.left, r.top);
        bg.setFillColor(sel ? UiTheme::panelHover() : UiTheme::panelAlt());
        bg.setOutlineThickness(sel ? 1.5f : 1.0f);
        bg.setOutlineColor(sel ? UiTheme::borderGold() : UiTheme::borderBronze());
        window.draw(bg);

        const std::string label = std::to_string(MonsterFactory::getMonsterFloor(id)) + "F · " +
                                  (seen ? mon->getName() : "???") + (mon->isBoss() ? " [BOSS]" : "");
        UiSystem::drawText(window, font, label, {r.left + 12.0f, r.top + 8.0f}, 14,
                           mon->isBoss() ? UiTheme::textGold() : (seen ? UiTheme::textPrimary() : UiTheme::textMuted()),
                           true, 310.0f);
        UiSystem::drawText(window, font, "x" + std::to_string(entry.defeated),
                           {r.left + r.width - 54.0f, r.top + 8.0f}, 12, UiTheme::textMuted());
    }

    // 우측 선택 몬스터 상세 도감 카드
    if (!m_monsterIds.empty()) {
        const std::string& id = m_monsterIds[static_cast<std::size_t>(m_selectedIndex)];
        const auto mon = MonsterFactory::createMonster(id);
        const auto entry = bestiary.getEntry(id);
        const bool seen = entry.encountered > 0;

        const sf::FloatRect det(frame.left + 444.0f, frame.top + 68.0f, 700.0f, 566.0f);
        UiSystem::drawPanel(window, assets, det, mon->isBoss(), 230);

        if (const sf::Texture* tex = assets.getMonsterTexture(id)) {
            sf::Sprite sp(*tex);
            const auto sz = tex->getSize();
            sp.setScale(290.0f / static_cast<float>(sz.x), 290.0f / static_cast<float>(sz.y));
            sp.setPosition(det.left + 24.0f, det.top + 28.0f);
            if (!seen) sp.setColor(sf::Color(18, 18, 24, 240));
            window.draw(sp);
        }

        const float ix = det.left + 336.0f;
        UiSystem::drawText(window, font, seen ? mon->getName() : lm.get("BESTIARY_UNKNOWN"),
                           {ix, det.top + 32.0f}, 22, mon->isBoss() ? UiTheme::textGold() : UiTheme::textPrimary(), true, 340.0f);
        UiSystem::drawText(window, font,
                           lm.format("DUNGEON_FLOOR_LABEL", {{"floor", std::to_string(MonsterFactory::getMonsterFloor(id))}}) +
                               (mon->isBoss() ? " · BOSS" : ""),
                           {ix, det.top + 64.0f}, 14, UiTheme::cyanAccent());
        UiSystem::drawText(window, font,
                           "HP: " + (seen ? std::to_string(mon->getMaxHp()) : "?") +
                               "   AC: " + (seen ? std::to_string(mon->getAc()) : "?") +
                               "   Stagger: " + (seen ? std::to_string(mon->getMaxStagger()) : "?"),
                           {ix, det.top + 102.0f}, 14, UiTheme::textPrimary());
        UiSystem::drawText(window, font,
                           "XP: " + (seen ? std::to_string(mon->getXpReward()) : "?") +
                               "   Defeated: " + std::to_string(entry.defeated),
                           {ix, det.top + 130.0f}, 14, UiTheme::textMuted());

        // 약점 & 저항 배지
        UiSystem::drawText(window, font, lm.get("BESTIARY_WEAKNESSES"), {ix, det.top + 178.0f}, 14, UiTheme::staggerAmber());
        float bx = ix;
        if (entry.weaknessRevealed) {
            for (const auto w : mon->getWeaknesses()) {
                UiSystem::drawBadge(window, font, {bx, det.top + 204.0f}, UiTheme::elementShortCode(w),
                                    sf::Color(28, 22, 18), UiTheme::elementColor(w), 11);
                bx += 48.0f;
            }
        } else {
            UiSystem::drawText(window, font, lm.get("BESTIARY_DEFEAT_TO_REVEAL"), {ix, det.top + 204.0f}, 13, UiTheme::textMuted());
        }

        UiSystem::drawText(window, font, lm.get("BESTIARY_RESISTANCES"), {ix, det.top + 248.0f}, 14, UiTheme::textMuted());
        bx = ix;
        if (entry.weaknessRevealed) {
            for (const auto r : mon->getResistances()) {
                UiSystem::drawBadge(window, font, {bx, det.top + 274.0f}, UiTheme::elementShortCode(r),
                                    sf::Color(20, 22, 30), UiTheme::elementColor(r), 11);
                bx += 48.0f;
            }
        }

        // 드롭 테이블
        UiSystem::drawText(window, font, lm.get("BESTIARY_DROPS"), {det.left + 28.0f, det.top + 356.0f}, 16, UiTheme::textGold());
        float dy = det.top + 390.0f;
        for (const auto& did : MonsterFactory::getDropItemIds(id)) {
            const auto item = ItemFactory::createItem(did);
            if (!item) continue;
            UiSystem::drawText(window, font, "• " + (seen ? item->getName() : "???"),
                               {det.left + 36.0f, dy}, 14,
                               seen ? UiTheme::rarityColor(item->getRarity()) : UiTheme::textMuted());
            dy += 28.0f;
        }
    }
}

} // namespace crawl
