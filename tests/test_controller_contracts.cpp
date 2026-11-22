#include "controller/BestiaryState.hpp"
#include "controller/CharacterCreationState.hpp"
#include "controller/CharacterInfoState.hpp"
#include "controller/CombatState.hpp"
#include "controller/DungeonState.hpp"
#include "controller/QuestJournalState.hpp"
#include "controller/SettingsState.hpp"
#include "controller/TitleState.hpp"
#include "controller/TownState.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"

#include <filesystem>
#include <iostream>

#define CHECK(expr)                                                                       \
    do {                                                                                  \
        if (!(expr)) {                                                                    \
            std::cerr << "[FAIL] ControllerContract: " #expr " (" << __LINE__ << ")\n"; \
            return 1;                                                                     \
        }                                                                                 \
    } while (false)

namespace {

sf::Event keyEvent(sf::Keyboard::Key key) {
    sf::Event e{};
    e.type = sf::Event::KeyPressed;
    e.key.code = key;
    return e;
}

} // namespace

int main() {
    using namespace crawl;
    const auto tempDir = std::filesystem::temp_directory_path() / "crawlmaster_controller_e2e";
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);
    Party::setDefaultSavePath((tempDir / "save.json").string());
    LocalizationManager::setDefaultConfigPath((tempDir / "config.json").string());
    SessionRng::global() = SessionRng(11223344U, 0);

    Game game(false);
    CHECK(dynamic_cast<TitleState*>(game.getStates().getCurrentState()) != nullptr);

    // 1. Title -> New Game -> Town
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::Enter));
    CHECK(dynamic_cast<TownState*>(game.getStates().getCurrentState()) != nullptr);

    // 2. Town -> Blacksmith (Num5 -> Enhance Enter -> Esc)
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::Num5));
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::Enter));
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::Escape));

    // 3. Town -> Quest Board (Num7 -> Accept Q1 -> Esc)
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::Num7));
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::Enter));
    CHECK(game.getParty().hasQuest("qst_01_relic"));
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::Escape));

    // 4. Town -> Bestiary (B) -> Esc
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::B));
    CHECK(dynamic_cast<BestiaryState*>(game.getStates().getCurrentState()) != nullptr);
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::Escape));

    // 5. Town -> CharacterInfo (C) -> Row Toggle (F) -> Esc
    const auto oldRow = game.getParty().getMember(0)->getRow();
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::C));
    CHECK(dynamic_cast<CharacterInfoState*>(game.getStates().getCurrentState()) != nullptr);
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::F));
    CHECK(game.getParty().getMember(0)->getRow() != oldRow);
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::Escape));

    // 6. Town -> Dungeon Gate (Num1 -> Enter) -> DungeonState
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::Num1));
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::Enter));
    CHECK(dynamic_cast<DungeonState*>(game.getStates().getCurrentState()) != nullptr);

    // 7. Dungeon movement + Turn + Full Map toggle + Quest Journal
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::W));
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::D));
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::M));
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::Escape));
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::J));
    CHECK(dynamic_cast<QuestJournalState*>(game.getStates().getCurrentState()) != nullptr);
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::Escape));

    // 8. CombatState E2E: Guard, Row Swap, Attack, Settings overlay
    EncounterSpec enc;
    enc.tier = EncounterTier::EARLY;
    enc.fixedMonsterId = "mon_kobold";
    enc.floorNumber = 1;
    game.getStates().pushState(std::make_unique<CombatState>(game, enc));
    CHECK(dynamic_cast<CombatState*>(game.getStates().getCurrentState()) != nullptr);
    game.getStates().getCurrentState()->update(sf::seconds(0.6f));
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::O));
    CHECK(dynamic_cast<SettingsState*>(game.getStates().getCurrentState()) != nullptr);
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::Escape));
    game.getStates().getCurrentState()->handleInput(keyEvent(sf::Keyboard::Num3)); // Guard
    game.getStates().getCurrentState()->update(sf::seconds(0.8f));

    // 9. Graceful shutdown
    game.requestShutdown();
    CHECK(game.isShutdownApproved());

    std::filesystem::remove_all(tempDir);
    std::cout << "[PASS] ControllerContractTests E2E passed.\n";
    return 0;
}
