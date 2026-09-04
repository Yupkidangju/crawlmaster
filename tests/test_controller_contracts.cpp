#include "controller/CharacterInfoState.hpp"
#include "controller/CharacterCreationState.hpp"
#include "controller/CombatState.hpp"
#include "controller/DungeonState.hpp"
#include "controller/GameOverState.hpp"
#include "controller/SettingsState.hpp"
#include "controller/ShutdownState.hpp"
#include "controller/QuestJournalState.hpp"
#include "controller/TitleState.hpp"
#include "controller/TownState.hpp"
#include "controller/VictoryState.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "model/ItemFactory.hpp"

#include <chrono>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace crawl {

class ControllerTestAccess {
public:
    static const std::string& titleStatus(const TitleState& state) { return state.m_statusMessage; }

    static bool moveDungeonToTileAndCheck(DungeonState& state, TileType tile) {
        for (int x = 0; x < DungeonMap::MAP_WIDTH; ++x) {
            for (int y = 0; y < DungeonMap::MAP_HEIGHT; ++y) {
                if (state.map().getTile(x, y) == tile) {
                    state.map().setPlayerPos(x, y);
                    return state.checkCurrentTileLog();
                }
            }
        }
        return false;
    }

    static int dungeonFloor(const DungeonState& state) { return state.m_floorNumber; }
    static TileType dungeonTile(const DungeonState& state, int x, int y) {
        return state.map().getTile(x, y);
    }
    static void markDungeonVisited(DungeonState& state, int x, int y) {
        state.map().setVisited(x, y, true);
        state.m_worldDirty = true;
    }

    static bool moveDungeonToTileAndInteract(DungeonState& state, TileType tile) {
        for (int x = 0; x < DungeonMap::MAP_WIDTH; ++x) {
            for (int y = 0; y < DungeonMap::MAP_HEIGHT; ++y) {
                if (state.map().getTile(x, y) == tile) {
                    state.map().setPlayerPos(x, y);
                    return state.interactCurrentTile();
                }
            }
        }
        return false;
    }

    static bool moveDungeonToObjectAndInteract(DungeonState& state, const std::string& objectId) {
        auto* object = state.m_game.getParty().getWorld().findObject(objectId);
        if (!object || object->floor != state.m_floorNumber) return false;
        state.map().setPlayerPos(object->x, object->y);
        return state.interactCurrentTile();
    }

    static bool moveDungeonToObjectAndCheck(DungeonState& state, const std::string& objectId) {
        auto* object = state.m_game.getParty().getWorld().findObject(objectId);
        if (!object || object->floor != state.m_floorNumber) return false;
        state.map().setPlayerPos(object->x, object->y);
        return state.checkCurrentTileLog();
    }

    static void defeatAllFoesAndAdvance(CombatState& state) {
        for (const auto& foe : state.m_foes) {
            if (foe) foe->takeDamage(foe->getMaxHp());
        }
        state.nextTurn();
    }

    static bool isPlayerTurn(const CombatState& state) {
        return !state.m_turnOrder.empty() &&
               !state.m_turnOrder[state.m_currentTurnIdx].isMonster;
    }

    static void setTurnOrder(CombatState& state, std::vector<TurnEntity> order, int currentIndex = 0) {
        state.m_turnOrder = std::move(order);
        state.m_currentTurnIdx = currentIndex;
    }

    static void addFoe(CombatState& state, std::shared_ptr<Monster> monster) {
        state.m_foes.push_back(std::move(monster));
    }

    static const std::vector<std::shared_ptr<Monster>>& foes(const CombatState& state) {
        return state.m_foes;
    }

    static void advanceTurn(CombatState& state) { state.nextTurn(); }
    static void attemptEscape(CombatState& state) { state.performEscapeAttempt(); }

    static bool autoStepTo(DungeonState& state, int targetX, int targetY) {
        static constexpr std::array<std::pair<int, int>, 4> deltas = {
            std::pair{0, -1}, std::pair{1, 0}, std::pair{0, 1}, std::pair{-1, 0}};
        for (const auto& [dx, dy] : deltas) {
            const int startX = targetX + dx;
            const int startY = targetY + dy;
            if (!state.map().isWalkable(startX, startY)) continue;
            state.map().setPlayerPos(startX, startY);
            state.m_autoPath = {{targetX, targetY}};
            state.m_autoPathIndex = 0;
            state.stepAutoMove();
            return true;
        }
        return false;
    }

    static CharacterCreationStage creationStage(const CharacterCreationState& state) {
        return state.m_stage;
    }

    static int creationRemainingPoints(const CharacterCreationState& state) {
        return state.m_draft.remainingPoints();
    }

    static int creationSelectedCost(const CharacterCreationState& state) {
        static constexpr std::array<Ability, 6> abilities = {
            Ability::STRENGTH, Ability::DEXTERITY, Ability::CONSTITUTION,
            Ability::INTELLIGENCE, Ability::WISDOM, Ability::CHARISMA};
        return state.m_draft.increaseCost(abilities[static_cast<std::size_t>(state.m_abilityRow)]);
    }

    static std::string characterStatus(const CharacterInfoState& state, const Character& character) {
        return state.statusSummary(character);
    }

    static void useSelectedConsumable(CharacterInfoState& state) {
        state.useSelectedConsumable();
    }
};

} // namespace crawl

namespace {

int failures = 0;
void check(bool condition, const char* expression, int line) {
    if (condition) return;
    ++failures;
    std::cerr << "[Failure] line " << line << ": " << expression << '\n';
}
#define CHECK(expression) check(static_cast<bool>(expression), #expression, __LINE__)

sf::Event key(sf::Keyboard::Key code) {
    sf::Event event{};
    event.type = sf::Event::KeyPressed;
    event.key.code = code;
    return event;
}

sf::Event text(sf::Uint32 unicode) {
    sf::Event event{};
    event.type = sf::Event::TextEntered;
    event.text.unicode = unicode;
    return event;
}

std::shared_ptr<crawl::Character> character(const std::string& name,
                                            crawl::CharacterClass characterClass,
                                            int strength, int dexterity,
                                            int hp, int maxHp) {
    const bool spellcaster = characterClass == crawl::CharacterClass::MAGE ||
                             characterClass == crawl::CharacterClass::CLERIC;
    const nlohmann::json fixture = {
        {"name", name}, {"class", static_cast<int>(characterClass)}, {"level", 1}, {"xp", 0},
        {"hp", hp}, {"maxHp", maxHp},
        {"spellSlots", spellcaster ? 2 : 0}, {"maxSpellSlots", spellcaster ? 2 : 0},
        {"poisonTurns", 0}, {"paralysisTurns", 0},
        {"abilities", {{"strength", strength}, {"dexterity", dexterity},
            {"constitution", 10}, {"intelligence", 10}, {"wisdom", 10}, {"charisma", 10}}},
        {"equipment", {{"weapon", ""}, {"armor", ""}, {"shield", ""}}}
    };
    return std::shared_ptr<crawl::Character>(crawl::Character::fromJson(fixture));
}

void testDeferredTpkReplacesRootAndRestoresTownCheckpoint(crawl::Game& game) {
    auto& party = game.getParty();
    party.resetToDefault();
    auto savedMember = character("Saved", crawl::CharacterClass::WARRIOR, 15, 10, 10, 10);
    CHECK(party.addMember(savedMember));
    CHECK(party.saveToFile());
    const auto checkpointSeed = party.getLastSessionSeed();
    const auto checkpointDraws = party.getSessionRngDrawCount();
    savedMember->takeDamage(savedMember->getMaxHp());

    crawl::SessionRng::reseedGlobal(1001U);
    game.getStates().replaceAll(std::make_unique<crawl::TownState>(game));
    game.getStates().pushState(std::make_unique<crawl::CombatState>(game, crawl::EncounterTier::EARLY));
    CHECK(game.getStates().size() == 2);
    auto* combat = dynamic_cast<crawl::CombatState*>(game.getStates().getCurrentState());
    CHECK(combat != nullptr);
    CHECK(savedMember->isDead());
    CHECK(dynamic_cast<crawl::CombatState*>(game.getStates().getCurrentState()) == combat);
    if (combat) combat->update(sf::Time::Zero);

    CHECK(game.getStates().size() == 1);
    CHECK(dynamic_cast<crawl::GameOverState*>(game.getStates().getCurrentState()) != nullptr);
    CHECK(party.getMemberCount() == 1);
    CHECK(!party.getMember(0)->isDead());
    CHECK(crawl::SessionRng::global().seed() == checkpointSeed);
    CHECK(crawl::SessionRng::global().drawCount() == checkpointDraws);
}

void testDungeonReturnCommitsAndFailureStays(crawl::Game& game) {
    auto& party = game.getParty();
    CHECK(party.startNewGame());
    party.addGold(25);
    game.getStates().replaceAll(std::make_unique<crawl::DungeonState>(game));
    auto* dungeon = dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState());
    CHECK(dungeon != nullptr);
    if (!dungeon) return;
    CHECK(!crawl::ControllerTestAccess::moveDungeonToTileAndCheck(*dungeon, crawl::TileType::UPSTAIRS));
    dungeon->handleInput(key(sf::Keyboard::Escape));
    CHECK(dynamic_cast<crawl::TownState*>(game.getStates().getCurrentState()) != nullptr);
    crawl::Party reloaded;
    CHECK(reloaded.loadFromFile());
    CHECK(reloaded.getGold() == 125);

