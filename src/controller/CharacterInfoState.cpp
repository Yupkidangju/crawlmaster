// [v0.6.0] CharacterInfoState.cpp 신규 작성
// 캐릭터 스탯 정보, 능력치 및 장착 장비 현황을 확인하고 공용 인벤토리의 아이템을 소모 및 장착하는 UI 및 이벤트 제어 클래스 구현.

#include "controller/CharacterInfoState.hpp"
#include "core/Game.hpp"
#include "model/Party.hpp"
#include "model/Character.hpp"
#include "model/ConcreteItems.hpp"
#include "model/CombatActionRules.hpp"
#include "core/LocalizationManager.hpp"
#include <random>
#include <algorithm>
#include <iostream>

namespace crawl {
namespace {

std::string genderLabel(Gender gender, const LocalizationManager& localization) {
    switch (gender) {
        case Gender::MALE: return localization.get("GENDER_MALE");
        case Gender::FEMALE: return localization.get("GENDER_FEMALE");
        case Gender::NON_BINARY: return localization.get("GENDER_NON_BINARY");
        case Gender::UNSPECIFIED: return localization.get("GENDER_UNSPECIFIED");
    }
    return localization.get("GENDER_UNSPECIFIED");
}

const char* traitKey(CharacterClass characterClass) {
    switch (characterClass) {
        case CharacterClass::WARRIOR: return "TRAIT_WARRIOR";
        case CharacterClass::MAGE: return "TRAIT_MAGE";
        case CharacterClass::ROGUE: return "TRAIT_ROGUE";
        case CharacterClass::CLERIC: return "TRAIT_CLERIC";
    }
    return "";
}

std::string ageLabel(const Character& character, const LocalizationManager& localization) {
    return character.getAge() == 0 ? localization.get("GENDER_UNSPECIFIED")
                                   : std::to_string(character.getAge());
}

} // namespace

CharacterInfoState::CharacterInfoState(Game& game, bool persistChanges)
    : m_game(game), m_persistChanges(persistChanges), m_focusArea(1), m_selectedCharIndex(0),
      m_equipmentSlotIndex(0), m_inventoryIndex(0) {
    m_statusMsg = LocalizationManager::getInstance().get("CHAR_INFO_STATUS_ACTIVE");
}

void CharacterInfoState::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        auto& party = m_game.getParty();
        int memberCount = party.getMemberCount();
        const int invCount = static_cast<int>(party.getInventory().size());

        switch (event.key.code) {
            case sf::Keyboard::Escape:
            case sf::Keyboard::I:
            case sf::Keyboard::C:
                // 이전 상태(마을 혹은 던전)로 안전하게 복귀 (popState 호출)
                m_game.getStates().popState();
                break;

            case sf::Keyboard::Num1:
                if (memberCount >= 1) m_selectedCharIndex = 0;
                break;
            case sf::Keyboard::Num2:
                if (memberCount >= 2) m_selectedCharIndex = 1;
                break;
            case sf::Keyboard::Num3:
                if (memberCount >= 3) m_selectedCharIndex = 2;
                break;
            case sf::Keyboard::Num4:
                if (memberCount >= 4) m_selectedCharIndex = 3;
                break;

            case sf::Keyboard::Tab:
            case sf::Keyboard::Left:
            case sf::Keyboard::Right:
                // 포커스 전환 (0: 캐릭터 장비 영역, 1: 인벤토리 목록 영역)
                m_focusArea = (m_focusArea == 0) ? 1 : 0;
                break;

            case sf::Keyboard::Up:
                if (m_focusArea == 0) {
                    m_equipmentSlotIndex = (m_equipmentSlotIndex + 2) % 3; // 0, 1, 2 순환
                } else {
                    if (invCount > 0) {
                        m_inventoryIndex = (m_inventoryIndex + invCount - 1) % invCount;
                    }
                }
                break;

            case sf::Keyboard::Down:
                if (m_focusArea == 0) {
                    m_equipmentSlotIndex = (m_equipmentSlotIndex + 1) % 3;
                } else {
                    if (invCount > 0) {
                        m_inventoryIndex = (m_inventoryIndex + 1) % invCount;
                    }
                }
                break;

            case sf::Keyboard::Enter:
                if (m_focusArea == 0) {
                    unequipSelectedSlot();
                } else {
                    if (invCount > 0 && m_inventoryIndex < invCount) {
                        auto item = party.getInventory()[m_inventoryIndex];
                        if (item->isEquipment()) {
                            equipSelectedItem();
                        } else {
                            useSelectedConsumable();
                        }
                    }
                }
                break;

            default:
                break;
        }
    }
}

