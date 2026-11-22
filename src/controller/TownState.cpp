#include "controller/TownState.hpp"
#include "controller/BestiaryState.hpp"
#include "controller/CharacterCreationState.hpp"
#include "controller/CharacterInfoState.hpp"
#include "controller/DungeonState.hpp"
#include "controller/QuestJournalState.hpp"
#include "controller/SettingsState.hpp"
#include "controller/TitleState.hpp"
#include "core/AudioManager.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "model/CombatRules.hpp"
#include "model/Equipment.hpp"
#include "model/ItemFactory.hpp"
#include "model/Quest.hpp"
#include "view/UiSystem.hpp"
#include "view/UiTheme.hpp"

#include <algorithm>
#include <array>

namespace crawl {

TownState::TownState(Game& game) : m_game(game) {
    m_selectedFloor = std::clamp(m_game.getParty().getDeepestFloorReached(), 1, DungeonWorld::FLOOR_COUNT);
    AudioManager::getInstance().stopAmbience();
    AudioManager::getInstance().playBgm("bgm_town");
}

void TownState::setNotice(const std::string& msg) {
    m_statusMessage = msg;
    m_game.pushToast(msg, UiTheme::borderGold());
}

std::vector<TownState::ForgeEntry> TownState::collectForgeEntries() const {
    std::vector<ForgeEntry> out;
    const auto& party = m_game.getParty();
    for (int i = 0; i < party.getMemberCount(); ++i) {
        const auto m = party.getMember(i);
        if (!m) continue;
        for (EquipSlot sl : {EquipSlot::WEAPON, EquipSlot::ARMOR, EquipSlot::SHIELD, EquipSlot::ACCESSORY}) {
            if (auto eq = m->getEquippedItem(sl)) {
                out.push_back({eq, m->getName()});
            }
        }
    }
    for (const auto& item : party.getInventory()) {
        if (auto eq = std::dynamic_pointer_cast<Equipment>(item)) {
            out.push_back({eq, LocalizationManager::getInstance().get("TOWN_FORGE_BAG")});
        }
    }
    return out;
}

void TownState::handleHubAction(int actionId) {
    AudioManager::getInstance().playSfx("sfx_ui_click");
    m_confirmingDismiss = false;
    m_confirmingSell = false;
    m_pendingConfirmIndex = -1;
    m_selectedIndex = 0;

    switch (actionId) {
        case 0: m_location = TownLocation::DUNGEON_GATE; break;
        case 1: m_location = TownLocation::GUILD; break;
        case 2: m_location = TownLocation::SHOP_BUY; break;
        case 3: m_location = TownLocation::SHOP_SELL; break;
        case 4: m_location = TownLocation::BLACKSMITH; break;
        case 5: m_location = TownLocation::CHURCH; break;
        case 6: m_location = TownLocation::QUEST_BOARD; break;
        case 7: m_game.getStates().pushState(std::make_unique<CharacterInfoState>(m_game, 0)); break;
        case 8: m_game.getStates().pushState(std::make_unique<BestiaryState>(m_game)); break;
        case 9: m_game.getStates().pushState(std::make_unique<SettingsState>(m_game)); break;
        case 10: {
            m_game.getParty().saveToFile();
            m_game.triggerFadeIn(0.25f);
            m_game.getStates().replaceAll(std::make_unique<TitleState>(m_game));
            break;
        }
        default: break;
    }
}

void TownState::handleSubAction(int actionId) {
    auto& party = m_game.getParty();
    const auto& lm = LocalizationManager::getInstance();

    if (m_location == TownLocation::DUNGEON_GATE) {
        if (actionId >= 1 && actionId <= DungeonWorld::FLOOR_COUNT) {
            if (actionId > party.getDeepestFloorReached()) {
                AudioManager::getInstance().playSfx("sfx_ui_error");
                setNotice(lm.get("TOWN_FLOOR_LOCKED"));
                return;
            }
            if (party.getMemberCount() <= 0) {
                AudioManager::getInstance().playSfx("sfx_ui_error");
                setNotice(lm.get("TOWN_NO_PARTY"));
                return;
            }
            m_selectedFloor = actionId;
            party.saveToFile();
            AudioManager::getInstance().playSfx("sfx_door");
            m_game.triggerFadeIn(0.30f);
            m_game.getStates().changeState(std::make_unique<DungeonState>(m_game, m_selectedFloor));
        }
        return;
    }

    if (m_location == TownLocation::GUILD) {
        if (actionId == 100) {
            if (party.getMemberCount() >= Party::MAX_MEMBERS) {
                AudioManager::getInstance().playSfx("sfx_ui_error");
                setNotice(lm.get("TOWN_GUILD_FULL"));
                return;
            }
            AudioManager::getInstance().playSfx("sfx_ui_click");
            m_game.getStates().pushState(std::make_unique<CharacterCreationState>(m_game));
            return;
        }
        if (actionId >= 0 && actionId < party.getMemberCount()) {
            if (party.getMemberCount() <= 1) {
                AudioManager::getInstance().playSfx("sfx_ui_error");
                setNotice(lm.get("TOWN_GUILD_LAST_MEMBER"));
                return;
            }
            if (!m_confirmingDismiss || m_pendingConfirmIndex != actionId) {
                m_confirmingDismiss = true;
                m_pendingConfirmIndex = actionId;
                setNotice(lm.get("TOWN_GUILD_DISMISS_CONFIRM"));
                AudioManager::getInstance().playSfx("sfx_ui_click");
                return;
            }
            const PartyCheckpoint cp = party.captureCheckpoint();
            party.removeMember(actionId);
            if (!party.saveToFile()) {
                party.restoreCheckpoint(cp);
                setNotice(lm.get("TITLE_SAVE_ERROR"));
                return;
            }
            m_confirmingDismiss = false;
            m_pendingConfirmIndex = -1;
            AudioManager::getInstance().playSfx("sfx_ui_click");
            setNotice(lm.get("TOWN_GUILD_DISMISSED"));
        }
        return;
    }

    if (m_location == TownLocation::SHOP_BUY) {
        const auto catalog = ItemFactory::getShopCatalog(party.getDeepestFloorReached());
        if (actionId < 0 || actionId >= static_cast<int>(catalog.size())) return;
        const auto& item = catalog[static_cast<std::size_t>(actionId)];
        if (!party.spendGold(item->getGoldValue())) {
            AudioManager::getInstance().playSfx("sfx_ui_error");
            setNotice(lm.get("SHOP_NO_GOLD"));
            return;
        }
        const PartyCheckpoint cp = party.captureCheckpoint();
        party.addItem(item->clone());
        if (!party.saveToFile()) {
            party.restoreCheckpoint(cp);
            party.addGold(item->getGoldValue());
            setNotice(lm.get("TITLE_SAVE_ERROR"));
            return;
        }
        AudioManager::getInstance().playSfx("sfx_chest");
        setNotice(lm.format("SHOP_BOUGHT", {{"item", item->getName()}}));
        return;
    }

    if (m_location == TownLocation::SHOP_SELL) {
        const auto& inv = party.getInventory();
        if (actionId < 0 || actionId >= static_cast<int>(inv.size())) return;
        if (!m_confirmingSell || m_pendingConfirmIndex != actionId) {
            m_confirmingSell = true;
            m_pendingConfirmIndex = actionId;
            setNotice(lm.get("SHOP_SELL_CONFIRM"));
            AudioManager::getInstance().playSfx("sfx_ui_click");
            return;
        }
        const int price = std::max(1, inv[static_cast<std::size_t>(actionId)]->getGoldValue() / 2);
        const std::string itemName = inv[static_cast<std::size_t>(actionId)]->getName();
        const PartyCheckpoint cp = party.captureCheckpoint();
        party.removeItem(actionId);
        party.addGold(price);
        if (!party.saveToFile()) {
            party.restoreCheckpoint(cp);
            setNotice(lm.get("TITLE_SAVE_ERROR"));
            return;
        }
        m_confirmingSell = false;
        m_pendingConfirmIndex = -1;
        AudioManager::getInstance().playSfx("sfx_chest");
        setNotice(lm.format("SHOP_SOLD", {{"item", itemName}, {"gold", std::to_string(price)}}));
        return;
    }

    if (m_location == TownLocation::BLACKSMITH) {
        const auto entries = collectForgeEntries();
        if (entries.empty()) return;
        m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(entries.size()) - 1);
        auto equip = entries[static_cast<std::size_t>(m_selectedIndex)].equip;

        if (actionId == 200) { // 강화 (+1)
            if (!CombatRules::canEnhance(*equip)) {
                AudioManager::getInstance().playSfx("sfx_ui_error");
                setNotice(lm.get("FORGE_MAX_LEVEL"));
                return;
            }
            const int cost = CombatRules::blacksmithEnhanceCost(*equip);
            if (!party.spendGold(cost)) {
                AudioManager::getInstance().playSfx("sfx_ui_error");
                setNotice(lm.get("SHOP_NO_GOLD"));
                return;
            }
            const PartyCheckpoint cp = party.captureCheckpoint();
            CombatRules::enhanceEquipment(*equip);
            if (!party.saveToFile()) {
                party.restoreCheckpoint(cp);
                setNotice(lm.get("TITLE_SAVE_ERROR"));
                return;
            }
            AudioManager::getInstance().playSfx("sfx_levelup");
            setNotice(lm.format("FORGE_ENHANCED", {{"item", equip->getDisplayName()}}));
        } else if (actionId == 201) { // 접사 재연마
            const int cost = CombatRules::blacksmithReforgeCost(*equip);
            if (!party.spendGold(cost)) {
                AudioManager::getInstance().playSfx("sfx_ui_error");
                setNotice(lm.get("SHOP_NO_GOLD"));
                return;
            }
            const PartyCheckpoint cp = party.captureCheckpoint();
            CombatRules::reforgeEquipment(*equip, SessionRng::global());
            if (!party.saveToFile()) {
                party.restoreCheckpoint(cp);
                setNotice(lm.get("TITLE_SAVE_ERROR"));
                return;
            }
            AudioManager::getInstance().playSfx("sfx_fire_spell");
            setNotice(lm.format("FORGE_REFORGED", {{"item", equip->getDisplayName()}}));
        }
        return;
    }