    party.addGold(10);
    game.getStates().replaceAll(std::make_unique<crawl::DungeonState>(game));
    dungeon = dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState());
    CHECK(dungeon != nullptr);
    if (!dungeon) return;
    const std::filesystem::path blockedTemporary = crawl::Party::getDefaultSavePath() + ".tmp";
    std::filesystem::create_directory(blockedTemporary);
    dungeon->handleInput(key(sf::Keyboard::Escape));
    CHECK(dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState()) == dungeon);
    std::filesystem::remove_all(blockedTemporary);
}

void testBossVictoryCommitsOnceAndBlocksReentry(crawl::Game& game) {
    auto& party = game.getParty();
    CHECK(party.startNewGame());
    CHECK(party.addMember(character("BossHero", crawl::CharacterClass::WARRIOR, 18, 18, 50, 50)));
    CHECK(party.saveToFile());
    const std::size_t inventoryBefore = party.getInventory().size();

    game.getStates().replaceAll(std::make_unique<crawl::TownState>(game));
    game.getStates().pushState(
        std::make_unique<crawl::CombatState>(game, crawl::EncounterTier::LATE, true));
    auto* combat = dynamic_cast<crawl::CombatState*>(game.getStates().getCurrentState());
    CHECK(combat != nullptr);
    if (!combat) return;
    crawl::ControllerTestAccess::defeatAllFoesAndAdvance(*combat);
    CHECK(dynamic_cast<crawl::VictoryState*>(game.getStates().getCurrentState()) != nullptr);

    crawl::Party completed;
    CHECK(completed.loadFromFile());
    CHECK(completed.isCampaignCompleted());
    CHECK(completed.getInventory().size() == inventoryBefore + 2);

    game.getParty().loadFromFile();
    game.getStates().replaceAll(std::make_unique<crawl::DungeonState>(game));
    auto* dungeon = dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState());
    CHECK(dungeon != nullptr);
    if (!dungeon) return;
    CHECK(crawl::ControllerTestAccess::moveDungeonToTileAndInteract(*dungeon, crawl::TileType::DOWNSTAIRS));
    CHECK(crawl::ControllerTestAccess::moveDungeonToTileAndInteract(*dungeon, crawl::TileType::DOWNSTAIRS));
    CHECK(crawl::ControllerTestAccess::dungeonFloor(*dungeon) == 3);
    CHECK(!crawl::ControllerTestAccess::moveDungeonToTileAndCheck(*dungeon, crawl::TileType::BOSS_GATE));
    CHECK(game.getStates().size() == 1);
    CHECK(dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState()) == dungeon);
}

void testTitleKeepsDurabilityWarning(crawl::Game& game) {
    const std::filesystem::path savePath = crawl::Party::getDefaultSavePath();
    std::filesystem::remove(savePath);
    std::filesystem::remove(savePath.string() + ".bak");
    game.getStates().replaceAll(std::make_unique<crawl::TitleState>(game));
    auto* title = dynamic_cast<crawl::TitleState*>(game.getStates().getCurrentState());
    CHECK(title != nullptr);
    if (!title) return;
    crawl::Persistence::setPostCommitSyncFailureForTests(true);
    title->handleInput(key(sf::Keyboard::Enter));
    title->handleInput(key(sf::Keyboard::Enter));
    crawl::Persistence::setPostCommitSyncFailureForTests(false);
    CHECK(dynamic_cast<crawl::TitleState*>(game.getStates().getCurrentState()) == title);
    CHECK(crawl::ControllerTestAccess::titleStatus(*title) ==
          crawl::LocalizationManager::getInstance().get("TITLE_DURABILITY_UNKNOWN"));
}

void testNewGameAlwaysRequiresConfirmationWithoutSave(crawl::Game& game) {
    const std::filesystem::path savePath = crawl::Party::getDefaultSavePath();
    std::filesystem::remove(savePath);
    std::filesystem::remove(savePath.string() + ".bak");
    auto& party = game.getParty();
    party.resetToDefault();
    party.addGold(10);
    game.getStates().replaceAll(std::make_unique<crawl::TitleState>(game));
    auto* title = dynamic_cast<crawl::TitleState*>(game.getStates().getCurrentState());
    CHECK(title != nullptr);
    if (!title) return;
    title->handleInput(key(sf::Keyboard::Enter));
    CHECK(dynamic_cast<crawl::TitleState*>(game.getStates().getCurrentState()) == title);
    CHECK(!std::filesystem::exists(savePath));
    CHECK(party.getGold() == 110);
    title->handleInput(key(sf::Keyboard::Escape));
    CHECK(!std::filesystem::exists(savePath));
    CHECK(party.getGold() == 110);
}

void testContinueCompletedCampaignRoutesToVictory(crawl::Game& game) {
    auto& party = game.getParty();
    party.resetToDefault();
    party.setCampaignCompleted(true);
    CHECK(party.saveToFile());
    game.getStates().replaceAll(std::make_unique<crawl::TitleState>(game));
    auto* title = dynamic_cast<crawl::TitleState*>(game.getStates().getCurrentState());
    CHECK(title != nullptr);
    if (!title) return;
    title->handleInput(key(sf::Keyboard::Down));
    title->handleInput(key(sf::Keyboard::Enter));
    CHECK(dynamic_cast<crawl::VictoryState*>(game.getStates().getCurrentState()) != nullptr);
}