void CharacterInfoState::update(sf::Time /*deltaTime*/) {
    // 인벤토리 상태는 실시간 물리/논리 애니메이션 업데이트 없음
}

void CharacterInfoState::draw(sf::RenderWindow& window) {
    sf::Color neonGreen = sf::Color(51, 255, 51);
    sf::Color brightGreen = sf::Color(102, 255, 102);
    sf::Color amber = sf::Color(255, 176, 0);
    sf::Color red = sf::Color(255, 51, 51);
    sf::Color mutedColor = sf::Color(110, 155, 110);

    if (LocalizationManager::getInstance().getTextScale() > 125) {
        drawLargeTextLayout(window);
        return;
    }

    // 1. 전체 화면 상단 타이틀
    drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_TITLE"), 512.0f, 30.0f, 20, brightGreen, true);

    auto& party = m_game.getParty();
    auto members = party.getMembers();
    int memberCount = party.getMemberCount();

    // 2. 상단 파티원 요약 표시
    std::string summaryStr = "";
    for (int i = 0; i < memberCount; ++i) {
        std::string mark = (i == m_selectedCharIndex) ? " [*]" : "    ";
        summaryStr += std::to_string(i + 1) + "." + members[i]->getName() + mark + "  ";
    }
    drawText(window, summaryStr, 512.0f, 65.0f, 12, brightGreen, true);

    // 3. 좌측 캐릭터 상세 프레임 (x: 40, y: 100, w: 450, h: 520)
    sf::Color leftFrameColor = (m_focusArea == 0) ? neonGreen : mutedColor;
    drawBox(window, 40.0f, 100.0f, 450.0f, 520.0f, leftFrameColor, 2.0f);
    drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_MEMBER_DETAILS"), 265.0f, 115.0f, 14, leftFrameColor, true);

    if (m_selectedCharIndex < memberCount) {
        auto caster = members[m_selectedCharIndex];
        
        // 기본 인적 정보
        drawText(window, LocalizationManager::getInstance().format("CHAR_INFO_NAME_LINE", {
            {"name", caster->getName()}}), 60.0f, 150.0f, 12, brightGreen);

        drawText(window, LocalizationManager::getInstance().format("CHAR_INFO_IDENTITY_LINE", {
            {"age", ageLabel(*caster, LocalizationManager::getInstance())},
            {"gender", genderLabel(caster->getGender(), LocalizationManager::getInstance())}}),
            60.0f, 175.0f, 12, brightGreen);
        
        // 클래스 정보 다국어 치환
        std::string classStr = "";
        switch (caster->getClass()) {
            case CharacterClass::WARRIOR: classStr = LocalizationManager::getInstance().get("CLASS_WARRIOR"); break;
            case CharacterClass::MAGE:    classStr = LocalizationManager::getInstance().get("CLASS_MAGE"); break;
            case CharacterClass::ROGUE:   classStr = LocalizationManager::getInstance().get("CLASS_ROGUE"); break;
            case CharacterClass::CLERIC:  classStr = LocalizationManager::getInstance().get("CLASS_CLERIC"); break;
        }
        drawText(window, LocalizationManager::getInstance().format("CHAR_INFO_CLASS_LINE", {
            {"class", classStr}, {"level", std::to_string(caster->getLevel())}}),
            60.0f, 200.0f, 12, brightGreen);
        drawText(window, LocalizationManager::getInstance().get("CREATE_TRAIT") + ": " +
                 LocalizationManager::getInstance().get(traitKey(caster->getClass())),
                 60.0f, 225.0f, 11, amber);
        
        std::string hpStr = LocalizationManager::getInstance().get("CHAR_INFO_HP") + ": " + std::to_string(caster->getHp()) + " / " + std::to_string(caster->getMaxHp());
        sf::Color hpColor = caster->isDead() ? red : (caster->getHp() < caster->getMaxHp() / 3 ? amber : brightGreen);
        drawText(window, hpStr, 60.0f, 250.0f, 12, hpColor);
        drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_XP") + ": " + std::to_string(caster->getXp()) + " / " + ((caster->getLevel() == 1) ? "300" : "900"), 60.0f, 275.0f, 12, brightGreen);
        drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_AC") + ": " + std::to_string(caster->getAc()), 60.0f, 300.0f, 12, brightGreen);
        drawText(window, statusSummary(*caster), 60.0f, 320.0f, 12,
                 caster->isDead() ? red : amber);

        if (caster->getClass() == CharacterClass::MAGE || caster->getClass() == CharacterClass::CLERIC) {
            drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_SPELLS") + ": " + std::to_string(caster->getSpellSlots()) + " / " + std::to_string(caster->getMaxSpellSlots()), 250.0f, 300.0f, 12, brightGreen);
        }

        // D&D 6대 능력치 및 보정치 출력
        drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_ABILITIES"), 60.0f, 340.0f, 12, leftFrameColor);
        const auto& ab = caster->getAbilities();
        auto getModStr = [&](int score) {
            int mod = ab.getModifier(score);
            return (mod >= 0 ? "+" : "") + std::to_string(mod);
        };
        drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_STR") + ": " + std::to_string(ab.strength) + " (" + getModStr(ab.strength) + ")", 60.0f, 365.0f, 12, brightGreen);
        drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_DEX") + ": " + std::to_string(ab.dexterity) + " (" + getModStr(ab.dexterity) + ")", 60.0f, 390.0f, 12, brightGreen);
        drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_CON") + ": " + std::to_string(ab.constitution) + " (" + getModStr(ab.constitution) + ")", 60.0f, 415.0f, 12, brightGreen);
        drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_INT") + ": " + std::to_string(ab.intelligence) + " (" + getModStr(ab.intelligence) + ")", 250.0f, 365.0f, 12, brightGreen);
        drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_WIS") + ": " + std::to_string(ab.wisdom) + " (" + getModStr(ab.wisdom) + ")", 250.0f, 390.0f, 12, brightGreen);
        drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_CHA") + ": " + std::to_string(ab.charisma) + " (" + getModStr(ab.charisma) + ")", 250.0f, 415.0f, 12, brightGreen);

        // 장착 장비 슬롯 정보
        drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_EQUIP_SLOTS_GUIDE"), 60.0f, 450.0f, 12, leftFrameColor);

        auto drawSlot = [&](const std::string& label, EquipSlot slot, float yPos, int idx) {
            auto item = caster->getEquippedItem(slot);
            std::string itemName = item ? item->getName() : LocalizationManager::getInstance().get("CHAR_INFO_NONE");
            std::string cursor = (m_focusArea == 0 && m_equipmentSlotIndex == idx) ? "> " : "  ";
            sf::Color slotColor = (m_focusArea == 0 && m_equipmentSlotIndex == idx) ? brightGreen : (item ? brightGreen : mutedColor);
            drawText(window, cursor + label + ": " + itemName, 60.0f, yPos, 12, slotColor);
        };

        drawSlot(LocalizationManager::getInstance().get("SLOT_WEAPON"), EquipSlot::WEAPON, 475.0f, 0);
        drawSlot(LocalizationManager::getInstance().get("SLOT_ARMOR"), EquipSlot::ARMOR, 500.0f, 1);
        drawSlot(LocalizationManager::getInstance().get("SLOT_SHIELD"), EquipSlot::SHIELD, 525.0f, 2);
    } else {
        drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_NO_MEMBERS"), 265.0f, 300.0f, 12, brightGreen, true);
    }

    // 4. 우측 공용 인벤토리 프레임 (x: 534, y: 100, w: 450, h: 520)
    sf::Color rightFrameColor = (m_focusArea == 1) ? neonGreen : mutedColor;
    drawBox(window, 534.0f, 100.0f, 450.0f, 520.0f, rightFrameColor, 2.0f);
    drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_PARTY_INVENTORY"), 759.0f, 115.0f, 14, rightFrameColor, true);

    drawText(window, LocalizationManager::getInstance().get("TOWN_GOLD") + ": " + std::to_string(party.getGold()) + " G", 554.0f, 150.0f, 12, amber);

    const auto& inv = party.getInventory();
    const int invSize = static_cast<int>(inv.size());

    float itemStartY = 180.0f;
    float itemSpacing = 22.0f;
    int maxDisplayItems = 10;
    int scrollOffset = 0;
    if (m_inventoryIndex >= maxDisplayItems) {
        scrollOffset = m_inventoryIndex - maxDisplayItems + 1;
    }

    if (invSize > 0) {
        for (int i = 0; i < maxDisplayItems; ++i) {
            int targetIdx = i + scrollOffset;
            if (targetIdx >= invSize) break;

            auto item = inv[targetIdx];
            std::string cursor = (m_focusArea == 1 && m_inventoryIndex == targetIdx) ? "> " : "  ";
            sf::Color itemColor = (m_focusArea == 1 && m_inventoryIndex == targetIdx) ? brightGreen : brightGreen;

            std::string itemLabel = cursor + item->getName();
            if (item->isEquipment()) {
                itemLabel += " (" + LocalizationManager::getInstance().get("CHAR_INFO_TYPE_EQUIP") + ")";
            }
            drawText(window, itemLabel, 554.0f, itemStartY + i * itemSpacing, 12, itemColor);
        }

        // 가방 내 선택된 아이템의 설명 상자
        drawBox(window, 554.0f, 430.0f, 410.0f, 80.0f, rightFrameColor, 1.0f);
        if (m_inventoryIndex < invSize) {
            auto selItem = inv[m_inventoryIndex];
            drawText(window, "[ " + selItem->getName() + " ]", 564.0f, 440.0f, 11, amber);
            drawText(window, selItem->getDescription(), 564.0f, 465.0f, 11, brightGreen);
            std::string typeVal = selItem->isEquipment() ? LocalizationManager::getInstance().get("CHAR_INFO_TYPE_EQUIP") : LocalizationManager::getInstance().get("CHAR_INFO_TYPE_POTION");
            drawText(window, typeVal + " | " + LocalizationManager::getInstance().get("CHAR_INFO_PRICE") + ": " + std::to_string(selItem->getGoldValue()) + " G", 564.0f, 490.0f, 10, mutedColor);
        }
    } else {
        drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_EMPTY_BAG"), 759.0f, 300.0f, 12, mutedColor, true);
    }

    // 5. 상태 알림 메시지 출력
    drawText(window, m_statusMsg, 50.0f, 635.0f, 12, amber);

    // 6. 하단 가이드바 드로우
    drawBox(window, 40.0f, 665.0f, 944.0f, 50.0f, neonGreen, 1.5f);
    drawText(window, LocalizationManager::getInstance().get("CHAR_INFO_GUIDE_BAR"), 512.0f, 690.0f, 12, brightGreen, true);
}

