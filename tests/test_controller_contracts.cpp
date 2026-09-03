#include "controller/CharacterInfoState.hpp"
#include "controller/CombatState.hpp"
#include "controller/DungeonState.hpp"
#include "controller/GameOverState.hpp"
#include "controller/SettingsState.hpp"
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
#include <iostream>

namespace crawl {

class ControllerTestAccess {
public:
    static const std::string& titleStatus(const TitleState& state) { return state.m_statusMessage; }

    static bool moveDungeonToTileAndCheck(DungeonState& state, TileType tile) {
        for (int x = 0; x < DungeonMap::MAP_WIDTH; ++x) {
            for (int y = 0; y < DungeonMap::MAP_HEIGHT; ++y) {
                if (state.m_map.getTile(x, y) == tile) {
                    state.m_map.setPlayerPos(x, y);
                    return state.checkCurrentTileLog();
                }
            }
        }
        return false;
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
    crawl::Persistence::setPostCommitSyncFailureForTests(false);
    CHECK(dynamic_cast<crawl::TitleState*>(game.getStates().getCurrentState()) == title);
    CHECK(crawl::ControllerTestAccess::titleStatus(*title) ==
          crawl::LocalizationManager::getInstance().get("TITLE_DURABILITY_UNKNOWN"));
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

void testRecruitmentRequiresPreviewAndConfirm(crawl::Game& game) {
    auto& party = game.getParty();
    CHECK(party.startNewGame());
    game.getStates().replaceAll(std::make_unique<crawl::TownState>(game));
    auto* town = dynamic_cast<crawl::TownState*>(game.getStates().getCurrentState());
    CHECK(town != nullptr);
    if (!town) return;
    town->handleInput(key(sf::Keyboard::Num1));
    town->handleInput(key(sf::Keyboard::Num1));
    CHECK(party.getMemberCount() == 0);
    town->handleInput(key(sf::Keyboard::R));
    CHECK(party.getMemberCount() == 0);
    town->handleInput(key(sf::Keyboard::Enter));
    CHECK(party.getMemberCount() == 1);
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
    testContinueCompletedCampaignRoutesToVictory(game);
    testContinueRestoresProductionRngCheckpoint(game);
    testRecruitmentRequiresPreviewAndConfirm(game);
    testFailedAutoSwapPreservesGreatsword(game);
    testCombatItemAndAllySkillNeedPreviewConfirm(game);
    testOptionsKeyWorksAcrossTownAndCombatOverlays(game);

    std::filesystem::remove_all(directory);
    if (failures != 0) return 1;
    std::cout << "Controller contract tests passed.\n";
    return 0;
}