void testContinueRestoresProductionRngCheckpoint(crawl::Game& game) {
    auto& party = game.getParty();
    party.resetToDefault();
    constexpr std::uint32_t savedSeed = 0x51A7E123U;
    crawl::SessionRng::reseedGlobal(savedSeed);
    for (int index = 0; index < 17; ++index) {
        static_cast<void>(crawl::SessionRng::global().rollRange(1, 37));
    }
    CHECK(party.saveToFile());
    const std::uint64_t savedDrawCount = party.getSessionRngDrawCount();
    CHECK(savedDrawCount == crawl::SessionRng::global().drawCount());
    crawl::SessionRng expected(savedSeed, savedDrawCount);

    crawl::SessionRng::reseedGlobal(7U);
    game.getStates().replaceAll(std::make_unique<crawl::TitleState>(game));
    auto* title = dynamic_cast<crawl::TitleState*>(game.getStates().getCurrentState());
    CHECK(title != nullptr);
    if (!title) return;
    title->handleInput(key(sf::Keyboard::Down));
    title->handleInput(key(sf::Keyboard::Enter));

    CHECK(dynamic_cast<crawl::TownState*>(game.getStates().getCurrentState()) != nullptr);
    CHECK(crawl::SessionRng::global().seed() == savedSeed);
    CHECK(crawl::SessionRng::global().drawCount() == savedDrawCount);
    for (int index = 0; index < 8; ++index) {
        CHECK(crawl::SessionRng::global().rollRange(1, 10'000) ==
              expected.rollRange(1, 10'000));
    }
}

void testCharacterCreationRequiresIdentityPointsAndConfirm(crawl::Game& game) {
    auto& party = game.getParty();
    CHECK(party.startNewGame());
    game.getStates().replaceAll(std::make_unique<crawl::TownState>(game));
    auto* town = dynamic_cast<crawl::TownState*>(game.getStates().getCurrentState());
    CHECK(town != nullptr);
    if (!town) return;
    town->handleInput(key(sf::Keyboard::Num1));
    town->handleInput(key(sf::Keyboard::Num1));
    auto* creation = dynamic_cast<crawl::CharacterCreationState*>(game.getStates().getCurrentState());
    CHECK(creation != nullptr);
    if (!creation) return;
    CHECK(party.getMemberCount() == 0);
    creation->handleInput(text('A'));
    creation->handleInput(text('r'));
    creation->handleInput(text('i'));
    creation->handleInput(text('a'));
    creation->handleInput(key(sf::Keyboard::Enter));
    CHECK(crawl::ControllerTestAccess::creationStage(*creation) ==
          crawl::CharacterCreationStage::ABILITIES);
    CHECK(party.getMemberCount() == 0);

    int stalledRows = 0;
    while (crawl::ControllerTestAccess::creationRemainingPoints(*creation) > 0 && stalledRows < 18) {
        const int remaining = crawl::ControllerTestAccess::creationRemainingPoints(*creation);
        const int cost = crawl::ControllerTestAccess::creationSelectedCost(*creation);
        if (cost > 0 && cost <= remaining) {
            creation->handleInput(key(sf::Keyboard::Right));
            stalledRows = 0;
        } else {
            creation->handleInput(key(sf::Keyboard::Down));
            ++stalledRows;
        }
    }
    CHECK(crawl::ControllerTestAccess::creationRemainingPoints(*creation) == 0);
    creation->handleInput(key(sf::Keyboard::Enter));
    CHECK(crawl::ControllerTestAccess::creationStage(*creation) ==
          crawl::CharacterCreationStage::CONFIRM);
    CHECK(party.getMemberCount() == 0);
    creation->handleInput(key(sf::Keyboard::Enter));
    CHECK(party.getMemberCount() == 1);
    CHECK(party.getMember(0)->getName() == "Aria");
}

void testFailedAutoSwapPreservesGreatsword(crawl::Game& game) {
    auto& party = game.getParty();
    party.resetToDefault();
    auto warrior = character("Swap", crawl::CharacterClass::WARRIOR, 13, 10, 10, 10);
    CHECK(warrior->equip(std::dynamic_pointer_cast<crawl::Equipment>(
        crawl::ItemFactory::createItem("wpn_greatsword"))));
    CHECK(party.addMember(warrior));
    party.addItem(crawl::ItemFactory::createItem("shd_tower"));
    crawl::CharacterInfoState state(game, false);
    state.handleInput(key(sf::Keyboard::Down));
    state.handleInput(key(sf::Keyboard::Down));
    state.handleInput(key(sf::Keyboard::Down));
    state.handleInput(key(sf::Keyboard::Enter));
    CHECK(warrior->getEquippedItem(crawl::EquipSlot::WEAPON) != nullptr);
    CHECK(warrior->getEquippedItem(crawl::EquipSlot::WEAPON)->getId() == "wpn_greatsword");
    CHECK(party.getInventory().back()->getId() == "shd_tower");
}

void testCombatItemAndAllySkillNeedPreviewConfirm(crawl::Game& game) {
    auto& party = game.getParty();
    party.resetToDefault();
    auto cleric = character("Cleric", crawl::CharacterClass::CLERIC, 10, 30, 20, 20);
    auto ally = character("Ally", crawl::CharacterClass::WARRIOR, 15, 10, 10, 20);
    CHECK(party.addMember(cleric));
    CHECK(party.addMember(ally));

    crawl::SessionRng::reseedGlobal(12345U);
    crawl::CombatState itemCombat(game, crawl::EncounterTier::EARLY);
    itemCombat.update(sf::Time::Zero);
    const auto inventoryBefore = party.getInventory().size();
    cleric->takeDamage(5);
    const int hpBeforeItem = cleric->getHp();
    itemCombat.handleInput(key(sf::Keyboard::Num3));
    itemCombat.handleInput(key(sf::Keyboard::Num1));
    itemCombat.handleInput(key(sf::Keyboard::Enter));
    CHECK(cleric->getHp() == hpBeforeItem);
    CHECK(party.getInventory().size() == inventoryBefore);
    itemCombat.handleInput(key(sf::Keyboard::Escape));
    itemCombat.handleInput(key(sf::Keyboard::Escape));
    itemCombat.handleInput(key(sf::Keyboard::Escape));
    CHECK(cleric->getHp() == hpBeforeItem);
    CHECK(party.getInventory().size() == inventoryBefore);
    itemCombat.handleInput(key(sf::Keyboard::Num3));
    itemCombat.handleInput(key(sf::Keyboard::Num1));
    itemCombat.handleInput(key(sf::Keyboard::Enter));
    itemCombat.handleInput(key(sf::Keyboard::Enter));
    CHECK(cleric->getHp() > hpBeforeItem);
    CHECK(party.getInventory().size() == inventoryBefore - 1);

    party.resetToDefault();
    cleric = character("Cleric", crawl::CharacterClass::CLERIC, 10, 30, 20, 20);
    ally = character("Ally", crawl::CharacterClass::WARRIOR, 15, 10, 10, 20);
    CHECK(party.addMember(cleric));
    CHECK(party.addMember(ally));
    crawl::SessionRng::reseedGlobal(54321U);
    crawl::CombatState skillCombat(game, crawl::EncounterTier::EARLY);
    skillCombat.update(sf::Time::Zero);
    const int slotsBefore = cleric->getSpellSlots();
    const int allyHpBefore = ally->getHp();
    skillCombat.handleInput(key(sf::Keyboard::Num2));
    skillCombat.handleInput(key(sf::Keyboard::Num1));
    skillCombat.handleInput(key(sf::Keyboard::Right));
    skillCombat.handleInput(key(sf::Keyboard::Enter));
    CHECK(cleric->getSpellSlots() == slotsBefore);
    CHECK(ally->getHp() == allyHpBefore);
    skillCombat.handleInput(key(sf::Keyboard::Escape));
    skillCombat.handleInput(key(sf::Keyboard::Escape));
    CHECK(cleric->getSpellSlots() == slotsBefore);
    CHECK(ally->getHp() == allyHpBefore);
    skillCombat.handleInput(key(sf::Keyboard::Num2));
    skillCombat.handleInput(key(sf::Keyboard::Num1));
    skillCombat.handleInput(key(sf::Keyboard::Right));
    skillCombat.handleInput(key(sf::Keyboard::Enter));
    skillCombat.handleInput(key(sf::Keyboard::Enter));
    CHECK(cleric->getSpellSlots() == slotsBefore - 1);
    CHECK(ally->getHp() > allyHpBefore);
}

void testOptionsKeyWorksAcrossTownAndCombatOverlays(crawl::Game& game) {
    auto& party = game.getParty();
    CHECK(party.startNewGame());
    CHECK(party.addMember(character("Options", crawl::CharacterClass::WARRIOR, 18, 30, 20, 20)));

    const std::array<std::vector<sf::Keyboard::Key>, 6> townPaths = {{
        {sf::Keyboard::Num1},
        {sf::Keyboard::Num2},
        {sf::Keyboard::Num2, sf::Keyboard::Num1},
        {sf::Keyboard::Num2, sf::Keyboard::Num2},
        {sf::Keyboard::Num3},
        {sf::Keyboard::Num4}
    }};
    for (const auto& path : townPaths) {
        game.getStates().replaceAll(std::make_unique<crawl::TownState>(game));
        auto* town = dynamic_cast<crawl::TownState*>(game.getStates().getCurrentState());
        CHECK(town != nullptr);
        if (!town) continue;
        for (const auto input : path) town->handleInput(key(input));
        town->handleInput(key(sf::Keyboard::O));
        CHECK(dynamic_cast<crawl::SettingsState*>(game.getStates().getCurrentState()) != nullptr);
        if (dynamic_cast<crawl::SettingsState*>(game.getStates().getCurrentState())) {
            game.getStates().popState();
        }
    }

    party.addItem(crawl::ItemFactory::createItem("pot_strength"));
    crawl::SessionRng::reseedGlobal(9191U);
    game.getStates().replaceAll(
        std::make_unique<crawl::CombatState>(game, crawl::EncounterTier::EARLY));
    auto* combat = dynamic_cast<crawl::CombatState*>(game.getStates().getCurrentState());
    CHECK(combat != nullptr);
    if (!combat) return;
    for (int turn = 0; turn < 12 && !crawl::ControllerTestAccess::isPlayerTurn(*combat); ++turn) {
        combat->update(sf::seconds(1.0f));
    }
    CHECK(crawl::ControllerTestAccess::isPlayerTurn(*combat));
    combat->handleInput(key(sf::Keyboard::Num3));
    combat->handleInput(key(sf::Keyboard::O));
    CHECK(dynamic_cast<crawl::SettingsState*>(game.getStates().getCurrentState()) != nullptr);
    if (dynamic_cast<crawl::SettingsState*>(game.getStates().getCurrentState())) {
        game.getStates().popState();
    }
    CHECK(dynamic_cast<crawl::CombatState*>(game.getStates().getCurrentState()) == combat);

    combat->handleInput(key(sf::Keyboard::Escape));
    combat->handleInput(key(sf::Keyboard::Num2));
    combat->handleInput(key(sf::Keyboard::O));
    CHECK(dynamic_cast<crawl::SettingsState*>(game.getStates().getCurrentState()) != nullptr);
}

void testPersistentFloorTravelAndObjectiveInteractions(crawl::Game& game) {
    auto& party = game.getParty();
    crawl::SessionRng::reseedGlobal(424242U);
    CHECK(party.startNewGame());
    CHECK(party.addMember(character("Explorer", crawl::CharacterClass::WARRIOR, 18, 20, 30, 30)));
    party.acceptQuest(crawl::Quest::createCanonical("qst_recover_moon_seal"));
    party.acceptQuest(crawl::Quest::createCanonical("qst_defeat_crypt_warden"));
    party.acceptQuest(crawl::Quest::createCanonical("qst_find_missing_scout"));
    CHECK(party.saveToFile());

    game.getStates().replaceAll(std::make_unique<crawl::DungeonState>(game));
    auto* dungeon = dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState());
    CHECK(dungeon != nullptr);
    if (!dungeon) return;
    CHECK(crawl::ControllerTestAccess::dungeonFloor(*dungeon) == 1);
    CHECK(party.addKeyItem("key_moon_seal"));
    CHECK(crawl::ControllerTestAccess::moveDungeonToObjectAndInteract(*dungeon, "obj_moon_seal"));
    CHECK(party.hasKeyItem("key_moon_seal"));
    CHECK(party.getQuest("qst_recover_moon_seal")->isReadyToReport());
    CHECK(party.getWorld().findObject("obj_moon_seal")->state == crawl::WorldObjectState::RESOLVED);

    CHECK(crawl::ControllerTestAccess::moveDungeonToTileAndInteract(*dungeon, crawl::TileType::DOWNSTAIRS));
    CHECK(crawl::ControllerTestAccess::dungeonFloor(*dungeon) == 2);
    CHECK(crawl::ControllerTestAccess::moveDungeonToObjectAndCheck(*dungeon, "obj_crypt_warden"));
    auto* combat = dynamic_cast<crawl::CombatState*>(game.getStates().getCurrentState());
    CHECK(combat != nullptr);
    if (!combat) return;
    crawl::ControllerTestAccess::defeatAllFoesAndAdvance(*combat);
    CHECK(dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState()) == dungeon);
    CHECK(party.getQuest("qst_defeat_crypt_warden")->isReadyToReport());
    CHECK(party.getWorld().findObject("obj_crypt_warden")->state == crawl::WorldObjectState::RESOLVED);
    CHECK(crawl::ControllerTestAccess::moveDungeonToTileAndInteract(*dungeon, crawl::TileType::DOWNSTAIRS));
    CHECK(crawl::ControllerTestAccess::dungeonFloor(*dungeon) == 3);
    CHECK(crawl::ControllerTestAccess::moveDungeonToObjectAndInteract(*dungeon, "obj_missing_scout"));
    CHECK(party.getQuest("qst_find_missing_scout")->isReadyToReport());
    CHECK(party.getWorld().findObject("obj_missing_scout")->state == crawl::WorldObjectState::RESOLVED);

    crawl::Party loaded;
    CHECK(loaded.loadFromFile());
    CHECK(loaded.hasKeyItem("key_moon_seal"));
    CHECK(loaded.getWorld().findObject("obj_crypt_warden")->state == crawl::WorldObjectState::RESOLVED);
    CHECK(loaded.getWorld().findObject("obj_missing_scout")->state == crawl::WorldObjectState::RESOLVED);
}

void testQuestReportRollsBackOnSaveFailure(crawl::Game& game) {
    auto& party = game.getParty();
    CHECK(party.startNewGame());
    party.acceptQuest(crawl::Quest::createCanonical("qst_recover_moon_seal"));
    CHECK(party.markQuestObjectiveComplete("qst_recover_moon_seal"));
    CHECK(party.addKeyItem("key_moon_seal"));
    party.getWorld().findObject("obj_moon_seal")->state = crawl::WorldObjectState::RESOLVED;
    CHECK(party.saveToFile());
    const int goldBefore = party.getGold();

    game.getStates().replaceAll(std::make_unique<crawl::TownState>(game));
    auto* town = dynamic_cast<crawl::TownState*>(game.getStates().getCurrentState());
    CHECK(town != nullptr);
    if (!town) return;
    town->handleInput(key(sf::Keyboard::Num4));
    const auto rngSeedBefore = crawl::SessionRng::global().seed();
    const auto rngDrawsBefore = crawl::SessionRng::global().drawCount();
    std::filesystem::remove(crawl::Party::getDefaultSavePath());
    std::filesystem::remove(crawl::Party::getDefaultSavePath() + ".bak");
    const std::filesystem::path blockedTemporary = crawl::Party::getDefaultSavePath() + ".tmp";
    std::filesystem::create_directory(blockedTemporary);
    town->handleInput(key(sf::Keyboard::Enter));
    std::filesystem::remove_all(blockedTemporary);
    CHECK(party.getGold() == goldBefore);
    CHECK(party.hasQuest("qst_recover_moon_seal"));
    CHECK(party.hasKeyItem("key_moon_seal"));
    CHECK(!party.isQuestCompleted("qst_recover_moon_seal"));
    CHECK(crawl::SessionRng::global().seed() == rngSeedBefore);
    CHECK(crawl::SessionRng::global().drawCount() == rngDrawsBefore);
    town->handleInput(key(sf::Keyboard::Enter));
    CHECK(party.getGold() == goldBefore + 75);
    CHECK(!party.hasQuest("qst_recover_moon_seal"));
    CHECK(!party.hasKeyItem("key_moon_seal"));
    CHECK(party.isQuestCompleted("qst_recover_moon_seal"));
}

void testQuestJournalOpensFromTownAndDungeon(crawl::Game& game) {
    auto& party = game.getParty();
    CHECK(party.startNewGame());
    party.acceptQuest(crawl::Quest::createCanonical("qst_find_missing_scout"));
    CHECK(party.saveToFile());

    game.getStates().replaceAll(std::make_unique<crawl::TownState>(game));
    auto* town = dynamic_cast<crawl::TownState*>(game.getStates().getCurrentState());
    CHECK(town != nullptr);
    if (!town) return;
    town->handleInput(key(sf::Keyboard::Q));
    CHECK(dynamic_cast<crawl::QuestJournalState*>(game.getStates().getCurrentState()) != nullptr);
    game.getStates().popState();

    game.getStates().replaceAll(std::make_unique<crawl::DungeonState>(game));
    auto* dungeon = dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState());
    CHECK(dungeon != nullptr);
    if (!dungeon) return;
    dungeon->handleInput(key(sf::Keyboard::Q));
    CHECK(dynamic_cast<crawl::QuestJournalState*>(game.getStates().getCurrentState()) != nullptr);
}