void CharacterInfoState::drawLargeTextLayout(sf::RenderWindow& window) {
    auto& localization = LocalizationManager::getInstance();
    auto& party = m_game.getParty();
    const auto members = party.getMembers();
    const sf::Color neonGreen(51, 255, 51);
    const sf::Color brightGreen(102, 255, 102);
    const sf::Color amber(255, 176, 0);
    const sf::Color muted(170, 210, 170);

    drawText(window, localization.get("CHAR_INFO_TITLE"), 512.0f, 22.0f, 20, brightGreen, true);
    std::string summary;
    for (int index = 0; index < party.getMemberCount(); ++index) {
        summary += std::to_string(index + 1) + "." + members[index]->getName();
        if (index == m_selectedCharIndex) summary += "[*]";
        summary += "  ";
    }
    drawText(window, summary, 512.0f, 70.0f, 14, brightGreen, true);
    drawBox(window, 40.0f, 110.0f, 944.0f, 535.0f, neonGreen, 2.0f);

    if (m_focusArea == 0) {
        drawText(window, localization.get("CHAR_INFO_MEMBER_DETAILS"), 512.0f, 125.0f, 14, neonGreen, true);
        if (m_selectedCharIndex < party.getMemberCount()) {
            const auto character = members[m_selectedCharIndex];
            std::string className;
            switch (character->getClass()) {
                case CharacterClass::WARRIOR: className = localization.get("CLASS_WARRIOR"); break;
                case CharacterClass::MAGE: className = localization.get("CLASS_MAGE"); break;
                case CharacterClass::ROGUE: className = localization.get("CLASS_ROGUE"); break;
                case CharacterClass::CLERIC: className = localization.get("CLASS_CLERIC"); break;
            }
            float y = 170.0f;
            const float step = 32.0f;
            auto line = [&](const std::string& text, sf::Color color) {
                drawText(window, text, 70.0f, y, 14, color);
                y += step;
            };
            line(localization.format("CHAR_INFO_NAME_LINE", {{"name", character->getName()}}), brightGreen);
            line(localization.format("CHAR_INFO_IDENTITY_LINE", {
                {"age", ageLabel(*character, localization)},
                {"gender", genderLabel(character->getGender(), localization)}}), brightGreen);
            line(localization.format("CHAR_INFO_CLASS_LINE", {
                {"class", className}, {"level", std::to_string(character->getLevel())}}), brightGreen);
            line(localization.get("CREATE_TRAIT") + ": " +
                 localization.get(traitKey(character->getClass())), amber);
            line(localization.get("CHAR_INFO_HP") + ": " + std::to_string(character->getHp()) +
                 "/" + std::to_string(character->getMaxHp()), brightGreen);
            line(localization.get("CHAR_INFO_XP") + ": " + std::to_string(character->getXp()) +
                 "/" + (character->getLevel() == 1 ? "300" : "900"), brightGreen);
            line(localization.get("CHAR_INFO_AC") + ": " + std::to_string(character->getAc()), brightGreen);
            line(statusSummary(*character), character->isDead() ? sf::Color(255, 51, 51) : amber);
            const auto abilities = character->getAbilities();
            line("STR " + std::to_string(abilities.strength) + " | DEX " +
                 std::to_string(abilities.dexterity) + " | CON " + std::to_string(abilities.constitution), muted);
            line("INT " + std::to_string(abilities.intelligence) + " | WIS " +
                 std::to_string(abilities.wisdom) + " | CHA " + std::to_string(abilities.charisma), muted);
            auto equipmentLine = [&](const std::string& label, EquipSlot slot, int index) {
                const auto item = character->getEquippedItem(slot);
                const std::string itemName = item ? item->getName() : localization.get("CHAR_INFO_NONE");
                const std::string cursor = m_equipmentSlotIndex == index ? "> " : "  ";
                line(cursor + label + ": " + itemName, brightGreen);
            };
            equipmentLine(localization.get("SLOT_WEAPON"), EquipSlot::WEAPON, 0);
            equipmentLine(localization.get("SLOT_ARMOR"), EquipSlot::ARMOR, 1);
            equipmentLine(localization.get("SLOT_SHIELD"), EquipSlot::SHIELD, 2);
        } else {
            drawText(window, localization.get("CHAR_INFO_NO_MEMBERS"), 512.0f, 300.0f, 14, brightGreen, true);
        }
    } else {
        drawText(window, localization.get("CHAR_INFO_PARTY_INVENTORY"), 512.0f, 125.0f, 14, neonGreen, true);
        drawText(window, localization.get("TOWN_GOLD") + ": " + std::to_string(party.getGold()) + " G",
                 70.0f, 170.0f, 14, amber);
        const auto& inventory = party.getInventory();
        const int maxItems = 8;
        const int scroll = m_inventoryIndex >= maxItems ? m_inventoryIndex - maxItems + 1 : 0;
        for (int row = 0; row < maxItems; ++row) {
            const int index = scroll + row;
            if (index >= static_cast<int>(inventory.size())) break;
            const std::string cursor = index == m_inventoryIndex ? "> " : "  ";
            drawText(window, cursor + inventory[static_cast<std::size_t>(index)]->getName(),
                     70.0f, 215.0f + row * 43.0f, 14, brightGreen);
        }
        if (!inventory.empty() && m_inventoryIndex < static_cast<int>(inventory.size())) {
            const auto item = inventory[static_cast<std::size_t>(m_inventoryIndex)];
            drawText(window, item->getName() + " | " + localization.get("CHAR_INFO_PRICE") + ": " +
                     std::to_string(item->getGoldValue()) + " G", 70.0f, 585.0f, 14, amber);
        }
    }

    if (!m_statusMsg.empty()) drawText(window, m_statusMsg, 50.0f, 650.0f, 14, amber);
    drawBox(window, 40.0f, 690.0f, 944.0f, 55.0f, neonGreen, 1.5f);
    drawText(window, localization.get("CHAR_INFO_GUIDE_LARGE"), 512.0f, 710.0f, 14, brightGreen, true);
}