    if (m_location == TownLocation::CHURCH) {
        if (actionId == 300) { // 완전 휴식 + 부활 + 정화
            const int cost = 35;
            if (!party.spendGold(cost)) {
                AudioManager::getInstance().playSfx("sfx_ui_error");
                setNotice(lm.get("SHOP_NO_GOLD"));
                return;
            }
            const PartyCheckpoint cp = party.captureCheckpoint();
            for (const auto& m : party.getMembers()) {
                if (m) m->rest();
            }
            if (!party.saveToFile()) {
                party.restoreCheckpoint(cp);
                setNotice(lm.get("TITLE_SAVE_ERROR"));
                return;
            }
            AudioManager::getInstance().playSfx("sfx_sanctuary");
            setNotice(lm.get("CHURCH_RESTORED"));
        }
        return;
    }

    if (m_location == TownLocation::QUEST_BOARD) {
        const auto ids = Quest::getOfferableIds();
        if (actionId < 0 || actionId >= static_cast<int>(ids.size())) return;
        const std::string& qid = ids[static_cast<std::size_t>(actionId)];
        if (party.isQuestCompleted(qid)) return;

        const PartyCheckpoint cp = party.captureCheckpoint();
        if (auto active = party.getQuest(qid)) {
            if (!active->isReadyToReport()) {
                AudioManager::getInstance().playSfx("sfx_ui_error");
                setNotice(lm.get("QUEST_NOT_READY"));
                return;
            }
            const int goldR = active->getGoldReward();
            const int xpR = active->getXpReward();
            const auto rewardItems = active->getRewardItemIds();
            if (active->getType() == QuestType::RETRIEVE_KEY_ITEM) {
                party.removeKeyItem(active->getTargetId());
            }
            party.completeQuest(qid);
            party.addGold(goldR);
            for (const auto& rid : rewardItems) {
                party.addItem(ItemFactory::createItem(rid));
            }
            for (const auto& m : party.getMembers()) {
                if (m && !m->isDead()) m->addXp(xpR);
            }
            if (!party.saveToFile()) {
                party.restoreCheckpoint(cp);
                setNotice(lm.get("TITLE_SAVE_ERROR"));
                return;
            }
            AudioManager::getInstance().playSfx("sfx_levelup");
            setNotice(lm.format("QUEST_COMPLETED_LOG", {{"quest", active->getName()}}));
        } else {
            auto q = Quest::createCanonical(qid);
            party.acceptQuest(q);
            if (!party.saveToFile()) {
                party.restoreCheckpoint(cp);
                setNotice(lm.get("TITLE_SAVE_ERROR"));
                return;
            }
            AudioManager::getInstance().playSfx("sfx_ui_click");
            setNotice(lm.format("QUEST_ACCEPTED_LOG", {{"quest", q->getName()}}));
        }
    }
}