void testContinueCommitsSchemaV3WorldMigration(crawl::Game& game) {
    auto& party = game.getParty();
    crawl::SessionRng::reseedGlobal(515151U);
    CHECK(party.startNewGame());
    nlohmann::json legacy;
    {
        std::ifstream input(crawl::Party::getDefaultSavePath());
        input >> legacy;
    }
    legacy["schemaVersion"] = 3;
    legacy.erase("world");
    legacy.erase("keyItems");
    {
        std::ofstream output(crawl::Party::getDefaultSavePath(), std::ios::trunc);
        output << legacy.dump(4);
    }

    game.getStates().replaceAll(std::make_unique<crawl::TitleState>(game));
    auto* title = dynamic_cast<crawl::TitleState*>(game.getStates().getCurrentState());
    CHECK(title != nullptr);
    if (!title) return;
    title->handleInput(key(sf::Keyboard::Down));
    title->handleInput(key(sf::Keyboard::Enter));
    CHECK(dynamic_cast<crawl::TownState*>(game.getStates().getCurrentState()) != nullptr);
    nlohmann::json migrated;
    {
        std::ifstream input(crawl::Party::getDefaultSavePath());
        input >> migrated;
    }
    CHECK(migrated.at("schemaVersion") == 4);
    CHECK(migrated.contains("world"));
}