std::string CharacterInfoState::statusSummary(const Character& character) const {
    auto& localization = LocalizationManager::getInstance();
    std::vector<std::string> states;
    if (character.isDead()) states.push_back(localization.get("STATUS_DEAD"));
    if (character.getPoisonTurns() > 0) {
        states.push_back(localization.get("STATUS_POISON") + ":" + std::to_string(character.getPoisonTurns()));
    }
    if (character.getParalysisTurns() > 0) {
        states.push_back(localization.get("STATUS_PARALYSIS") + ":" + std::to_string(character.getParalysisTurns()));
    }
    if (character.getStrBuffTurns() > 0) {
        states.push_back(localization.get("STATUS_STRENGTH") + ":+" +
                         std::to_string(character.getStrBuffAmount()) + "/" +
                         std::to_string(character.getStrBuffTurns()));
    }
    if (character.getDexBuffTurns() > 0) {
        states.push_back(localization.get("STATUS_DEXTERITY") + ":+" +
                         std::to_string(character.getDexBuffAmount()) + "/" +
                         std::to_string(character.getDexBuffTurns()));
    }
    if (character.getBlessTurns() > 0) {
        states.push_back(localization.get("STATUS_BLESS") + ":" + std::to_string(character.getBlessTurns()));
    }
    std::string summary = states.empty() ? localization.get("CHAR_INFO_HEALTHY") : states.front();
    for (std::size_t index = 1; index < states.size(); ++index) summary += ", " + states[index];
    return localization.get("CHAR_INFO_CONDITIONS") + ": " + summary;
}