void TownState::handleInput(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        const auto pos = m_game.mapPixelToLogical({event.mouseButton.x, event.mouseButton.y});
        for (const auto& btn : m_navButtons) {
            if (btn.rect.contains(pos)) {
                handleHubAction(btn.actionId);
                return;
            }
        }
        for (const auto& btn : m_actionButtons) {
            if (btn.rect.contains(pos)) {
                handleSubAction(btn.actionId);
                return;
            }
        }
        for (const auto& item : m_listButtons) {
            if (item.rect.contains(pos)) {
                if (m_location == TownLocation::BLACKSMITH) {
                    m_selectedIndex = item.actionId;
                    AudioManager::getInstance().playSfx("sfx_ui_hover");
                } else {
                    m_selectedIndex = item.actionId;
                    handleSubAction(item.actionId);
                }
                return;
            }
        }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
        if (m_location != TownLocation::HUB) {
            m_location = TownLocation::HUB;
            AudioManager::getInstance().playSfx("sfx_ui_click");
        }
        return;
    }

    if (event.type != sf::Event::KeyPressed) return;
    const auto key = event.key.code;

    if (key == sf::Keyboard::Escape) {
        if (m_location != TownLocation::HUB) {
            m_location = TownLocation::HUB;
            m_confirmingDismiss = false;
            m_confirmingSell = false;
            AudioManager::getInstance().playSfx("sfx_ui_click");
        }
        return;
    }
    if (key == sf::Keyboard::O) { handleHubAction(9); return; }
    if (key == sf::Keyboard::C) { handleHubAction(7); return; }
    if (key == sf::Keyboard::B) { handleHubAction(8); return; }
    if (key == sf::Keyboard::J || key == sf::Keyboard::Q) {
        m_game.getStates().pushState(std::make_unique<QuestJournalState>(m_game));
        return;
    }

    if (m_location == TownLocation::HUB) {
        if (key == sf::Keyboard::Num1) handleHubAction(0);
        else if (key == sf::Keyboard::Num2) handleHubAction(1);
        else if (key == sf::Keyboard::Num3) handleHubAction(2);
        else if (key == sf::Keyboard::Num4) handleHubAction(3);
        else if (key == sf::Keyboard::Num5) handleHubAction(4);
        else if (key == sf::Keyboard::Num6) handleHubAction(5);
        else if (key == sf::Keyboard::Num7) handleHubAction(6);
        else if (key == sf::Keyboard::Num8) handleHubAction(7);
        else if (key == sf::Keyboard::Num9) handleHubAction(8);
        return;
    }

    if (key == sf::Keyboard::Up || key == sf::Keyboard::W) {
        m_selectedIndex = std::max(0, m_selectedIndex - 1);
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    } else if (key == sf::Keyboard::Down || key == sf::Keyboard::S) {
        ++m_selectedIndex;
        AudioManager::getInstance().playSfx("sfx_ui_hover");
    } else if (key == sf::Keyboard::Enter) {
        if (m_location == TownLocation::DUNGEON_GATE) handleSubAction(m_selectedFloor);
        else if (m_location == TownLocation::CHURCH) handleSubAction(300);
        else if (m_location == TownLocation::BLACKSMITH) handleSubAction(200);
        else handleSubAction(m_selectedIndex);
    } else if (m_location == TownLocation::BLACKSMITH && key == sf::Keyboard::R) {
        handleSubAction(201);
    } else if (m_location == TownLocation::GUILD && key == sf::Keyboard::N) {
        handleSubAction(100);
    } else if (m_location == TownLocation::DUNGEON_GATE && key >= sf::Keyboard::Num1 && key <= sf::Keyboard::Num5) {
        handleSubAction(key - sf::Keyboard::Num1 + 1);
    }
}