void testCombatSaveRollbackDoesNotInvalidateDungeonWorld(crawl::Game& game) {
    auto& party = game.getParty();
    CHECK(party.startNewGame());
    CHECK(party.addMember(character("Rollback", crawl::CharacterClass::WARRIOR, 18, 18, 50, 50)));
    party.getMember(0)->applyStrBuff(3, 4);
    party.getMember(0)->applyDexBuff(3, 4);
    party.getMember(0)->applyBless(4);
    party.acceptQuest(crawl::Quest::createCanonical("qst_defeat_crypt_warden"));
    CHECK(party.saveToFile());
    game.getStates().replaceAll(std::make_unique<crawl::DungeonState>(game));
    auto* dungeon = dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState());
    CHECK(dungeon != nullptr);
    if (!dungeon) return;
    CHECK(crawl::ControllerTestAccess::moveDungeonToTileAndInteract(*dungeon, crawl::TileType::DOWNSTAIRS));
    CHECK(crawl::ControllerTestAccess::moveDungeonToObjectAndCheck(*dungeon, "obj_crypt_warden"));
    auto* combat = dynamic_cast<crawl::CombatState*>(game.getStates().getCurrentState());
    CHECK(combat != nullptr);
    if (!combat) return;

    const std::filesystem::path blockedTemporary = crawl::Party::getDefaultSavePath() + ".tmp";
    std::filesystem::remove(crawl::Party::getDefaultSavePath());
    std::filesystem::remove(crawl::Party::getDefaultSavePath() + ".bak");
    std::filesystem::create_directory(blockedTemporary);
    const auto rollbackSeed = crawl::SessionRng::global().seed();
    const auto rollbackDraws = crawl::SessionRng::global().drawCount();
    crawl::ControllerTestAccess::defeatAllFoesAndAdvance(*combat);
    CHECK(dynamic_cast<crawl::CombatState*>(game.getStates().getCurrentState()) == combat);
    CHECK(crawl::SessionRng::global().seed() == rollbackSeed);
    CHECK(crawl::SessionRng::global().drawCount() == rollbackDraws);
    CHECK(party.getMember(0)->getStrBuffAmount() == 3);
    CHECK(party.getMember(0)->getDexBuffAmount() == 3);
    CHECK(party.getMember(0)->getBlessTurns() == 4);
    std::filesystem::remove_all(blockedTemporary);
    crawl::ControllerTestAccess::defeatAllFoesAndAdvance(*combat);
    CHECK(dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState()) == dungeon);
    CHECK(crawl::ControllerTestAccess::dungeonTile(*dungeon, 1, 1) == crawl::TileType::UPSTAIRS);
}

void testPoisonTurnStartDeathSkipsIndividualActions(crawl::Game& game) {
    auto& party = game.getParty();
    CHECK(party.startNewGame());
    auto dying = character("Dying", crawl::CharacterClass::WARRIOR, 10, 10, 1, 10);
    auto survivor = character("Survivor", crawl::CharacterClass::WARRIOR, 10, 10, 20, 20);
    dying->setPoison(1);
    CHECK(party.addMember(dying));
    CHECK(party.addMember(survivor));
    crawl::SessionRng::reseedGlobal(31337U);
    game.getStates().replaceAll(std::make_unique<crawl::CombatState>(game, crawl::EncounterTier::EARLY));
    auto* combat = dynamic_cast<crawl::CombatState*>(game.getStates().getCurrentState());
    CHECK(combat != nullptr);
    if (!combat) return;
    crawl::ControllerTestAccess::setTurnOrder(*combat, {
        {false, 1, 4}, {false, 0, 3}, {true, 0, 2}});
    crawl::ControllerTestAccess::advanceTurn(*combat);
    CHECK(dying->isDead());
    CHECK(crawl::ControllerTestAccess::isPlayerTurn(*combat));

    dying->rest();
    auto poisonedFoe = crawl::MonsterFactory::createMonster("mon_kobold");
    poisonedFoe->takeDamage(poisonedFoe->getMaxHp() - 1);
    poisonedFoe->setPoison(1);
    auto paralyzedFoe = crawl::MonsterFactory::createMonster("mon_goblin");
    paralyzedFoe->setParalysis(1);
    crawl::ControllerTestAccess::foes(*combat)[0]->takeDamage(
        crawl::ControllerTestAccess::foes(*combat)[0]->getMaxHp());
    crawl::ControllerTestAccess::addFoe(*combat, poisonedFoe);
    crawl::ControllerTestAccess::addFoe(*combat, paralyzedFoe);
    const int hpBefore = survivor->getHp();
    crawl::ControllerTestAccess::setTurnOrder(*combat, {
        {false, 1, 4}, {true, 1, 3}, {true, 2, 2}, {false, 0, 1}});
    crawl::ControllerTestAccess::advanceTurn(*combat);
    CHECK(poisonedFoe->isDead());
    CHECK(survivor->getHp() == hpBefore);
}

void testSuccessfulEscapeClearsAllCombatBuffs(crawl::Game& game) {
    auto& party = game.getParty();
    CHECK(party.startNewGame());
    auto runner = character("Runner", crawl::CharacterClass::ROGUE, 10, 30, 20, 20);
    runner->applyStrBuff(3, 5);
    runner->applyDexBuff(3, 5);
    runner->applyBless(5);
    CHECK(party.addMember(runner));
    game.getStates().replaceAll(std::make_unique<crawl::DungeonState>(game));
    game.getStates().pushState(std::make_unique<crawl::CombatState>(game, crawl::EncounterTier::EARLY));
    auto* combat = dynamic_cast<crawl::CombatState*>(game.getStates().getCurrentState());
    CHECK(combat != nullptr);
    if (!combat) return;
    crawl::ControllerTestAccess::setTurnOrder(*combat, {{false, 0, 20}});
    for (std::uint32_t seed = 1;; ++seed) {
        crawl::SessionRng candidate(seed);
        if (candidate.rollDie(20) >= 2) {
            crawl::SessionRng::reseedGlobal(seed);
            break;
        }
    }
    crawl::ControllerTestAccess::attemptEscape(*combat);
    CHECK(dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState()) != nullptr);
    CHECK(runner->getStrBuffAmount() == 0);
    CHECK(runner->getDexBuffAmount() == 0);
    CHECK(runner->getBlessTurns() == 0);
}

void testCharacterInfoShowsAllConditionsAndRejectsNoEffectConsumable(crawl::Game& game) {
    auto& party = game.getParty();
    CHECK(party.startNewGame());
    auto member = character("Status", crawl::CharacterClass::WARRIOR, 10, 10, 10, 10);
    member->setPoison(3);
    member->setParalysis(2);
    member->applyStrBuff(3, 4);
    member->applyDexBuff(3, 5);
    member->applyBless(6);
    CHECK(party.addMember(member));
    crawl::CharacterInfoState info(game, false);
    const std::string status = crawl::ControllerTestAccess::characterStatus(info, *member);
    auto& localization = crawl::LocalizationManager::getInstance();
    CHECK(status.find(localization.get("STATUS_POISON")) != std::string::npos);
    CHECK(status.find(localization.get("STATUS_PARALYSIS")) != std::string::npos);
    CHECK(status.find(localization.get("STATUS_STRENGTH")) != std::string::npos);
    CHECK(status.find(localization.get("STATUS_DEXTERITY")) != std::string::npos);
    CHECK(status.find(localization.get("STATUS_BLESS")) != std::string::npos);

    member->rest();
    const std::size_t inventoryBefore = party.getInventory().size();
    const auto rngBefore = crawl::SessionRng::global().drawCount();
    crawl::ControllerTestAccess::useSelectedConsumable(info);
    CHECK(party.getInventory().size() == inventoryBefore);
    CHECK(crawl::SessionRng::global().drawCount() == rngBefore);
}

