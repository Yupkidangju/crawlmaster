#include "controller/CharacterCreationState.hpp"
#include "core/AudioManager.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "model/CharacterIdentityRules.hpp"
#include "view/UiSystem.hpp"
#include "view/UiTheme.hpp"

#include <array>

namespace crawl {
namespace {

constexpr std::array<Ability, 6> kAbilities = {
    Ability::STRENGTH, Ability::DEXTERITY, Ability::CONSTITUTION,
    Ability::INTELLIGENCE, Ability::WISDOM, Ability::CHARISMA
};

constexpr std::array<const char*, 6> kAbilityKeys = {
    "STAT_STR", "STAT_DEX", "STAT_CON", "STAT_INT", "STAT_WIS", "STAT_CHA"
};

} // namespace

CharacterCreationState::CharacterCreationState(Game& game)
    : m_game(game), m_draft(SessionRng::global()) {
    m_nameBuffer = "Hero";
    m_draft.setName(m_nameBuffer);
}

void CharacterCreationState::adjustFocused(int delta) {
    if (m_focusIndex == static_cast<int>(FocusRow::AGE)) {
        m_draft.setAge(m_draft.identity().age + delta);
    } else if (m_focusIndex == static_cast<int>(FocusRow::GENDER)) {
        const int cur = static_cast<int>(m_draft.identity().gender);
        m_draft.setGender(static_cast<Gender>((cur + delta + 4) % 4));
    } else if (m_focusIndex == static_cast<int>(FocusRow::CLASS)) {
        const int cur = static_cast<int>(m_draft.identity().characterClass);
        m_draft.setClass(static_cast<CharacterClass>((cur + delta + 4) % 4));
    } else if (m_focusIndex >= static_cast<int>(FocusRow::STR) &&
               m_focusIndex <= static_cast<int>(FocusRow::CHA)) {
        const auto ab = kAbilities[static_cast<std::size_t>(m_focusIndex - 4)];
        if (delta > 0) m_draft.increase(ab);
        else m_draft.decrease(ab);
    }
    AudioManager::getInstance().playSfx("sfx_ui_click");
}

void CharacterCreationState::attemptConfirm() {
    const auto& lm = LocalizationManager::getInstance();
    m_draft.setName(m_nameBuffer);
    if (!m_draft.isReady()) {
        m_statusMessage = lm.get("CHAR_CREATE_INVALID");
        AudioManager::getInstance().playSfx("sfx_ui_error");
        return;
    }
    if (m_game.getParty().getMemberCount() >= Party::MAX_MEMBERS) {
        m_statusMessage = lm.get("TOWN_GUILD_FULL");
        AudioManager::getInstance().playSfx("sfx_ui_error");
        return;
    }
    const PartyCheckpoint cp = m_game.getParty().captureCheckpoint();
    if (!m_draft.confirm(m_game.getParty())) {
        m_statusMessage = lm.get("CHAR_CREATE_INVALID");
        AudioManager::getInstance().playSfx("sfx_ui_error");
        return;
    }
    const auto saveRes = m_game.getParty().saveToFile();
    if (!saveRes || !saveRes.durabilityConfirmed()) {
        m_game.getParty().restoreCheckpoint(cp);
        m_statusMessage = lm.get("TITLE_SAVE_ERROR");
        AudioManager::getInstance().playSfx("sfx_ui_error");
        return;
    }
    AudioManager::getInstance().playSfx("sfx_levelup");
    m_game.pushToast(lm.format("CHAR_CREATE_SUCCESS", {{"name", m_draft.identity().name}}));
    m_game.getStates().popState();
}

void CharacterCreationState::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::TextEntered && m_focusIndex == static_cast<int>(FocusRow::NAME)) {
        const sf::Uint32 u = event.text.unicode;
        if (u == 8) {
            if (!m_nameBuffer.empty()) {
                m_nameBuffer.pop_back();
                m_draft.setName(m_nameBuffer);
            }
        } else if (u >= 32 && u < 127 && m_nameBuffer.size() < 18) {
            m_nameBuffer.push_back(static_cast<char>(u));
            m_draft.setName(m_nameBuffer);
        }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        const auto pos = m_game.mapPixelToLogical({event.mouseButton.x, event.mouseButton.y});
        for (int i = 0; i < 4; ++i) {
            if (m_classRects[static_cast<std::size_t>(i)].contains(pos)) {
                m_draft.setClass(static_cast<CharacterClass>(i));
                m_focusIndex = static_cast<int>(FocusRow::CLASS);
                AudioManager::getInstance().playSfx("sfx_ui_click");
                return;
            }
        }
        for (int i = 0; i < 6; ++i) {
            if (m_statMinusRects[static_cast<std::size_t>(i)].contains(pos)) {
                m_draft.decrease(kAbilities[static_cast<std::size_t>(i)]);
                AudioManager::getInstance().playSfx("sfx_ui_click");
                return;
            }
            if (m_statPlusRects[static_cast<std::size_t>(i)].contains(pos)) {
                m_draft.increase(kAbilities[static_cast<std::size_t>(i)]);
                AudioManager::getInstance().playSfx("sfx_ui_click");
                return;
            }
        }
        if (m_ageMinusRect.contains(pos)) { m_draft.setAge(m_draft.identity().age - 1); AudioManager::getInstance().playSfx("sfx_ui_click"); return; }
        if (m_agePlusRect.contains(pos)) { m_draft.setAge(m_draft.identity().age + 1); AudioManager::getInstance().playSfx("sfx_ui_click"); return; }
        if (m_genderRect.contains(pos)) {
            const int cur = static_cast<int>(m_draft.identity().gender);
            m_draft.setGender(static_cast<Gender>((cur + 1) % 4));
            AudioManager::getInstance().playSfx("sfx_ui_click");
            return;
        }
        if (m_rerollRect.contains(pos)) { m_draft.reroll(); AudioManager::getInstance().playSfx("sfx_ui_click"); return; }
        if (m_confirmRect.contains(pos)) { attemptConfirm(); return; }
        if (m_cancelRect.contains(pos)) { AudioManager::getInstance().playSfx("sfx_ui_click"); m_game.getStates().popState(); return; }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
        AudioManager::getInstance().playSfx("sfx_ui_click");
        m_game.getStates().popState();
        return;
    }

    if (event.type != sf::Event::KeyPressed) return;
    const auto key = event.key.code;
    if (key == sf::Keyboard::Escape) {
        AudioManager::getInstance().playSfx("sfx_ui_click");
        m_game.getStates().popState();
        return;
    }
    if (key == sf::Keyboard::Up) {
        m_focusIndex = (m_focusIndex + 12) % 13;
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    } else if (key == sf::Keyboard::Down || key == sf::Keyboard::Tab) {
        m_focusIndex = (m_focusIndex + 1) % 13;
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    } else if (key == sf::Keyboard::Left) {
        adjustFocused(-1);
    } else if (key == sf::Keyboard::Right) {
        adjustFocused(1);
    } else if (key == sf::Keyboard::R && m_focusIndex != static_cast<int>(FocusRow::NAME)) {
        m_draft.reroll();
        AudioManager::getInstance().playSfx("sfx_ui_click");
    } else if (key == sf::Keyboard::Enter) {
        if (m_focusIndex == static_cast<int>(FocusRow::REROLL)) {
            m_draft.reroll();
            AudioManager::getInstance().playSfx("sfx_ui_click");
        } else if (m_focusIndex == static_cast<int>(FocusRow::CANCEL)) {
            m_game.getStates().popState();
        } else if (m_focusIndex == static_cast<int>(FocusRow::CONFIRM)) {
            attemptConfirm();
        } else {
            m_focusIndex = (m_focusIndex + 1) % 13;
        }
    }
}