void TownState::update(sf::Time deltaTime) { m_time += deltaTime.asSeconds(); }

void TownState::draw(sf::RenderWindow& window) {
    auto& assets = m_game.getAssets();
    const auto& font = m_game.getFont();
    const auto& lm = LocalizationManager::getInstance();
    const auto& party = m_game.getParty();
    const auto mouse = m_game.getMouseLogicalPosition();

    m_navButtons.clear();
    m_listButtons.clear();
    m_actionButtons.clear();

    // 1. 다크 판타지 마을 광장 배경
    UiSystem::drawBackdrop(window, assets, "town_bg", 98);

    // 2. 상단 헤더 바
    UiSystem::drawPanel(window, assets, {16.0f, 12.0f, 1248.0f, 54.0f}, true, 232);
    UiSystem::drawText(window, font, lm.get("TOWN_HEADER_TITLE"), {34.0f, 24.0f}, 22, UiTheme::textGold(), true);
    const std::string stats = lm.format("TOWN_HEADER_STATS", {
        {"gold", std::to_string(party.getGold())},
        {"floor", std::to_string(party.getDeepestFloorReached())},
        {"codex", std::to_string(party.getBestiary().discoveredCount()) + "/24"}
    });
    UiSystem::drawText(window, font, stats, {700.0f, 28.0f}, 15, UiTheme::textPrimary(), true, 540.0f);

    // 3. 좌측 네비게이션 레일 (11개 시설/메뉴)
    const sf::FloatRect navPanel(16.0f, 76.0f, 286.0f, 502.0f);
    UiSystem::drawPanel(window, assets, navPanel, false, 228);

    struct NavDef { const char* key; const char* hk; TownLocation loc; int id; bool gold; };
    const std::array<NavDef, 11> navs = {{
        {"TOWN_NAV_DUNGEON",    "1", TownLocation::DUNGEON_GATE, 0, true},
        {"TOWN_NAV_GUILD",      "2", TownLocation::GUILD,        1, false},
        {"TOWN_NAV_SHOP_BUY",   "3", TownLocation::SHOP_BUY,     2, false},
        {"TOWN_NAV_SHOP_SELL",  "4", TownLocation::SHOP_SELL,    3, false},
        {"TOWN_NAV_BLACKSMITH", "5", TownLocation::BLACKSMITH,   4, false},
        {"TOWN_NAV_CHURCH",     "6", TownLocation::CHURCH,       5, false},
        {"TOWN_NAV_QUESTS",     "7", TownLocation::QUEST_BOARD,  6, false},
        {"TOWN_NAV_PARTY",      "C", TownLocation::HUB,          7, false},
        {"TOWN_NAV_BESTIARY",   "B", TownLocation::HUB,          8, false},
        {"TOWN_NAV_SETTINGS",   "O", TownLocation::HUB,          9, false},
        {"TOWN_NAV_TITLE",      "",  TownLocation::HUB,         10, false}
    }};

    for (std::size_t i = 0; i < navs.size(); ++i) {
        const sf::FloatRect br(28.0f, 88.0f + static_cast<float>(i) * 43.0f, 262.0f, 38.0f);
        const bool isActive = (m_location != TownLocation::HUB && m_location == navs[i].loc);
        UiSystem::drawButton(window, assets, font, {
            br, lm.get(navs[i].key), navs[i].hk, isActive, br.contains(mouse), true, navs[i].gold
        });
        m_navButtons.push_back({br, navs[i].id});
    }

    // 4. 우측 메인 스테이지 패널
    const sf::FloatRect stage(314.0f, 76.0f, 950.0f, 502.0f);
    UiSystem::drawPanel(window, assets, stage, false, 228);

    std::string tooltipTitle;
    std::vector<std::string> tooltipLines;
    sf::Color tooltipCol = UiTheme::textGold();

    if (m_location == TownLocation::HUB) {
        UiSystem::drawText(window, font, lm.get("TOWN_WELCOME_TITLE"), {stage.left + 28.0f, stage.top + 24.0f},
                           24, UiTheme::textGold(), true);
        const auto lines = UiSystem::wrapUtf8Lines(lm.get("TOWN_WELCOME_DESC"), font, 15, stage.width - 56.0f);
        float y = stage.top + 68.0f;
        for (const auto& l : lines) {
            UiSystem::drawText(window, font, l, {stage.left + 28.0f, y}, 15, UiTheme::textPrimary());
            y += 24.0f;
        }

        // 활성 퀘스트 요약 카드
        UiSystem::drawPanel(window, assets, {stage.left + 28.0f, stage.top + 172.0f, 430.0f, 290.0f}, false, 210);
        UiSystem::drawText(window, font, lm.get("TOWN_ACTIVE_QUESTS"), {stage.left + 44.0f, stage.top + 186.0f},
                           17, UiTheme::textGold());
        float qy = stage.top + 220.0f;
        if (party.getActiveQuests().empty()) {
            UiSystem::drawText(window, font, lm.get("QUEST_NONE_ACTIVE"), {stage.left + 44.0f, qy}, 14, UiTheme::textMuted());
        } else {
            for (const auto& q : party.getActiveQuests()) {
                const std::string tag = q->isReadyToReport() ? " [READY]" : (" (" + std::to_string(q->getTargetFloor()) + "F)");
                UiSystem::drawText(window, font, "• " + q->getName() + tag,
                                   {stage.left + 44.0f, qy}, 14,
                                   q->isReadyToReport() ? UiTheme::statPos() : UiTheme::textPrimary(), false, 396.0f);
                qy += 28.0f;
            }
        }

        // 5층 탐험 진행 카드
        UiSystem::drawPanel(window, assets, {stage.left + 480.0f, stage.top + 172.0f, 442.0f, 290.0f}, false, 210);
        UiSystem::drawText(window, font, lm.get("TOWN_BIOME_PROGRESS"), {stage.left + 496.0f, stage.top + 186.0f},
                           17, UiTheme::textGold());
        for (int f = 1; f <= DungeonWorld::FLOOR_COUNT; ++f) {
            const bool unlocked = f <= party.getDeepestFloorReached();
            const std::string bname = std::to_string(f) + "F · " + lm.get("BIOME_" + std::to_string(f) + "_NAME");
            UiSystem::drawText(window, font, bname, {stage.left + 498.0f, stage.top + 224.0f + static_cast<float>(f - 1) * 42.0f},
                               15, unlocked ? UiTheme::textPrimary() : UiTheme::textMuted(), true, 320.0f);
            UiSystem::drawBadge(window, font, {stage.left + 826.0f, stage.top + 224.0f + static_cast<float>(f - 1) * 42.0f},
                                unlocked ? "OPEN" : "LOCK",
                                unlocked ? sf::Color(32, 92, 54) : sf::Color(58, 36, 36),
                                unlocked ? UiTheme::statPos() : UiTheme::textMuted(), 11);
        }
    } else if (m_location == TownLocation::DUNGEON_GATE) {
        UiSystem::drawText(window, font, lm.get("TOWN_GATE_TITLE"), {stage.left + 26.0f, stage.top + 20.0f},
                           22, UiTheme::textGold(), true);
        UiSystem::drawText(window, font, lm.get("TOWN_GATE_DESC"), {stage.left + 26.0f, stage.top + 54.0f},
                           14, UiTheme::textMuted());
        for (int f = 1; f <= DungeonWorld::FLOOR_COUNT; ++f) {
            const bool unlocked = f <= party.getDeepestFloorReached();
            const sf::FloatRect br(stage.left + 36.0f, stage.top + 98.0f + static_cast<float>(f - 1) * 72.0f,
                                   stage.width - 72.0f, 58.0f);
            const std::string label = std::to_string(f) + "F  —  " + lm.get("BIOME_" + std::to_string(f) + "_NAME") +
                                      "   [" + lm.get("BIOME_" + std::to_string(f) + "_DESC") + "]";
            UiSystem::drawButton(window, assets, font, {
                br, label, std::to_string(f), m_selectedFloor == f, br.contains(mouse), unlocked, f == 5 && unlocked
            });
            m_actionButtons.push_back({br, f});
        }
    } else if (m_location == TownLocation::GUILD) {
        UiSystem::drawText(window, font, lm.get("TOWN_GUILD_TITLE"), {stage.left + 26.0f, stage.top + 20.0f},
                           22, UiTheme::textGold(), true);
        const sf::FloatRect recruitBtn(stage.left + 26.0f, stage.top + 66.0f, 340.0f, 48.0f);
        UiSystem::drawButton(window, assets, font, {
            recruitBtn, lm.get("TOWN_GUILD_RECRUIT"), "N", false, recruitBtn.contains(mouse),
            party.getMemberCount() < Party::MAX_MEMBERS, true
        });
        m_actionButtons.push_back({recruitBtn, 100});

        for (int i = 0; i < party.getMemberCount(); ++i) {
            const auto m = party.getMember(i);
            const sf::FloatRect r(stage.left + 26.0f, stage.top + 132.0f + static_cast<float>(i) * 82.0f,
                                  stage.width - 52.0f, 70.0f);
            UiSystem::drawPanel(window, assets, r, false, 224);
            const std::string rowStr = m->getRow() == CombatRow::FRONT ? lm.get("ROW_FRONT") : lm.get("ROW_BACK");
            UiSystem::drawText(window, font,
                               m->getName() + " · " + m->getClassString() + " (" + m->getSpecializationName() + ") · Lv." +
                                   std::to_string(m->getLevel()) + " [" + rowStr + "]",
                               {r.left + 18.0f, r.top + 14.0f}, 16, UiTheme::textPrimary(), true, 640.0f);
            UiSystem::drawText(window, font,
                               "HP " + std::to_string(m->getHp()) + "/" + std::to_string(m->getMaxHp()) +
                                   "  ·  AC " + std::to_string(m->getAc()),
                               {r.left + 18.0f, r.top + 40.0f}, 13, UiTheme::textMuted());
            const sf::FloatRect dismissR(r.left + r.width - 170.0f, r.top + 15.0f, 152.0f, 40.0f);
            const bool isConfirm = m_confirmingDismiss && m_pendingConfirmIndex == i;
            UiSystem::drawButton(window, assets, font, {
                dismissR, isConfirm ? lm.get("COMMON_CONFIRM") : lm.get("TOWN_GUILD_DISMISS"), "",
                isConfirm, dismissR.contains(mouse), party.getMemberCount() > 1, isConfirm
            });
            m_actionButtons.push_back({dismissR, i});
        }
    } else if (m_location == TownLocation::SHOP_BUY || m_location == TownLocation::SHOP_SELL) {
        const bool isBuy = (m_location == TownLocation::SHOP_BUY);
        UiSystem::drawText(window, font, lm.get(isBuy ? "SHOP_BUY_TITLE" : "SHOP_SELL_TITLE"),
                           {stage.left + 26.0f, stage.top + 18.0f}, 22, UiTheme::textGold(), true);
        const auto items = isBuy ? ItemFactory::getShopCatalog(party.getDeepestFloorReached()) : party.getInventory();
        const int maxPerPage = 10;
        const int total = static_cast<int>(items.size());
        m_selectedIndex = total > 0 ? std::clamp(m_selectedIndex, 0, total - 1) : 0;
        const int offset = std::max(0, m_selectedIndex - maxPerPage + 1);

        if (items.empty()) {
            UiSystem::drawText(window, font, lm.get("SHOP_EMPTY"), {stage.left + 32.0f, stage.top + 86.0f},
                               15, UiTheme::textMuted());
        } else {
            for (int i = offset; i < std::min(total, offset + maxPerPage); ++i) {
                const auto& item = items[static_cast<std::size_t>(i)];
                const int vis = i - offset;
                const sf::FloatRect r(stage.left + 26.0f, stage.top + 62.0f + static_cast<float>(vis) * 41.0f,
                                      stage.width - 52.0f, 36.0f);
                const bool hov = r.contains(mouse);
                const bool sel = (m_selectedIndex == i) || hov;
                const int price = isBuy ? item->getGoldValue() : std::max(1, item->getGoldValue() / 2);

                sf::RectangleShape rowBg({r.width, r.height});
                rowBg.setPosition(r.left, r.top);
                rowBg.setFillColor(sel ? UiTheme::panelHover() : UiTheme::panelAlt());
                rowBg.setOutlineThickness(sel ? 1.5f : 1.0f);
                rowBg.setOutlineColor(sel ? UiTheme::borderGold() : UiTheme::borderBronze());
                window.draw(rowBg);

                std::string disp = item->getName();
                if (auto eq = std::dynamic_pointer_cast<Equipment>(item)) disp = eq->getDisplayName();
                UiSystem::drawText(window, font, disp, {r.left + 14.0f, r.top + 8.0f}, 14,
                                   UiTheme::rarityColor(item->getRarity()), true, 440.0f);
                UiSystem::drawText(window, font, item->getDescription(), {r.left + 465.0f, r.top + 9.0f},
                                   12, UiTheme::textMuted(), false, 300.0f);
                UiSystem::drawText(window, font, std::to_string(price) + " G",
                                   {r.left + r.width - 110.0f, r.top + 8.0f}, 14,
                                   (!isBuy || party.getGold() >= price) ? UiTheme::textGold() : UiTheme::hpCrimson());
                m_listButtons.push_back({r, i});

                if (hov) {
                    tooltipTitle = disp;
                    tooltipCol = UiTheme::rarityColor(item->getRarity());
                    tooltipLines = {item->getDescription(), "Price: " + std::to_string(price) + " G"};
                    if (auto eq = std::dynamic_pointer_cast<Equipment>(item)) {
                        if (eq->getSlot() == EquipSlot::WEAPON) {
                            tooltipLines.push_back("DMG: " + std::to_string(eq->getDamageDiceCount()) + "d" +
                                                   std::to_string(eq->getDamageDiceSides()) + " +" +
                                                   std::to_string(eq->getUpgradeDamageBonus()) + " [" +
                                                   UiTheme::elementShortCode(eq->getDamageType()) + "]");
                        } else {
                            tooltipLines.push_back("AC +" + std::to_string(eq->getAcBonus()) +
                                                   "  HP +" + std::to_string(eq->getBonusHp()));
                        }
                    }
                }
            }
        }
    } else if (m_location == TownLocation::BLACKSMITH) {
        UiSystem::drawText(window, font, lm.get("TOWN_FORGE_TITLE"), {stage.left + 26.0f, stage.top + 18.0f},
                           22, UiTheme::textGold(), true);
        const auto entries = collectForgeEntries();
        if (entries.empty()) {
            UiSystem::drawText(window, font, lm.get("FORGE_NO_GEAR"), {stage.left + 28.0f, stage.top + 80.0f},
                               15, UiTheme::textMuted());
        } else {
            m_selectedIndex = std::clamp(m_selectedIndex, 0, static_cast<int>(entries.size()) - 1);
            const int maxShow = 10;
            const int offset = std::max(0, m_selectedIndex - maxShow + 1);
            for (int i = offset; i < std::min(static_cast<int>(entries.size()), offset + maxShow); ++i) {
                const auto& fe = entries[static_cast<std::size_t>(i)];
                const sf::FloatRect r(stage.left + 24.0f, stage.top + 60.0f + static_cast<float>(i - offset) * 41.0f,
                                      500.0f, 36.0f);
                const bool sel = (m_selectedIndex == i) || r.contains(mouse);
                sf::RectangleShape bg({r.width, r.height});
                bg.setPosition(r.left, r.top);
                bg.setFillColor(sel ? UiTheme::panelHover() : UiTheme::panelAlt());
                bg.setOutlineThickness(sel ? 1.5f : 1.0f);
                bg.setOutlineColor(sel ? UiTheme::borderGold() : UiTheme::borderBronze());
                window.draw(bg);
                UiSystem::drawText(window, font, fe.equip->getDisplayName(), {r.left + 12.0f, r.top + 8.0f}, 14,
                                   UiTheme::rarityColor(fe.equip->getRarity()), true, 340.0f);
                UiSystem::drawText(window, font, "[" + fe.ownerLabel + "]", {r.left + 362.0f, r.top + 9.0f}, 12,
                                   UiTheme::textMuted(), false, 128.0f);
                m_listButtons.push_back({r, i});
            }

            // 우측 강화/재연마 상세 패널
            const auto& selEq = entries[static_cast<std::size_t>(m_selectedIndex)].equip;
            const sf::FloatRect det(stage.left + 544.0f, stage.top + 60.0f, 380.0f, 412.0f);
            UiSystem::drawPanel(window, assets, det, true, 234);
            UiSystem::drawText(window, font, selEq->getDisplayName(), {det.left + 18.0f, det.top + 18.0f}, 18,
                               UiTheme::rarityColor(selEq->getRarity()), true, det.width - 36.0f);
            UiSystem::drawText(window, font, selEq->getDescription(), {det.left + 18.0f, det.top + 50.0f}, 13,
                               UiTheme::textMuted(), false, det.width - 36.0f);
            UiSystem::drawText(window, font,
                               "Upgrade: +" + std::to_string(selEq->getUpgradeLevel()) + " / +" +
                                   std::to_string(Equipment::MAX_UPGRADE_LEVEL),
                               {det.left + 18.0f, det.top + 96.0f}, 15, UiTheme::textGold());
            UiSystem::drawText(window, font,
                               "AC +" + std::to_string(selEq->getAcBonus()) +
                                   "  ·  DMG Bonus +" + std::to_string(selEq->getUpgradeDamageBonus()) +
                                   "  ·  HP +" + std::to_string(selEq->getBonusHp()),
                               {det.left + 18.0f, det.top + 126.0f}, 14, UiTheme::textPrimary());
            if (!selEq->getAffix().empty()) {
                UiSystem::drawText(window, font,
                                   "Affix: " + lm.get(selEq->getAffix().affixKey) +
                                       " (Crit +" + std::to_string(selEq->getCritBonusPct()) + "%, LS " +
                                       std::to_string(selEq->getLifestealPct()) + "%)",
                                   {det.left + 18.0f, det.top + 158.0f}, 13, UiTheme::cyanAccent(), false, det.width - 36.0f);
            }

            const int enhCost = CombatRules::blacksmithEnhanceCost(*selEq);
            const int refCost = CombatRules::blacksmithReforgeCost(*selEq);
            const sf::FloatRect enhBtn(det.left + 20.0f, det.top + 266.0f, det.width - 40.0f, 52.0f);
            const sf::FloatRect refBtn(det.left + 20.0f, det.top + 334.0f, det.width - 40.0f, 52.0f);
            UiSystem::drawButton(window, assets, font, {
                enhBtn, lm.get("FORGE_BTN_ENHANCE") + " (" + std::to_string(enhCost) + " G)", "Enter",
                false, enhBtn.contains(mouse), CombatRules::canEnhance(*selEq) && party.getGold() >= enhCost, true
            });
            UiSystem::drawButton(window, assets, font, {
                refBtn, lm.get("FORGE_BTN_REFORGE") + " (" + std::to_string(refCost) + " G)", "R",
                false, refBtn.contains(mouse), party.getGold() >= refCost, false
            });
            m_actionButtons.push_back({enhBtn, 200});
            m_actionButtons.push_back({refBtn, 201});
        }
    } else if (m_location == TownLocation::CHURCH) {
        UiSystem::drawText(window, font, lm.get("CHURCH_TITLE"), {stage.left + 26.0f, stage.top + 22.0f},
                           24, UiTheme::textGold(), true);
        UiSystem::drawText(window, font, lm.get("CHURCH_DESC"), {stage.left + 26.0f, stage.top + 66.0f},
                           15, UiTheme::textPrimary(), false, stage.width - 52.0f);
        const sf::FloatRect healBtn(stage.left + 26.0f, stage.top + 132.0f, 440.0f, 58.0f);
        UiSystem::drawButton(window, assets, font, {
            healBtn, lm.get("CHURCH_REST_BTN") + " (35 G)", "Enter", true, healBtn.contains(mouse),
            party.getGold() >= 35, true
        });
        m_actionButtons.push_back({healBtn, 300});
    } else if (m_location == TownLocation::QUEST_BOARD) {
        UiSystem::drawText(window, font, lm.get("QUEST_BOARD_TITLE"), {stage.left + 26.0f, stage.top + 18.0f},
                           22, UiTheme::textGold(), true);
        const auto ids = Quest::getOfferableIds();
        for (std::size_t i = 0; i < ids.size(); ++i) {
            const std::string& qid = ids[i];
            const auto canon = Quest::createCanonical(qid);
            const bool done = party.isQuestCompleted(qid);
            const auto active = party.getQuest(qid);
            const sf::FloatRect r(stage.left + 24.0f, stage.top + 58.0f + static_cast<float>(i) * 53.0f,
                                  stage.width - 48.0f, 46.0f);
            const bool sel = (m_selectedIndex == static_cast<int>(i)) || r.contains(mouse);
            sf::RectangleShape bg({r.width, r.height});
            bg.setPosition(r.left, r.top);
            bg.setFillColor(sel ? UiTheme::panelHover() : UiTheme::panelAlt());
            bg.setOutlineThickness(sel ? 1.5f : 1.0f);
            bg.setOutlineColor(done ? sf::Color(60, 82, 62)
                                    : (active && active->isReadyToReport() ? UiTheme::statPos() : UiTheme::borderBronze()));
            window.draw(bg);

            const std::string title = "[" + std::to_string(canon->getTargetFloor()) + "F] " + canon->getName();
            UiSystem::drawText(window, font, title, {r.left + 14.0f, r.top + 6.0f}, 14,
                               done ? UiTheme::textMuted() : UiTheme::textGold(), true, 470.0f);
            UiSystem::drawText(window, font, canon->getDescription(), {r.left + 14.0f, r.top + 25.0f}, 12,
                               UiTheme::textMuted(), false, 640.0f);

            std::string badge = lm.get("QUEST_ACCEPT");
            sf::Color bCol = UiTheme::borderGold();
            if (done) { badge = lm.get("QUEST_DONE"); bCol = UiTheme::textMuted(); }
            else if (active && active->isReadyToReport()) { badge = lm.get("QUEST_REPORT"); bCol = UiTheme::statPos(); }
            else if (active) { badge = lm.get("QUEST_ACTIVE"); bCol = UiTheme::cyanAccent(); }

            UiSystem::drawText(window, font, std::to_string(canon->getGoldReward()) + "G / " +
                                   std::to_string(canon->getXpReward()) + "XP",
                               {r.left + r.width - 250.0f, r.top + 14.0f}, 12, UiTheme::textPrimary());
            UiSystem::drawBadge(window, font, {r.left + r.width - 110.0f, r.top + 12.0f},
                                badge, sf::Color(20, 22, 30), bCol, 11);
            m_listButtons.push_back({r, static_cast<int>(i)});
        }
    }

    // 5. 하단 4인 초상화 바
    const auto snap = buildPartyHudSnapshot(party);
    const auto& slots = snap.slots();
    for (std::size_t i = 0; i < 4; ++i) {
        const sf::FloatRect r(16.0f + static_cast<float>(i) * 314.0f, 592.0f, 304.0f, 114.0f);
        if (i < slots.size()) {
            UiSystem::drawPortraitCard(window, assets, font, slots[i], r, false, r.contains(mouse), false);
        } else {
            UiSystem::drawPanel(window, assets, r, false, 150);
            UiSystem::drawCenteredText(window, font, lm.get("HUD_EMPTY_SLOT"), r, 13, UiTheme::textMuted(), false);
        }
    }

    if (!tooltipTitle.empty()) {
        UiSystem::drawTooltip(window, assets, font, mouse, tooltipTitle, tooltipLines, tooltipCol);
    }
}

} // namespace crawl