void testShutdownRequiresExplicitResolutionAfterSaveFailure(crawl::Game& game) {
    auto& party = game.getParty();
    CHECK(party.startNewGame());
    const std::filesystem::path blockedTemporary = crawl::Party::getDefaultSavePath() + ".tmp";
    std::filesystem::create_directory(blockedTemporary);
    game.requestShutdown();
    CHECK(!game.isShutdownApproved());
    auto* shutdown = dynamic_cast<crawl::ShutdownState*>(game.getStates().getCurrentState());
    CHECK(shutdown != nullptr);
    std::filesystem::remove_all(blockedTemporary);
    if (shutdown) shutdown->handleInput(key(sf::Keyboard::Enter));
    CHECK(game.isShutdownApproved());

    crawl::Game durabilityUnknownGame(true);
    CHECK(durabilityUnknownGame.getParty().startNewGame());
    crawl::Persistence::setPostCommitSyncFailureForTests(true);
    durabilityUnknownGame.requestShutdown();
    crawl::Persistence::setPostCommitSyncFailureForTests(false);
    CHECK(!durabilityUnknownGame.isShutdownApproved());
    auto* unknownState = dynamic_cast<crawl::ShutdownState*>(
        durabilityUnknownGame.getStates().getCurrentState());
    CHECK(unknownState != nullptr);
    if (unknownState) unknownState->handleInput(key(sf::Keyboard::Escape));
    CHECK(durabilityUnknownGame.isShutdownApproved());
}

void testRecoveryPendingBlocksTpkAndTitleShutdownOverwrite() {
    const std::string previousSavePath = crawl::Party::getDefaultSavePath();
    const auto directory = std::filesystem::path(previousSavePath).parent_path() /
        "recovery-pending-controller";
    const auto savePath = directory / "save.json";
    std::filesystem::create_directories(directory);
    crawl::Party::setDefaultSavePath(savePath.string());

    {
        crawl::Game tpkGame(true);
        auto& party = tpkGame.getParty();
        CHECK(party.startNewGame());
        auto member = character("DeadCheckpoint", crawl::CharacterClass::WARRIOR,
                                18, 18, 20, 20);
        CHECK(party.addMember(member));
        CHECK(party.saveToFile());
        member->takeDamage(member->getMaxHp());
        std::ofstream(savePath, std::ios::binary | std::ios::trunc) << "{ corrupt-primary";
        std::filesystem::remove(savePath.string() + ".bak");

        tpkGame.getStates().replaceAll(
            std::make_unique<crawl::CombatState>(tpkGame, crawl::EncounterTier::EARLY));
        auto* combat = dynamic_cast<crawl::CombatState*>(tpkGame.getStates().getCurrentState());
        CHECK(combat != nullptr);
        if (combat) crawl::ControllerTestAccess::advanceTurn(*combat);
        CHECK(dynamic_cast<crawl::GameOverState*>(tpkGame.getStates().getCurrentState()) != nullptr);
        CHECK(member->isDead());
        CHECK(party.isRecoveryPending());
        CHECK(!std::filesystem::exists(savePath));
        CHECK(party.saveToFile().status == crawl::PersistenceStatus::RecoveryPending);
        CHECK(!std::filesystem::exists(savePath));

        tpkGame.requestShutdown();
        auto* shutdown = dynamic_cast<crawl::ShutdownState*>(tpkGame.getStates().getCurrentState());
        CHECK(shutdown != nullptr);
        CHECK(!tpkGame.isShutdownApproved());
        CHECK(!std::filesystem::exists(savePath));
        if (shutdown) shutdown->handleInput(key(sf::Keyboard::Enter));
        CHECK(!tpkGame.isShutdownApproved());
        CHECK(party.isRecoveryPending());
        CHECK(!std::filesystem::exists(savePath));
        if (shutdown) shutdown->handleInput(key(sf::Keyboard::Escape));
        CHECK(tpkGame.isShutdownApproved());
        CHECK(!std::filesystem::exists(savePath));
    }

    {
        crawl::Game titleGame(true);
        auto& party = titleGame.getParty();
        CHECK(party.startNewGame());
        CHECK(party.addMember(character("TitleRecovery", crawl::CharacterClass::WARRIOR,
                                        18, 18, 20, 20)));
        CHECK(party.saveToFile());
        std::ofstream(savePath, std::ios::binary | std::ios::trunc) << "{ corrupt-title";
        std::filesystem::remove(savePath.string() + ".bak");
        titleGame.getStates().replaceAll(std::make_unique<crawl::TitleState>(titleGame));
        auto* title = dynamic_cast<crawl::TitleState*>(titleGame.getStates().getCurrentState());
        CHECK(title != nullptr);
        if (title) {
            title->handleInput(key(sf::Keyboard::Down));
            title->handleInput(key(sf::Keyboard::Enter));
        }
        CHECK(party.isRecoveryPending());
        CHECK(!std::filesystem::exists(savePath));
        titleGame.requestShutdown();
        CHECK(dynamic_cast<crawl::ShutdownState*>(titleGame.getStates().getCurrentState()) != nullptr);
        CHECK(!titleGame.isShutdownApproved());
        CHECK(!std::filesystem::exists(savePath));
    }

    {
        crawl::Party party;
        party.markRecoveryPending();
        party.resetToDefault();
        CHECK(party.isRecoveryPending());
        CHECK(party.saveToFile().status == crawl::PersistenceStatus::RecoveryPending);
        CHECK(party.startNewGame().status == crawl::PersistenceStatus::Saved);
        CHECK(!party.isRecoveryPending());
    }

    {
        crawl::Game successfulRetryGame(true);
        auto& party = successfulRetryGame.getParty();
        CHECK(party.startNewGame());
        party.markRecoveryPending();
        successfulRetryGame.requestShutdown();
        auto* shutdown = dynamic_cast<crawl::ShutdownState*>(
            successfulRetryGame.getStates().getCurrentState());
        CHECK(shutdown != nullptr);
        if (shutdown) shutdown->handleInput(key(sf::Keyboard::Enter));
        CHECK(successfulRetryGame.isShutdownApproved());
        CHECK(!party.isRecoveryPending());
    }

    std::filesystem::remove_all(directory);
    crawl::Party::setDefaultSavePath(previousSavePath);
}