void CharacterCreationState::update(sf::Time /*deltaTime*/) {}

void CharacterCreationState::draw(sf::RenderWindow& window) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    const auto& lm = LocalizationManager::getInstance();
    const auto mouse = m_game.getMouseLogicalPosition();

    UiSystem::drawBackdrop(window, assets, "town_bg", 145);

    const sf::FloatRect frame(70.0f, 36.0f, 1140.0f, 648.0f);
    UiSystem::drawPanel(window, assets, frame, true, 238);
    UiSystem::drawCenteredText(window, font, lm.get("CHAR_CREATE_TITLE"),
                               {frame.left, frame.top + 14.0f, frame.width, 36.0f}, 24, UiTheme::textGold(), true);

    // 좌측: 초상화 & 직업 선택 카드
    const auto& id = m_draft.identity();
    static constexpr std::array<const char*, 4> kSlugs = {"warrior", "mage", "rogue", "cleric"};
    static constexpr std::array<const char*, 4> kClassKeys = {"CLASS_WARRIOR", "CLASS_MAGE", "CLASS_ROGUE", "CLASS_CLERIC"};
    const std::string curSlug = kSlugs[static_cast<std::size_t>(id.characterClass)];

    const sf::FloatRect pBox(108.0f, 96.0f, 236.0f, 236.0f);
    UiSystem::drawPanel(window, assets, pBox, true, 240);
    if (const sf::Texture* tex = assets.getPortraitTexture(curSlug)) {
        sf::Sprite sp(*tex);
        const auto sz = tex->getSize();
        sp.setScale(220.0f / static_cast<float>(sz.x), 220.0f / static_cast<float>(sz.y));
        sp.setPosition(pBox.left + 8.0f, pBox.top + 8.0f);
        window.draw(sp);
    }

    for (int i = 0; i < 4; ++i) {
        const float bx = 108.0f + static_cast<float>(i % 2) * 122.0f;
        const float by = 348.0f + static_cast<float>(i / 2) * 52.0f;
        m_classRects[static_cast<std::size_t>(i)] = {bx, by, 114.0f, 44.0f};
        UiSystem::drawButton(window, assets, font, {
            m_classRects[static_cast<std::size_t>(i)], lm.get(kClassKeys[static_cast<std::size_t>(i)]), "",
            static_cast<int>(id.characterClass) == i,
            m_classRects[static_cast<std::size_t>(i)].contains(mouse), true,
            static_cast<int>(id.characterClass) == i
        });
    }

    // 클래스 추천 진형/설명
    const std::string classDesc = lm.get("CLASS_DESC_" + std::to_string(static_cast<int>(id.characterClass)));
    const auto descLines = UiSystem::wrapUtf8Lines(classDesc, font, 13, 236.0f);
    float dy = 464.0f;
    for (const auto& l : descLines) {
        UiSystem::drawText(window, font, l, {108.0f, dy}, 13, UiTheme::textMuted(), false);
        dy += 18.0f;
    }

    // 우측: 신원 입력 + 6대 능력치 가중 배분
    const float rx = 390.0f;
    UiSystem::drawPanel(window, assets, {rx, 96.0f, 780.0f, 128.0f}, false, 224);

    // 이름 입력
    UiSystem::drawText(window, font, lm.get("CHAR_CREATE_NAME"), {rx + 18.0f, 110.0f}, 14, UiTheme::textMuted());
    sf::RectangleShape nameBox({290.0f, 34.0f});
    nameBox.setPosition(rx + 18.0f, 134.0f);
    nameBox.setFillColor(sf::Color(12, 13, 19));
    nameBox.setOutlineThickness(m_focusIndex == 0 ? 2.0f : 1.0f);
    nameBox.setOutlineColor(m_focusIndex == 0 ? UiTheme::borderGold() : UiTheme::borderBronze());
    window.draw(nameBox);
    UiSystem::drawText(window, font, m_nameBuffer + (m_focusIndex == 0 ? "_" : ""),
                       {rx + 28.0f, 142.0f}, 15, UiTheme::textPrimary(), false, 270.0f);

    // 나이
    UiSystem::drawText(window, font, lm.get("CHAR_CREATE_AGE"), {rx + 332.0f, 110.0f}, 14, UiTheme::textMuted());
    m_ageMinusRect = {rx + 332.0f, 134.0f, 34.0f, 34.0f};
    m_agePlusRect = {rx + 428.0f, 134.0f, 34.0f, 34.0f};
    UiSystem::drawButton(window, assets, font, {m_ageMinusRect, "-", "", false, m_ageMinusRect.contains(mouse), true, false});
    UiSystem::drawCenteredText(window, font, std::to_string(id.age), {rx + 366.0f, 134.0f, 62.0f, 34.0f}, 16, UiTheme::textPrimary());
    UiSystem::drawButton(window, assets, font, {m_agePlusRect, "+", "", false, m_agePlusRect.contains(mouse), true, false});

    // 성별
    UiSystem::drawText(window, font, lm.get("CHAR_CREATE_GENDER"), {rx + 492.0f, 110.0f}, 14, UiTheme::textMuted());
    m_genderRect = {rx + 492.0f, 134.0f, 260.0f, 34.0f};
    static constexpr std::array<const char*, 4> kGenderKeys = {
        "GENDER_MALE", "GENDER_FEMALE", "GENDER_NON_BINARY", "GENDER_UNSPECIFIED"
    };
    UiSystem::drawButton(window, assets, font, {
        m_genderRect, lm.get(kGenderKeys[static_cast<std::size_t>(id.gender)]), "",
        m_focusIndex == 2, m_genderRect.contains(mouse), true, false
    });

    // 능력치 분배 패널
    UiSystem::drawPanel(window, assets, {rx, 240.0f, 780.0f, 326.0f}, false, 224);
    UiSystem::drawText(window, font,
                       lm.format("CHAR_CREATE_POINTS", {{"points", std::to_string(m_draft.remainingPoints())}}),
                       {rx + 20.0f, 254.0f}, 16, UiTheme::textGold(), true);

    const auto& sc = m_draft.abilities();
    const std::array<int, 6> vals = {sc.strength, sc.dexterity, sc.constitution, sc.intelligence, sc.wisdom, sc.charisma};

    for (int i = 0; i < 6; ++i) {
        const float y = 294.0f + static_cast<float>(i) * 42.0f;
        const bool foc = (m_focusIndex == 4 + i);
        UiSystem::drawText(window, font, lm.get(kAbilityKeys[static_cast<std::size_t>(i)]),
                           {rx + 24.0f, y + 6.0f}, 15, foc ? UiTheme::textGold() : UiTheme::textPrimary());
        const int val = vals[static_cast<std::size_t>(i)];
        const int mod = sc.getModifier(val);
        const std::string modStr = (mod >= 0 ? "+" : "") + std::to_string(mod);
        UiSystem::drawGaugeBar(window, font, {rx + 180.0f, y + 6.0f, 330.0f, 22.0f},
                               static_cast<float>(val) / 20.0f, static_cast<float>(val) / 20.0f,
                               UiTheme::borderGold(), UiTheme::borderGold(),
                               std::to_string(val) + " (" + modStr + ")");

        m_statMinusRects[static_cast<std::size_t>(i)] = {rx + 530.0f, y, 42.0f, 34.0f};
        m_statPlusRects[static_cast<std::size_t>(i)] = {rx + 584.0f, y, 42.0f, 34.0f};
        UiSystem::drawButton(window, assets, font, {
            m_statMinusRects[static_cast<std::size_t>(i)], "-", "", false,
            m_statMinusRects[static_cast<std::size_t>(i)].contains(mouse), true, false
        });
        UiSystem::drawButton(window, assets, font, {
            m_statPlusRects[static_cast<std::size_t>(i)], "+", "", false,
            m_statPlusRects[static_cast<std::size_t>(i)].contains(mouse), true, false
        });

        const int cost = m_draft.increaseCost(kAbilities[static_cast<std::size_t>(i)]);
        UiSystem::drawText(window, font, "Cost: " + std::to_string(cost),
                           {rx + 642.0f, y + 8.0f}, 13, UiTheme::textMuted());
    }

    // 하단 액션 버튼
    m_rerollRect = {rx, 588.0f, 220.0f, 48.0f};
    m_confirmRect = {rx + 250.0f, 588.0f, 280.0f, 48.0f};
    m_cancelRect = {rx + 560.0f, 588.0f, 220.0f, 48.0f};

    UiSystem::drawButton(window, assets, font, {m_rerollRect, lm.get("CHAR_CREATE_REROLL"), "R", m_focusIndex == 10, m_rerollRect.contains(mouse), true, false});
    UiSystem::drawButton(window, assets, font, {m_confirmRect, lm.get("CHAR_CREATE_CONFIRM"), "Enter", m_focusIndex == 11, m_confirmRect.contains(mouse), true, true});
    UiSystem::drawButton(window, assets, font, {m_cancelRect, lm.get("COMMON_CANCEL"), "Esc", m_focusIndex == 12, m_cancelRect.contains(mouse), true, false});

    if (!m_statusMessage.empty()) {
        UiSystem::drawText(window, font, m_statusMessage, {108.0f, 600.0f}, 14, UiTheme::hpCrimson(), true, 260.0f);
    }
}

} // namespace crawl