void CharacterInfoState::useSelectedConsumable() {
    auto& party = m_game.getParty();
    const auto& inv = party.getInventory();
    if (m_inventoryIndex >= static_cast<int>(inv.size())) return;

    auto item = inv[m_inventoryIndex];
    if (item->isEquipment()) return;

    auto members = party.getMembers();
    if (m_selectedCharIndex >= static_cast<int>(members.size())) return;
    auto caster = members[m_selectedCharIndex];

    if (caster->isDead()) {
        m_statusMsg = LocalizationManager::getInstance().format("CHAR_MSG_DEAD_CANNOT_USE", {{"name", caster->getName()}});
        return;
    }

    std::string itemId = item->getId();

    // 1. 비전투 버프 물약 사용 차단
    if (itemId == "pot_strength" || itemId == "pot_dexterity") {
        m_statusMsg = LocalizationManager::getInstance().get("CHAR_MSG_COMBAT_ONLY_ITEM");
        return;
    }

    // 2. 마나 물약 캐스터 전용 및 슬롯 상태 확인
    if (itemId == "pot_mana") {
        if (caster->getClass() != CharacterClass::MAGE && caster->getClass() != CharacterClass::CLERIC) {
            m_statusMsg = LocalizationManager::getInstance().get("CHAR_MSG_INVALID_MANA_CLASS");
            return;
        }
        if (caster->getSpellSlots() >= caster->getMaxSpellSlots()) {
            m_statusMsg = LocalizationManager::getInstance().format("ITEM_LOG_SLOT_FULL", {{"target", caster->getName()}});
            return;
        }
    }

    // 3. 해독 스크롤 독 상태 확인
    if (itemId == "scr_cure") {
        if (caster->getPoisonTurns() <= 0) {
            m_statusMsg = LocalizationManager::getInstance().format("ITEM_LOG_NOT_POISONED", {{"target", caster->getName()}});
            return;
        }
    }

    // 다형성 기반 효과 적용
    auto consumable = std::dynamic_pointer_cast<ConsumableItem>(item);
    if (!consumable) return;
    if (!CombatActionRules::canUseConsumable(*consumable, *caster)) {
        m_statusMsg = LocalizationManager::getInstance().get("CHAR_MSG_ITEM_NO_EFFECT");
        return;
    }
    const PartyCheckpoint checkpoint = party.captureCheckpoint();

    std::vector<std::string> useLogs;
    std::vector<std::shared_ptr<Character>> partyList;
    for (int i = 0; i < party.getMemberCount(); ++i) {
        partyList.push_back(party.getMember(i));
    }

    consumable->applyEffect(*caster, partyList, useLogs);

    // 아이템 제거
    party.removeItem(m_inventoryIndex);

    if (m_persistChanges) {
        const auto saveResult = party.saveToFile();
        if (!saveResult.durabilityConfirmed()) {
            m_statusMsg = LocalizationManager::getInstance().get("TOWN_MSG_DURABILITY_UNKNOWN");
            return;
        }
        if (!saveResult) {
            party.restoreCheckpoint(checkpoint);
            m_statusMsg = LocalizationManager::getInstance().get("CHAR_MSG_ITEM_ROLLBACK");
            return;
        }
    }

    if (!useLogs.empty()) {
        m_statusMsg = useLogs[0];
    } else {
        m_statusMsg = LocalizationManager::getInstance().get("CHAR_MSG_ITEM_USED");
    }

    // 인벤토리 범위 보정
    if (m_inventoryIndex >= static_cast<int>(party.getInventory().size()) && m_inventoryIndex > 0) {
        m_inventoryIndex--;
    }
}