void testEveryObjectiveQuestReportIsAtomicAndOneTime(crawl::Game& game) {
    struct Scenario { const char* id; const char* objectId; int boardIndex; int gold; };
    const std::array<Scenario, 3> scenarios = {{
        {"qst_recover_moon_seal", "obj_moon_seal", 0, 75},
        {"qst_defeat_crypt_warden", "obj_crypt_warden", 1, 150},
        {"qst_find_missing_scout", "obj_missing_scout", 2, 200},
    }};
    for (const auto& scenario : scenarios) {
        auto& party = game.getParty();
        CHECK(party.startNewGame());
        CHECK(party.addMember(character("Reporter", crawl::CharacterClass::WARRIOR, 18, 18, 20, 20)));
        auto quest = crawl::Quest::createCanonical(scenario.id);
        party.acceptQuest(quest);
        CHECK(party.markQuestObjectiveComplete(scenario.id));
        auto* object = party.getWorld().findObject(scenario.objectId);
        CHECK(object != nullptr);
        if (!object) return;
        object->state = crawl::WorldObjectState::RESOLVED;
        if (quest->getType() == crawl::QuestType::RETRIEVE_KEY_ITEM) {
            CHECK(party.addKeyItem(quest->getTargetId()));
        }
        CHECK(party.saveToFile());
        const int goldBefore = party.getGold();
        const std::size_t inventoryBefore = party.getInventory().size();

        game.getStates().replaceAll(std::make_unique<crawl::TownState>(game));
        auto* town = dynamic_cast<crawl::TownState*>(game.getStates().getCurrentState());
        CHECK(town != nullptr);
        if (!town) return;
        town->handleInput(key(sf::Keyboard::Num4));
        for (int index = 0; index < scenario.boardIndex; ++index) {
            town->handleInput(key(sf::Keyboard::Down));
        }
        std::filesystem::remove(crawl::Party::getDefaultSavePath());
        std::filesystem::remove(crawl::Party::getDefaultSavePath() + ".bak");
        const auto blocked = std::filesystem::path(crawl::Party::getDefaultSavePath() + ".tmp");
        std::filesystem::create_directory(blocked);
        const auto seedBefore = crawl::SessionRng::global().seed();
        const auto drawsBefore = crawl::SessionRng::global().drawCount();
        town->handleInput(key(sf::Keyboard::Enter));
        std::filesystem::remove_all(blocked);
        CHECK(party.hasQuest(scenario.id));
        CHECK(!party.isQuestCompleted(scenario.id));
        CHECK(party.getGold() == goldBefore);
        CHECK(party.getInventory().size() == inventoryBefore);
        CHECK(crawl::SessionRng::global().seed() == seedBefore);
        CHECK(crawl::SessionRng::global().drawCount() == drawsBefore);

        town->handleInput(key(sf::Keyboard::Enter));
        CHECK(!party.hasQuest(scenario.id));
        CHECK(party.isQuestCompleted(scenario.id));
        CHECK(party.getGold() == goldBefore + scenario.gold);
        const int completedGold = party.getGold();
        const std::size_t completedInventory = party.getInventory().size();
        town->handleInput(key(sf::Keyboard::Enter));
        CHECK(party.getGold() == completedGold);
        CHECK(party.getInventory().size() == completedInventory);

        crawl::Party reloaded;
        CHECK(reloaded.loadFromFile());
        CHECK(reloaded.isQuestCompleted(scenario.id));
        CHECK(!reloaded.hasQuest(scenario.id));
    }
}

void testFourCharacterCreationIndividualDeathAndTpkLifecycle(crawl::Game& game) {
    auto& party = game.getParty();
    CHECK(party.startNewGame());
    game.getStates().replaceAll(std::make_unique<crawl::TownState>(game));
    auto* town = dynamic_cast<crawl::TownState*>(game.getStates().getCurrentState());
    CHECK(town != nullptr);
    if (!town) return;
    town->handleInput(key(sf::Keyboard::Num1));
    const std::array<std::string, 4> names = {"Aria", "Borin", "Cira", "Dane"};
    for (const auto& name : names) {
        town->handleInput(key(sf::Keyboard::Num1));
        auto* creation = dynamic_cast<crawl::CharacterCreationState*>(game.getStates().getCurrentState());
        CHECK(creation != nullptr);
        if (!creation) return;
        for (const char characterCode : name) creation->handleInput(text(static_cast<sf::Uint32>(characterCode)));
        creation->handleInput(key(sf::Keyboard::Enter));
        for (int reroll = 0;
             reroll < 50 && crawl::ControllerTestAccess::creationRemainingPoints(*creation) > 0;
             ++reroll) {
            int stalledRows = 0;
            while (crawl::ControllerTestAccess::creationRemainingPoints(*creation) > 0 && stalledRows < 18) {
                const int remaining = crawl::ControllerTestAccess::creationRemainingPoints(*creation);
                const int cost = crawl::ControllerTestAccess::creationSelectedCost(*creation);
                if (cost > 0 && cost <= remaining) {
                    creation->handleInput(key(sf::Keyboard::Right));
                    stalledRows = 0;
                } else {
                    creation->handleInput(key(sf::Keyboard::Down));
                    ++stalledRows;
                }
            }
            if (crawl::ControllerTestAccess::creationRemainingPoints(*creation) > 0) {
                creation->handleInput(key(sf::Keyboard::R));
            }
        }
        CHECK(crawl::ControllerTestAccess::creationRemainingPoints(*creation) == 0);
        if (crawl::ControllerTestAccess::creationRemainingPoints(*creation) != 0) return;
        creation->handleInput(key(sf::Keyboard::Enter));
        creation->handleInput(key(sf::Keyboard::Enter));
        CHECK(dynamic_cast<crawl::TownState*>(game.getStates().getCurrentState()) == town);
    }
    CHECK(party.getMemberCount() == 4);
    CHECK(party.saveToFile());
    const auto checkpointSeed = party.getLastSessionSeed();
    const auto checkpointDraws = party.getSessionRngDrawCount();

    auto dying = party.getMember(0);
    dying->takeDamage(dying->getHp() - 1);
    dying->setPoison(1);
    game.getStates().replaceAll(std::make_unique<crawl::DungeonState>(game));
    game.getStates().pushState(std::make_unique<crawl::CombatState>(game, crawl::EncounterTier::EARLY));
    auto* combat = dynamic_cast<crawl::CombatState*>(game.getStates().getCurrentState());
    CHECK(combat != nullptr);
    if (!combat) return;
    crawl::ControllerTestAccess::setTurnOrder(*combat, {
        {false, 1, 5}, {false, 0, 4}, {false, 2, 3}, {false, 3, 2}, {true, 0, 1}});
    crawl::ControllerTestAccess::advanceTurn(*combat);
    CHECK(dying->isDead());
    CHECK(!party.getMember(1)->isDead());
    for (int index = 1; index < party.getMemberCount(); ++index) {
        party.getMember(index)->takeDamage(party.getMember(index)->getHp());
    }
    crawl::ControllerTestAccess::advanceTurn(*combat);
    CHECK(dynamic_cast<crawl::GameOverState*>(game.getStates().getCurrentState()) != nullptr);
    CHECK(party.getMemberCount() == 4);
    for (int index = 0; index < party.getMemberCount(); ++index) CHECK(!party.getMember(index)->isDead());
    CHECK(crawl::SessionRng::global().seed() == checkpointSeed);
    CHECK(crawl::SessionRng::global().drawCount() == checkpointDraws);
}

void testAutoMoveActivatesOnlyBossEntryTriggers(crawl::Game& game) {
    auto& party = game.getParty();
    CHECK(party.startNewGame());
    CHECK(party.addMember(character("Auto", crawl::CharacterClass::WARRIOR, 18, 18, 30, 30)));
    party.acceptQuest(crawl::Quest::createCanonical("qst_recover_moon_seal"));
    party.acceptQuest(crawl::Quest::createCanonical("qst_defeat_crypt_warden"));
    party.acceptQuest(crawl::Quest::createCanonical("qst_find_missing_scout"));
    CHECK(party.saveToFile());
    game.getStates().replaceAll(std::make_unique<crawl::DungeonState>(game));
    auto* dungeon = dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState());
    CHECK(dungeon != nullptr);
    if (!dungeon) return;

    auto* item = party.getWorld().findObject("obj_moon_seal");
    CHECK(item != nullptr);
    auto seedNonEncounter = [] {
        for (std::uint32_t seed = 1;; ++seed) {
            crawl::SessionRng candidate(seed);
            if (candidate.rollRange(1, 100) > 10) return seed;
        }
    };
    crawl::SessionRng::reseedGlobal(seedNonEncounter());
    if (item) CHECK(crawl::ControllerTestAccess::autoStepTo(*dungeon, item->x, item->y));
    CHECK(dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState()) == dungeon);
    CHECK(item && item->state != crawl::WorldObjectState::RESOLVED);
    CHECK(!party.hasKeyItem("key_moon_seal"));

    int stairX = -1;
    int stairY = -1;
    for (int x = 0; x < crawl::DungeonMap::MAP_WIDTH; ++x) {
        for (int y = 0; y < crawl::DungeonMap::MAP_HEIGHT; ++y) {
            if (party.getWorld().getFloor(1).getTile(x, y) == crawl::TileType::DOWNSTAIRS) {
                stairX = x;
                stairY = y;
            }
        }
    }
    crawl::SessionRng::reseedGlobal(seedNonEncounter());
    CHECK(crawl::ControllerTestAccess::autoStepTo(*dungeon, stairX, stairY));
    CHECK(crawl::ControllerTestAccess::dungeonFloor(*dungeon) == 1);

    CHECK(crawl::ControllerTestAccess::moveDungeonToTileAndInteract(*dungeon, crawl::TileType::DOWNSTAIRS));
    auto* boss = party.getWorld().findObject("obj_crypt_warden");
    CHECK(boss != nullptr);
    if (boss) CHECK(crawl::ControllerTestAccess::autoStepTo(*dungeon, boss->x, boss->y));
    CHECK(dynamic_cast<crawl::CombatState*>(game.getStates().getCurrentState()) != nullptr);
    game.getStates().popState();
    CHECK(crawl::ControllerTestAccess::moveDungeonToTileAndInteract(*dungeon, crawl::TileType::DOWNSTAIRS));
    auto* scout = party.getWorld().findObject("obj_missing_scout");
    CHECK(scout != nullptr);
    crawl::SessionRng::reseedGlobal(seedNonEncounter());
    if (scout) CHECK(crawl::ControllerTestAccess::autoStepTo(*dungeon, scout->x, scout->y));
    CHECK(dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState()) == dungeon);
    CHECK(scout && scout->state != crawl::WorldObjectState::RESOLVED);

    int gateX = -1;
    int gateY = -1;
    for (int x = 0; x < crawl::DungeonMap::MAP_WIDTH; ++x) {
        for (int y = 0; y < crawl::DungeonMap::MAP_HEIGHT; ++y) {
            if (party.getWorld().getFloor(3).getTile(x, y) == crawl::TileType::BOSS_GATE) {
                gateX = x;
                gateY = y;
            }
        }
    }
    CHECK(crawl::ControllerTestAccess::autoStepTo(*dungeon, gateX, gateY));
    CHECK(dynamic_cast<crawl::CombatState*>(game.getStates().getCurrentState()) != nullptr);
}

