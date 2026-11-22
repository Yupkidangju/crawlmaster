#include "controller/CharacterInfoState.hpp"
#include "core/AudioManager.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "model/CombatActionRules.hpp"
#include "model/ConcreteItems.hpp"
#include "model/Equipment.hpp"
#include "model/Skill.hpp"
#include "view/UiSystem.hpp"
#include "view/UiTheme.hpp"

#include <algorithm>
#include <array>

namespace crawl {

CharacterInfoState::CharacterInfoState(Game& game, int initialMemberIndex)
    : m_game(game), m_selectedMemberIdx(initialMemberIndex) {
    const int cnt = m_game.getParty().getMemberCount();
    if (cnt > 0) m_selectedMemberIdx = std::clamp(m_selectedMemberIdx, 0, cnt - 1);
}

void CharacterInfoState::equipOrUseInventoryItem(int invIndex) {
    auto& party = m_game.getParty();
    const auto& lm = LocalizationManager::getInstance();
    auto member = party.getMember(m_selectedMemberIdx);
    const auto& inv = party.getInventory();
    if (!member || invIndex < 0 || invIndex >= static_cast<int>(inv.size())) return;

    auto item = inv[static_cast<std::size_t>(invIndex)];
    if (auto eq = std::dynamic_pointer_cast<Equipment>(item)) {
        if (!member->canEquip(*eq)) {
            m_statusMessage = lm.get("CHAR_INFO_CANNOT_EQUIP");
            AudioManager::getInstance().playSfx("sfx_ui_error");
            return;
        }
        auto prev = member->getEquippedItem(eq->getSlot());
        if (member->equip(eq)) {
            party.removeItem(invIndex);
            if (prev) party.addItem(prev);
            party.saveToFile();
            AudioManager::getInstance().playSfx("sfx_ui_click");
            m_statusMessage = lm.format("CHAR_INFO_EQUIPPED", {{"item", eq->getDisplayName()}});
        }
        return;
    }

    if (auto cons = std::dynamic_pointer_cast<ConsumableItem>(item)) {
        if (cons->isOffensiveBomb() || !CombatActionRules::canUseConsumable(*cons, *member)) {
            m_statusMessage = lm.get("COMBAT_LOG_ITEM_NO_EFFECT");
            AudioManager::getInstance().playSfx("sfx_ui_error");
            return;
        }
        std::vector<std::string> logs;
        auto membersCopy = party.getMembers();
        cons->applyEffect(*member, membersCopy, logs);
        party.removeItem(invIndex);
        party.saveToFile();
        AudioManager::getInstance().playSfx("sfx_heal");
        m_statusMessage = logs.empty() ? lm.get("CHAR_INFO_USED") : logs.back();
    }
}

void CharacterInfoState::unequipSlot(EquipSlot slot) {
    auto& party = m_game.getParty();
    auto member = party.getMember(m_selectedMemberIdx);
    if (!member) return;
    if (auto cur = member->getEquippedItem(slot)) {
        member->unequip(slot);
        party.addItem(cur);
        party.saveToFile();
        AudioManager::getInstance().playSfx("sfx_ui_click");
        m_statusMessage = LocalizationManager::getInstance().format("CHAR_INFO_UNEQUIPPED", {{"item", cur->getDisplayName()}});
    }
}

void CharacterInfoState::handleInput(const sf::Event& event) {
    auto& party = m_game.getParty();
    const int mCount = party.getMemberCount();
    if (mCount <= 0) {
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) m_game.getStates().popState();
        return;
    }
    m_selectedMemberIdx = std::clamp(m_selectedMemberIdx, 0, mCount - 1);
    auto member = party.getMember(m_selectedMemberIdx);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        const auto pos = m_game.mapPixelToLogical({event.mouseButton.x, event.mouseButton.y});
        if (m_closeRect.contains(pos)) {
            AudioManager::getInstance().playSfx("sfx_ui_click");
            m_game.getStates().popState();
            return;
        }
        for (int i = 0; i < mCount; ++i) {
            if (m_memberTabRects[static_cast<std::size_t>(i)].contains(pos)) {
                m_selectedMemberIdx = i;
                AudioManager::getInstance().playSfx("sfx_ui_click");
                return;
            }
        }
        if (m_rowToggleRect.contains(pos)) {
            member->toggleRow();
            party.saveToFile();
            AudioManager::getInstance().playSfx("sfx_ui_click");
            return;
        }
        for (int s = 0; s < 4; ++s) {
            if (m_equipSlotRects[static_cast<std::size_t>(s)].contains(pos)) {
                unequipSlot(static_cast<EquipSlot>(s));
                return;
            }
        }
        for (int t = 0; t < 3; ++t) {
            if (m_rightTabRects[static_cast<std::size_t>(t)].contains(pos)) {
                m_rightTab = static_cast<RightTab>(t);
                AudioManager::getInstance().playSfx("sfx_ui_click");
                return;
            }
        }
        if (m_rightTab == RightTab::INVENTORY) {
            for (std::size_t i = 0; i < m_invRowRects.size(); ++i) {
                if (m_invRowRects[i].contains(pos)) {
                    m_selectedInvIdx = static_cast<int>(i);
                    equipOrUseInventoryItem(m_selectedInvIdx);
                    return;
                }
            }
        } else if (m_rightTab == RightTab::TALENTS) {
            if (member->canChooseSpecialization()) {
                const auto [sa, sb] = member->getAvailableSpecializations();
                if (m_specRects[0].contains(pos) && member->chooseSpecialization(sa)) {
                    party.saveToFile();
                    AudioManager::getInstance().playSfx("sfx_levelup");
                    m_statusMessage = member->getSpecializationName();
                    return;
                }
                if (m_specRects[1].contains(pos) && member->chooseSpecialization(sb)) {
                    party.saveToFile();
                    AudioManager::getInstance().playSfx("sfx_levelup");
                    m_statusMessage = member->getSpecializationName();
                    return;
                }
            }
            for (int n = 0; n < 4; ++n) {
                if (m_talentRects[static_cast<std::size_t>(n)].contains(pos)) {
                    if (member->investTalent(static_cast<TalentNode>(n))) {
                        party.saveToFile();
                        AudioManager::getInstance().playSfx("sfx_levelup");
                    } else {
                        AudioManager::getInstance().playSfx("sfx_ui_error");
                    }
                    return;
                }
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
    const auto key = event.key.code;
    if (key == sf::Keyboard::Escape || key == sf::Keyboard::C) {
        AudioManager::getInstance().playSfx("sfx_ui_click");
        m_game.getStates().popState();
    } else if (key == sf::Keyboard::Left) {
        m_selectedMemberIdx = (m_selectedMemberIdx + mCount - 1) % mCount;
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    } else if (key == sf::Keyboard::Right) {
        m_selectedMemberIdx = (m_selectedMemberIdx + 1) % mCount;
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    } else if (key >= sf::Keyboard::Num1 && key <= sf::Keyboard::Num4) {
        const int idx = key - sf::Keyboard::Num1;
        if (idx < mCount) m_selectedMemberIdx = idx;
    } else if (key == sf::Keyboard::Tab) {
        m_rightTab = static_cast<RightTab>((static_cast<int>(m_rightTab) + 1) % 3);
        AudioManager::getInstance().playSfx("sfx_ui_click");
    } else if (key == sf::Keyboard::F) {
        member->toggleRow();
        party.saveToFile();
        AudioManager::getInstance().playSfx("sfx_ui_click");
    } else if (key == sf::Keyboard::Up && m_rightTab == RightTab::INVENTORY) {
        m_selectedInvIdx = std::max(0, m_selectedInvIdx - 1);
    } else if (key == sf::Keyboard::Down && m_rightTab == RightTab::INVENTORY) {
        const int n = static_cast<int>(party.getInventory().size());
        if (n > 0) m_selectedInvIdx = std::min(n - 1, m_selectedInvIdx + 1);
    } else if (key == sf::Keyboard::Enter && m_rightTab == RightTab::INVENTORY) {
        equipOrUseInventoryItem(m_selectedInvIdx);
    }
}

void CharacterInfoState::update(sf::Time /*deltaTime*/) {}

void CharacterInfoState::draw(sf::RenderWindow& window) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    const auto& lm = LocalizationManager::getInstance();
    const auto& party = m_game.getParty();
    const auto mouse = m_game.getMouseLogicalPosition();

    sf::RectangleShape dim({Game::LOGICAL_WIDTH, Game::LOGICAL_HEIGHT});
    dim.setFillColor(sf::Color(5, 6, 10, 212));
    window.draw(dim);

    const sf::FloatRect frame(36.0f, 24.0f, 1208.0f, 672.0f);
    UiSystem::drawPanel(window, assets, frame, true, 244);

    // 상단 파티원 탭
    for (int i = 0; i < party.getMemberCount(); ++i) {
        const auto m = party.getMember(i);
        const sf::FloatRect r(56.0f + static_cast<float>(i) * 238.0f, 40.0f, 226.0f, 42.0f);
        m_memberTabRects[static_cast<std::size_t>(i)] = r;
        UiSystem::drawButton(window, assets, font, {
            r, m->getName() + " (Lv." + std::to_string(m->getLevel()) + ")", std::to_string(i + 1),
            m_selectedMemberIdx == i, r.contains(mouse), true, m_selectedMemberIdx == i
        });
    }
    m_closeRect = {1048.0f, 40.0f, 176.0f, 42.0f};
    UiSystem::drawButton(window, assets, font, {m_closeRect, lm.get("COMMON_BACK"), "Esc", false, m_closeRect.contains(mouse), true, false});

    auto member = party.getMember(m_selectedMemberIdx);
    if (!member) return;

    // 좌측: 초상화 + 신원 + 진형 토글 + 6대 능력치
    const sf::FloatRect leftP(56.0f, 96.0f, 340.0f, 580.0f);
    UiSystem::drawPanel(window, assets, leftP, false, 224);

    if (const sf::Texture* tex = assets.getPortraitTexture(member->getClassSlug())) {
        sf::Sprite sp(*tex);
        const auto sz = tex->getSize();
        sp.setScale(140.0f / static_cast<float>(sz.x), 140.0f / static_cast<float>(sz.y));
        sp.setPosition(leftP.left + 16.0f, leftP.top + 16.0f);
        window.draw(sp);
    }
    UiSystem::drawText(window, font, member->getName(), {leftP.left + 168.0f, leftP.top + 20.0f}, 18, UiTheme::textGold(), true, 160.0f);
    UiSystem::drawText(window, font, member->getClassString(), {leftP.left + 168.0f, leftP.top + 46.0f}, 14, UiTheme::textPrimary());
    UiSystem::drawText(window, font, member->getSpecializationName(), {leftP.left + 168.0f, leftP.top + 68.0f}, 13, UiTheme::cyanAccent());
    UiSystem::drawText(window, font, "Lv." + std::to_string(member->getLevel()) + " · AC " + std::to_string(member->getAc()),
                       {leftP.left + 168.0f, leftP.top + 92.0f}, 14, UiTheme::textPrimary());
    UiSystem::drawText(window, font, "XP " + std::to_string(member->getXp()) + " / " + std::to_string(member->getNextLevelXp()),
                       {leftP.left + 168.0f, leftP.top + 116.0f}, 12, UiTheme::textMuted());

    // 전열/후열 진형 전환 버튼
    m_rowToggleRect = {leftP.left + 16.0f, leftP.top + 170.0f, leftP.width - 32.0f, 38.0f};
    const std::string rowLbl = lm.get("CHAR_INFO_ROW") + ": " +
                               lm.get(member->getRow() == CombatRow::FRONT ? "ROW_FRONT" : "ROW_BACK");
    UiSystem::drawButton(window, assets, font, {m_rowToggleRect, rowLbl, "F", false, m_rowToggleRect.contains(mouse), true, false});

    // HP / SP 게이지
    const float hpR = static_cast<float>(member->getHp()) / static_cast<float>(std::max(1, member->getMaxHp()));
    UiSystem::drawGaugeBar(window, font, {leftP.left + 16.0f, leftP.top + 220.0f, leftP.width - 32.0f, 22.0f},
                           hpR, hpR, UiTheme::hpCrimson(), UiTheme::hpTrail(),
                           "HP " + std::to_string(member->getHp()) + " / " + std::to_string(member->getMaxHp()));
    if (member->getMaxSpellSlots() > 0) {
        const float spR = static_cast<float>(member->getSpellSlots()) / static_cast<float>(member->getMaxSpellSlots());
        UiSystem::drawGaugeBar(window, font, {leftP.left + 16.0f, leftP.top + 250.0f, leftP.width - 32.0f, 18.0f},
                               spR, spR, UiTheme::spAzure(), UiTheme::spAzure(),
                               "SP " + std::to_string(member->getSpellSlots()) + " / " + std::to_string(member->getMaxSpellSlots()));
    }

    // 6대 능력치
    const auto ab = member->getAbilities();
    const std::array<std::pair<const char*, int>, 6> stats = {{
        {"STAT_STR", ab.strength}, {"STAT_DEX", ab.dexterity}, {"STAT_CON", ab.constitution},
        {"STAT_INT", ab.intelligence}, {"STAT_WIS", ab.wisdom}, {"STAT_CHA", ab.charisma}
    }};
    for (int i = 0; i < 6; ++i) {
        const float y = leftP.top + 286.0f + static_cast<float>(i) * 34.0f;
        const int v = stats[static_cast<std::size_t>(i)].second;
        const int mod = ab.getModifier(v);
        UiSystem::drawText(window, font, lm.get(stats[static_cast<std::size_t>(i)].first), {leftP.left + 20.0f, y}, 14, UiTheme::textMuted());
        UiSystem::drawGaugeBar(window, font, {leftP.left + 126.0f, y, 196.0f, 20.0f},
                               static_cast<float>(v) / 22.0f, static_cast<float>(v) / 22.0f,
                               UiTheme::borderGold(), UiTheme::borderGold(),
                               std::to_string(v) + " (" + (mod >= 0 ? "+" : "") + std::to_string(mod) + ")");
    }
    UiSystem::drawText(window, font,
                       "Crit: " + std::to_string(member->getCritThreshold()) + "+  ·  Lifesteal: " +
                           std::to_string(member->getLifestealPct()) + "%",
                       {leftP.left + 20.0f, leftP.top + 506.0f}, 13, UiTheme::cyanAccent());
    if (!m_statusMessage.empty()) {
        UiSystem::drawText(window, font, m_statusMessage, {leftP.left + 20.0f, leftP.top + 542.0f}, 13, UiTheme::textGold(), true, leftP.width - 40.0f);
    }

    // 중앙: 4개 장비 슬롯 (클릭으로 장착 해제)
    const sf::FloatRect midP(408.0f, 96.0f, 326.0f, 580.0f);
    UiSystem::drawPanel(window, assets, midP, false, 224);
    UiSystem::drawText(window, font, lm.get("CHAR_INFO_EQUIPMENT"), {midP.left + 16.0f, midP.top + 14.0f}, 17, UiTheme::textGold());

    static constexpr std::array<const char*, 4> kSlotKeys = {"SLOT_WEAPON", "SLOT_ARMOR", "SLOT_SHIELD", "SLOT_ACCESSORY"};
    for (int s = 0; s < 4; ++s) {
        const sf::FloatRect r(midP.left + 14.0f, midP.top + 52.0f + static_cast<float>(s) * 124.0f, midP.width - 28.0f, 112.0f);
        m_equipSlotRects[static_cast<std::size_t>(s)] = r;
        const auto eq = member->getEquippedItem(static_cast<EquipSlot>(s));
        UiSystem::drawPanel(window, assets, r, r.contains(mouse), 232);
        UiSystem::drawText(window, font, lm.get(kSlotKeys[static_cast<std::size_t>(s)]),
                           {r.left + 12.0f, r.top + 10.0f}, 12, UiTheme::textMuted());
        if (eq) {
            UiSystem::drawText(window, font, eq->getDisplayName(), {r.left + 12.0f, r.top + 32.0f}, 15,
                               UiTheme::rarityColor(eq->getRarity()), true, r.width - 24.0f);
            std::string statLine;
            if (eq->getSlot() == EquipSlot::WEAPON) {
                statLine = std::to_string(eq->getDamageDiceCount()) + "d" + std::to_string(eq->getDamageDiceSides()) +
                           " +" + std::to_string(eq->getUpgradeDamageBonus()) + " [" +
                           UiTheme::elementShortCode(eq->getDamageType()) + "]";
            } else {
                statLine = "AC +" + std::to_string(eq->getAcBonus()) + " · HP +" + std::to_string(eq->getBonusHp());
            }
            UiSystem::drawText(window, font, statLine, {r.left + 12.0f, r.top + 58.0f}, 13, UiTheme::textPrimary());
            UiSystem::drawText(window, font, lm.get("CHAR_INFO_CLICK_UNEQUIP"), {r.left + 12.0f, r.top + 84.0f}, 11, UiTheme::textMuted());
        } else {
            UiSystem::drawText(window, font, lm.get("CHAR_INFO_EMPTY"), {r.left + 12.0f, r.top + 44.0f}, 14, UiTheme::textMuted());
        }
    }

    // 우측: 탭 패널 (인벤토리 / 특성·전직 / 스킬)
    const sf::FloatRect rightP(746.0f, 96.0f, 478.0f, 580.0f);
    UiSystem::drawPanel(window, assets, rightP, false, 224);

    static constexpr std::array<const char*, 3> kTabs = {"TAB_INVENTORY", "TAB_TALENTS", "TAB_SKILLS"};
    for (int t = 0; t < 3; ++t) {
        const sf::FloatRect tr(rightP.left + 12.0f + static_cast<float>(t) * 154.0f, rightP.top + 12.0f, 148.0f, 36.0f);
        m_rightTabRects[static_cast<std::size_t>(t)] = tr;
        std::string lbl = lm.get(kTabs[static_cast<std::size_t>(t)]);
        if (t == 1 && (member->getTalentPoints() > 0 || member->canChooseSpecialization())) lbl += " (!)";
        UiSystem::drawButton(window, assets, font, {
            tr, lbl, "", static_cast<int>(m_rightTab) == t, tr.contains(mouse), true, static_cast<int>(m_rightTab) == t
        });
    }

    m_invRowRects.clear();
    std::string tipTitle;
    std::vector<std::string> tipLines;
    sf::Color tipColor = UiTheme::textGold();

    if (m_rightTab == RightTab::INVENTORY) {
        const auto& inv = party.getInventory();
        if (inv.empty()) {
            UiSystem::drawText(window, font, lm.get("CHAR_INFO_BAG_EMPTY"), {rightP.left + 20.0f, rightP.top + 74.0f}, 14, UiTheme::textMuted());
        } else {
            const int maxRows = 12;
            const int start = std::max(0, m_selectedInvIdx - maxRows + 1);
            for (int i = start; i < std::min(static_cast<int>(inv.size()), start + maxRows); ++i) {
                const auto& item = inv[static_cast<std::size_t>(i)];
                const sf::FloatRect r(rightP.left + 14.0f, rightP.top + 60.0f + static_cast<float>(i - start) * 41.0f,
                                      rightP.width - 28.0f, 36.0f);
                const bool hov = r.contains(mouse);
                sf::RectangleShape bg({r.width, r.height});
                bg.setPosition(r.left, r.top);
                bg.setFillColor((hov || m_selectedInvIdx == i) ? UiTheme::panelHover() : UiTheme::panelAlt());
                bg.setOutlineThickness(1.0f);
                bg.setOutlineColor(hov ? UiTheme::borderGold() : UiTheme::borderBronze());
                window.draw(bg);

                std::string disp = item->getName();
                if (auto eq = std::dynamic_pointer_cast<Equipment>(item)) disp = eq->getDisplayName();
                UiSystem::drawText(window, font, disp, {r.left + 10.0f, r.top + 8.0f}, 14,
                                   UiTheme::rarityColor(item->getRarity()), true, 320.0f);
                UiSystem::drawText(window, font, item->isEquipment() ? lm.get("CHAR_INFO_EQUIP") : lm.get("CHAR_INFO_USE"),
                                   {r.left + r.width - 84.0f, r.top + 9.0f}, 12, UiTheme::textGold());

                while (static_cast<int>(m_invRowRects.size()) <= i) m_invRowRects.emplace_back();
                m_invRowRects[static_cast<std::size_t>(i)] = r;

                if (hov) {
                    tipTitle = disp;
                    tipColor = UiTheme::rarityColor(item->getRarity());
                    tipLines = {item->getDescription()};
                    if (auto eq = std::dynamic_pointer_cast<Equipment>(item)) {
                        const auto cur = member->getEquippedItem(eq->getSlot());
                        const int curAc = cur ? cur->getAcBonus() : 0;
                        const int acDiff = eq->getAcBonus() - curAc;
                        if (eq->getSlot() == EquipSlot::WEAPON) {
                            tipLines.push_back("DMG: " + std::to_string(eq->getDamageDiceCount()) + "d" +
                                               std::to_string(eq->getDamageDiceSides()) + " +" +
                                               std::to_string(eq->getUpgradeDamageBonus()));
                        } else {
                            tipLines.push_back("AC: " + std::to_string(eq->getAcBonus()) + " (" +
                                               (acDiff >= 0 ? "+" : "") + std::to_string(acDiff) + ")");
                        }
                    }
                }
            }
        }
    } else if (m_rightTab == RightTab::TALENTS) {
        UiSystem::drawText(window, font,
                           lm.format("TALENT_POINTS_LABEL", {{"points", std::to_string(member->getTalentPoints())}}),
                           {rightP.left + 18.0f, rightP.top + 62.0f}, 16, UiTheme::textGold());

        // 전직 선택 영역 (Lv 3+)
        if (member->canChooseSpecialization()) {
            UiSystem::drawText(window, font, lm.get("SPEC_CHOOSE_PROMPT"), {rightP.left + 18.0f, rightP.top + 94.0f},
                               14, UiTheme::statPos());
            const auto [sa, sb] = member->getAvailableSpecializations();
            auto specKey = [](Specialization s) -> const char* {
                switch (s) {
                    case Specialization::VANGUARD:     return "SPEC_VANGUARD";
                    case Specialization::BERSERKER:    return "SPEC_BERSERKER";
                    case Specialization::PYROMANCER:   return "SPEC_PYROMANCER";
                    case Specialization::CHRONOMANCER: return "SPEC_CHRONOMANCER";
                    case Specialization::ASSASSIN:     return "SPEC_ASSASSIN";
                    case Specialization::TRAPSMITH:    return "SPEC_TRAPSMITH";
                    case Specialization::PALADIN:      return "SPEC_PALADIN";
                    case Specialization::ORACLE:       return "SPEC_ORACLE";
                    default: return "SPEC_NONE";
                }
            };
            m_specRects[0] = {rightP.left + 16.0f, rightP.top + 120.0f, 218.0f, 48.0f};
            m_specRects[1] = {rightP.left + 244.0f, rightP.top + 120.0f, 218.0f, 48.0f};
            UiSystem::drawButton(window, assets, font, {m_specRects[0], lm.get(specKey(sa)), "", false, m_specRects[0].contains(mouse), true, true});
            UiSystem::drawButton(window, assets, font, {m_specRects[1], lm.get(specKey(sb)), "", false, m_specRects[1].contains(mouse), true, true});
        } else {
            UiSystem::drawText(window, font,
                               lm.get("SPEC_LABEL") + ": " + member->getSpecializationName() +
                                   (member->getLevel() < Character::SPEC_UNLOCK_LEVEL ? " (Lv.3+)" : ""),
                               {rightP.left + 18.0f, rightP.top + 104.0f}, 14, UiTheme::cyanAccent());
        }

        static constexpr std::array<const char*, 4> kTalName = {"TALENT_VITALITY", "TALENT_MASTERY", "TALENT_TACTICS", "TALENT_APEX"};
        static constexpr std::array<const char*, 4> kTalDesc = {"TALENT_VITALITY_DESC", "TALENT_MASTERY_DESC", "TALENT_TACTICS_DESC", "TALENT_APEX_DESC"};
        for (int n = 0; n < 4; ++n) {
            const sf::FloatRect r(rightP.left + 16.0f, rightP.top + 190.0f + static_cast<float>(n) * 90.0f,
                                  rightP.width - 32.0f, 78.0f);
            m_talentRects[static_cast<std::size_t>(n)] = r;
            const int rank = member->getTalentRank(static_cast<TalentNode>(n));
            const bool canUp = member->getTalentPoints() > 0 && rank < Character::MAX_TALENT_RANK;
            UiSystem::drawPanel(window, assets, r, canUp && r.contains(mouse), 232);
            UiSystem::drawText(window, font,
                               lm.get(kTalName[static_cast<std::size_t>(n)]) + "  [" +
                                   std::to_string(rank) + "/" + std::to_string(Character::MAX_TALENT_RANK) + "]",
                               {r.left + 14.0f, r.top + 12.0f}, 15,
                               canUp ? UiTheme::textGold() : UiTheme::textPrimary());
            UiSystem::drawText(window, font, lm.get(kTalDesc[static_cast<std::size_t>(n)]),
                               {r.left + 14.0f, r.top + 40.0f}, 12, UiTheme::textMuted(), false, r.width - 28.0f);
        }
    } else {
        const auto& skills = member->getSkills();
        for (std::size_t i = 0; i < skills.size(); ++i) {
            const auto& sk = skills[i];
            const sf::FloatRect r(rightP.left + 16.0f, rightP.top + 62.0f + static_cast<float>(i) * 78.0f,
                                  rightP.width - 32.0f, 68.0f);
            UiSystem::drawPanel(window, assets, r, false, 226);
            UiSystem::drawText(window, font, sk->getName() + (sk->isSpell() ? " [SP]" : ""),
                               {r.left + 14.0f, r.top + 10.0f}, 15, UiTheme::textGold(), true, 320.0f);
            UiSystem::drawBadge(window, font, {r.left + r.width - 62.0f, r.top + 10.0f},
                                UiTheme::elementShortCode(sk->getDamageType()),
                                sf::Color(22, 24, 34), UiTheme::elementColor(sk->getDamageType()), 10);
            UiSystem::drawText(window, font, sk->getDescription(),
                               {r.left + 14.0f, r.top + 36.0f}, 12, UiTheme::textMuted(), false, r.width - 28.0f);
        }
    }

    if (!tipTitle.empty()) {
        UiSystem::drawTooltip(window, assets, font, mouse, tipTitle, tipLines, tipColor);
    }
}

} // namespace crawl