void CharacterInfoState::equipSelectedItem() {
    auto& party = m_game.getParty();
    const PartyCheckpoint checkpoint = party.captureCheckpoint();
    const auto& inv = party.getInventory();
    if (m_inventoryIndex >= static_cast<int>(inv.size())) return;

    auto item = inv[m_inventoryIndex];
    if (!item->isEquipment()) return;

    auto members = party.getMembers();
    if (m_selectedCharIndex >= static_cast<int>(members.size())) return;
    auto caster = members[m_selectedCharIndex];

    auto equip = std::dynamic_pointer_cast<Equipment>(item);
    if (!equip) return;

    // 캐릭터 클래스에 따른 D&D 아이템 장착 제한 룰 적용 (v0.8.0)
    std::string itemId = equip->getId();
    if (caster->getClass() == CharacterClass::MAGE) {
        // 마법사: 대거, 스태프, 로브만 착용 가능
        if (itemId != "wpn_dagger" && itemId != "wpn_staff" && itemId != "arm_robe") {
            m_statusMsg = LocalizationManager::getInstance().get("CHAR_MSG_MAGE_EQUIP_LIMIT");
            return;
        }
    } else if (caster->getClass() == CharacterClass::ROGUE) {
        // 도적: 대거, 레이피어, 가죽 갑옷만 착용 가능 (방패 및 중장비 불가)
        if (itemId != "wpn_dagger" && itemId != "wpn_rapier" && itemId != "arm_leather") {
            m_statusMsg = LocalizationManager::getInstance().get("CHAR_MSG_ROGUE_EQUIP_LIMIT");
            return;
        }
    } else if (caster->getClass() == CharacterClass::CLERIC) {
        // 성직자: 대검(그레이트소드), 마법지팡이, 판금갑옷, 타워실드, 레이피어 불가
        if (itemId == "wpn_greatsword" || itemId == "wpn_staff" || itemId == "wpn_rapier" || 
            itemId == "arm_plate" || itemId == "shd_tower") {
            m_statusMsg = LocalizationManager::getInstance().get("CHAR_MSG_CLERIC_EQUIP_LIMIT");
            return;
        }
    } else if (caster->getClass() == CharacterClass::WARRIOR) {
        // 전사: 마법지팡이, 마법로브 불가
        if (itemId == "wpn_staff" || itemId == "arm_robe") {
            m_statusMsg = LocalizationManager::getInstance().get("CHAR_MSG_WARRIOR_EQUIP_LIMIT");
            return;
        }
    }

    EquipSlot slot = equip->getSlot();
    auto prevEquip = caster->getEquippedItem(slot);

    // [v0.8.0] 양손 무기(그레이트소드) 장착 시 방패 탈거 자동화 및 역 연산
    std::shared_ptr<Equipment> autoUnequippedItem = nullptr;
    if (slot == EquipSlot::WEAPON && itemId == "wpn_greatsword") {
        if (caster->getEquippedItem(EquipSlot::SHIELD)) {
            autoUnequippedItem = caster->getEquippedItem(EquipSlot::SHIELD);
            caster->unequip(EquipSlot::SHIELD);
            m_statusMsg = LocalizationManager::getInstance().get("CHAR_MSG_SHIELD_AUTO_UNEQUIP");
        }
    } else if (slot == EquipSlot::SHIELD) {
        auto curWpn = caster->getEquippedItem(EquipSlot::WEAPON);
        if (curWpn && curWpn->getId() == "wpn_greatsword") {
            autoUnequippedItem = curWpn;
            caster->unequip(EquipSlot::WEAPON);
            m_statusMsg = LocalizationManager::getInstance().get("CHAR_MSG_WEAPON_AUTO_UNEQUIP");
        }
    }

    // 장비 장착 처리
    if (caster->equip(equip)) {
        // 기존 장비가 있었다면 해제하여 가방에 반환
        party.removeItem(m_inventoryIndex);
        if (prevEquip) {
            party.addItem(prevEquip);
        }
        // 양손 무기로 인해 자동 탈거된 장비도 가방에 안전 적재 반환
        if (autoUnequippedItem) {
            party.addItem(autoUnequippedItem);
        }

        if (m_persistChanges) {
            const auto saveResult = party.saveToFile();
            if (!saveResult.durabilityConfirmed()) {
                m_statusMsg = LocalizationManager::getInstance().get("TOWN_MSG_DURABILITY_UNKNOWN");
                return;
            }
            if (!saveResult) {
                party.restoreCheckpoint(checkpoint);
                m_statusMsg = LocalizationManager::getInstance().get("CHAR_MSG_EQUIP_ROLLBACK");
                return;
            }
        }
        
        m_statusMsg = LocalizationManager::getInstance().format("CHAR_MSG_EQUIPPED", {
            {"name", caster->getName()}, {"item", equip->getName()}});
        
        // 인벤토리 인덱스 보정
        if (m_inventoryIndex >= static_cast<int>(party.getInventory().size()) && m_inventoryIndex > 0) {
            m_inventoryIndex--;
        }
    } else {
        if (autoUnequippedItem) {
            static_cast<void>(caster->equip(autoUnequippedItem));
        }
        m_statusMsg = LocalizationManager::getInstance().get("CHAR_MSG_EQUIP_FAILED");
    }
}