void testTpkRestoresLatestFullDungeonCheckpoint(crawl::Game& game) {
    auto& party = game.getParty();
    CHECK(party.startNewGame());
    auto member = character("Checkpoint", crawl::CharacterClass::WARRIOR, 18, 18, 20, 20);
    CHECK(party.addMember(member));
    CHECK(party.saveToFile());
    game.getStates().replaceAll(std::make_unique<crawl::DungeonState>(game));
    auto* dungeon = dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState());
    CHECK(dungeon != nullptr);
    if (!dungeon) return;
    party.addGold(25);
    crawl::ControllerTestAccess::markDungeonVisited(*dungeon, 5, 5);
    dungeon->update(sf::seconds(2.1f));
    const auto checkpointSeed = party.getLastSessionSeed();
    const auto checkpointDraws = party.getSessionRngDrawCount();
    party.addGold(50);
    member->takeDamage(member->getHp());
    game.getStates().pushState(std::make_unique<crawl::CombatState>(game, crawl::EncounterTier::EARLY));
    auto* combat = dynamic_cast<crawl::CombatState*>(game.getStates().getCurrentState());
    CHECK(combat != nullptr);
    if (!combat) return;
    crawl::ControllerTestAccess::advanceTurn(*combat);
    CHECK(dynamic_cast<crawl::GameOverState*>(game.getStates().getCurrentState()) != nullptr);
    CHECK(party.getGold() == 125);
    CHECK(party.getWorld().getFloor(1).isVisited(5, 5));
    CHECK(crawl::SessionRng::global().seed() == checkpointSeed);
    CHECK(crawl::SessionRng::global().drawCount() == checkpointDraws);
}

void testRepeatedContinueAndReentryPreserveWorldUntilNewGame(crawl::Game& game) {
    auto& party = game.getParty();
    crawl::SessionRng::reseedGlobal(0x11112222U);
    CHECK(party.startNewGame());
    CHECK(party.addMember(character("Persistent", crawl::CharacterClass::WARRIOR, 18, 18, 20, 20)));
    party.acceptQuest(crawl::Quest::createCanonical("qst_recover_moon_seal"));
    auto* object = party.getWorld().findObject("obj_moon_seal");
    CHECK(object != nullptr);
    if (!object) return;
    party.getWorld().getFloor(1).setVisited(object->x, object->y, true);
    object->state = crawl::WorldObjectState::DISCOVERED;
    CHECK(party.saveToFile());
    game.getStates().replaceAll(std::make_unique<crawl::DungeonState>(game));
    auto* dungeon = dynamic_cast<crawl::DungeonState*>(game.getStates().getCurrentState());
    CHECK(dungeon != nullptr);
    if (!dungeon) return;
    const auto expectedWorld = party.getWorld().toJson();
    CHECK(!crawl::ControllerTestAccess::moveDungeonToTileAndCheck(*dungeon, crawl::TileType::UPSTAIRS));
    dungeon->handleInput(key(sf::Keyboard::Escape));
    CHECK(dynamic_cast<crawl::TownState*>(game.getStates().getCurrentState()) != nullptr);
    game.getStates().replaceAll(std::make_unique<crawl::DungeonState>(game));
    CHECK(party.getWorld().toJson() == expectedWorld);

    for (int attempt = 0; attempt < 2; ++attempt) {
        game.getStates().replaceAll(std::make_unique<crawl::TitleState>(game));
        auto* title = dynamic_cast<crawl::TitleState*>(game.getStates().getCurrentState());
        CHECK(title != nullptr);
        if (!title) return;
        title->handleInput(key(sf::Keyboard::Down));
        title->handleInput(key(sf::Keyboard::Enter));
        CHECK(dynamic_cast<crawl::TownState*>(game.getStates().getCurrentState()) != nullptr);
        CHECK(party.getWorld().toJson() == expectedWorld);
    }

    crawl::SessionRng::reseedGlobal(0x33334444U);
    game.getStates().replaceAll(std::make_unique<crawl::TitleState>(game));
    auto* title = dynamic_cast<crawl::TitleState*>(game.getStates().getCurrentState());
    CHECK(title != nullptr);
    if (!title) return;
    title->handleInput(key(sf::Keyboard::Enter));
    title->handleInput(key(sf::Keyboard::Enter));
    CHECK(party.getWorld().toJson() != expectedWorld);
}

} // namespace

int main() {
    const auto directory = std::filesystem::temp_directory_path() /
        ("crawlmaster-controller-tests-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    crawl::Party::setDefaultSavePath((directory / "save.json").string());
    crawl::LocalizationManager::setDefaultConfigPath((directory / "config.json").string());
    crawl::Game game(true);

    testDeferredTpkReplacesRootAndRestoresTownCheckpoint(game);
    testDungeonReturnCommitsAndFailureStays(game);
    testBossVictoryCommitsOnceAndBlocksReentry(game);
    testTitleKeepsDurabilityWarning(game);
    testNewGameAlwaysRequiresConfirmationWithoutSave(game);
    testContinueCompletedCampaignRoutesToVictory(game);
    testContinueRestoresProductionRngCheckpoint(game);
    testCharacterCreationRequiresIdentityPointsAndConfirm(game);
    testFailedAutoSwapPreservesGreatsword(game);
    testCombatItemAndAllySkillNeedPreviewConfirm(game);
    testOptionsKeyWorksAcrossTownAndCombatOverlays(game);
    testPersistentFloorTravelAndObjectiveInteractions(game);
    testQuestReportRollsBackOnSaveFailure(game);
    testQuestJournalOpensFromTownAndDungeon(game);
    testContinueCommitsSchemaV3WorldMigration(game);
    testCombatSaveRollbackDoesNotInvalidateDungeonWorld(game);
    testPoisonTurnStartDeathSkipsIndividualActions(game);
    testSuccessfulEscapeClearsAllCombatBuffs(game);
    testCharacterInfoShowsAllConditionsAndRejectsNoEffectConsumable(game);
    testEveryObjectiveQuestReportIsAtomicAndOneTime(game);
    testFourCharacterCreationIndividualDeathAndTpkLifecycle(game);
    testAutoMoveActivatesOnlyBossEntryTriggers(game);
    testTpkRestoresLatestFullDungeonCheckpoint(game);
    testRepeatedContinueAndReentryPreserveWorldUntilNewGame(game);
    testRecoveryPendingBlocksTpkAndTitleShutdownOverwrite();
    testShutdownRequiresExplicitResolutionAfterSaveFailure(game);

    std::filesystem::remove_all(directory);
    if (failures != 0) return 1;
    std::cout << "Controller contract tests passed.\n";
    return 0;
}