void CharacterInfoState::unequipSelectedSlot() {
    auto& party = m_game.getParty();
    const PartyCheckpoint checkpoint = party.captureCheckpoint();
    auto members = party.getMembers();
    if (m_selectedCharIndex >= static_cast<int>(members.size())) return;
    auto caster = members[m_selectedCharIndex];

    EquipSlot slot = EquipSlot::WEAPON;
    std::string slotName = LocalizationManager::getInstance().get("SLOT_WEAPON");
    if (m_equipmentSlotIndex == 1) {
        slot = EquipSlot::ARMOR;
        slotName = LocalizationManager::getInstance().get("SLOT_ARMOR");
    } else if (m_equipmentSlotIndex == 2) {
        slot = EquipSlot::SHIELD;
        slotName = LocalizationManager::getInstance().get("SLOT_SHIELD");
    }

    auto equipped = caster->getEquippedItem(slot);
    if (!equipped) {
        m_statusMsg = LocalizationManager::getInstance().get("CHAR_MSG_SLOT_EMPTY");
        return;
    }

    // 장비 장착 해제 후 가방 반환
    caster->unequip(slot);
    party.addItem(equipped);

    if (m_persistChanges) {
        const auto saveResult = party.saveToFile();
        if (!saveResult.durabilityConfirmed()) {
            m_statusMsg = LocalizationManager::getInstance().get("TOWN_MSG_DURABILITY_UNKNOWN");
            return;
        }
        if (!saveResult) {
            party.restoreCheckpoint(checkpoint);
            m_statusMsg = LocalizationManager::getInstance().get("CHAR_MSG_UNEQUIP_ROLLBACK");
            return;
        }
    }

    m_statusMsg = LocalizationManager::getInstance().format("CHAR_MSG_UNEQUIPPED", {
        {"name", caster->getName()}, {"slot", slotName}, {"item", equipped->getName()}});
}

void CharacterInfoState::drawBox(sf::RenderWindow& window, float x, float y, float w, float h, sf::Color color, float thickness) {
    sf::RectangleShape box(sf::Vector2f(w, h));
    box.setPosition(x, y);
    box.setFillColor(sf::Color::Transparent);
    box.setOutlineColor(color);
    box.setOutlineThickness(thickness);
    window.draw(box);
}

void CharacterInfoState::drawText(sf::RenderWindow& window, const std::string& str, float x, float y, int size, sf::Color color, bool center) {
    sf::Text text;
    text.setFont(m_game.getFont());

    // C++ std::string의 메모리 안정적 수명을 보장하기 위해 로컬 변수에 명시 보존 후 이터레이터로 UTF-8 로드
    std::string safeStr = str;
    text.setString(sf::String::fromUtf8(safeStr.begin(), safeStr.end()));

    text.setCharacterSize(LocalizationManager::getInstance().getScaledTextSize(
        static_cast<unsigned int>(std::max(1, size))));
    text.setFillColor(color);

    if (center) {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    }

    text.setPosition(x, y);
    window.draw(text);
}

} // namespace crawl
