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

#include <SFML/Graphics.hpp>

#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

namespace crawl {

class ControllerTestAccess {
public:
    static int remainingPoints(const CharacterCreationState& state) {
        return state.m_draft.remainingPoints();
    }

    static int selectedCost(const CharacterCreationState& state) {
        static constexpr std::array<Ability, 6> abilities = {
            Ability::STRENGTH, Ability::DEXTERITY, Ability::CONSTITUTION,
            Ability::INTELLIGENCE, Ability::WISDOM, Ability::CHARISMA};
        return state.m_draft.increaseCost(abilities[static_cast<std::size_t>(state.m_abilityRow)]);
    }
};

} // namespace crawl

namespace {

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

std::string suffix(crawl::Language language) {
    switch (language) {
        case crawl::Language::KO: return "ko";
        case crawl::Language::EN: return "en";
        case crawl::Language::JA: return "ja";
        case crawl::Language::ZH_TW: return "zh_tw";
        case crawl::Language::ZH_CN: return "zh_cn";
    }
    return "unknown";
}

std::shared_ptr<crawl::Character> sampleCharacter() {
    const nlohmann::json fixture = {
        {"name", "Ragnar"}, {"class", 3}, {"level", 1}, {"xp", 0},
        {"age", 29}, {"gender", "male"},
        {"hp", 10}, {"maxHp", 12}, {"spellSlots", 2}, {"maxSpellSlots", 2},
        {"poisonTurns", 2}, {"paralysisTurns", 0},
        {"abilities", {{"strength", 16}, {"dexterity", 30}, {"constitution", 14},
            {"intelligence", 10}, {"wisdom", 10}, {"charisma", 10}}},
        {"equipment", {{"weapon", "wpn_mace"}, {"armor", "arm_scale"}, {"shield", ""}}}
    };
    return std::shared_ptr<crawl::Character>(crawl::Character::fromJson(fixture, 3));
}

bool capture(sf::RenderWindow& window, crawl::GameState& state,
             const std::filesystem::path& output) {
    window.clear(sf::Color(5, 11, 5));
    state.draw(window);
    window.display();
    sf::Texture texture;
    if (!texture.create(window.getSize().x, window.getSize().y)) return false;
    texture.update(window);
    return texture.copyToImage().saveToFile(output.string()) &&
           std::filesystem::file_size(output) > 1024U;
}

bool hasCyanPlayerMarker(sf::RenderWindow& window, crawl::GameState& state) {
    window.clear(sf::Color(5, 11, 5));
    state.draw(window);
    window.display();
    sf::Texture texture;
    if (!texture.create(window.getSize().x, window.getSize().y)) return false;
    texture.update(window);
    const sf::Image image = texture.copyToImage();
    for (unsigned int y = 40; y < 280; ++y) {
        for (unsigned int x = 750; x < 990; ++x) {
            if (image.getPixel(x, y) == sf::Color(102, 255, 255)) return true;
        }
    }
    return false;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) return 2;
    const std::filesystem::path outputDirectory(argv[1]);
    std::filesystem::create_directories(outputDirectory);
    const auto temp = std::filesystem::temp_directory_path() /
        ("crawlmaster-ui-raster-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(temp);
    crawl::Party::setDefaultSavePath((temp / "save.json").string());
    crawl::LocalizationManager::setDefaultConfigPath((temp / "config.json").string());
    crawl::SessionRng::reseedGlobal(0xA11CEU);

    crawl::Game game(false);
    game.getParty().resetToDefault();
    auto rasterCharacter = sampleCharacter();
    rasterCharacter->setParalysis(2);
    rasterCharacter->applyStrBuff(3, 4);
    rasterCharacter->applyDexBuff(3, 5);
    rasterCharacter->applyBless(6);
    if (!game.getParty().addMember(rasterCharacter)) return 1;
    for (const auto& questId : crawl::Quest::getOfferableIds()) {
        game.getParty().acceptQuest(crawl::Quest::createCanonical(questId));
    }
    game.getParty().getWorld().generate(crawl::SessionRng::global().seed());
    if (auto* object = game.getParty().getWorld().findObject("obj_moon_seal")) {
        object->state = crawl::WorldObjectState::DISCOVERED;
        game.getParty().getWorld().getFloor(1).setVisited(object->x, object->y, true);
    }
    if (!game.getParty().saveToFile()) return 1;
    sf::RenderWindow window(sf::VideoMode(1024, 768), "Crawlmaster UI Raster",
                            sf::Style::Titlebar);

    const std::array<crawl::Language, 5> languages = {
        crawl::Language::KO, crawl::Language::EN, crawl::Language::JA,
        crawl::Language::ZH_TW, crawl::Language::ZH_CN
    };
    const std::array<int, 3> scales = {75, 100, 200};
    for (const auto language : languages) {
        for (const int scale : scales) {
            auto& localization = crawl::LocalizationManager::getInstance();
            localization.setLanguage(language);
            localization.setTextScale(scale);
            const std::string prefix = suffix(language) + "-scale-" + std::to_string(scale) + "-";

            crawl::TitleState title(game);
            if (!capture(window, title, outputDirectory / (prefix + "title.png"))) return 1;
            title.handleInput(key(sf::Keyboard::Enter));
            if (!capture(window, title, outputDirectory / (prefix + "title-confirm.png"))) return 1;
            crawl::TownState town(game);
            if (!capture(window, town, outputDirectory / (prefix + "town.png"))) return 1;
            auto captureTown = [&](const std::string& name,
                                   std::initializer_list<sf::Keyboard::Key> inputs) {
                crawl::TownState state(game);
                for (const auto input : inputs) state.handleInput(key(input));
                return capture(window, state, outputDirectory / (prefix + "town-" + name + ".png"));
            };
            if (!captureTown("guild", {sf::Keyboard::Num1})) return 1;
            crawl::CharacterCreationState creation(game);
            if (!capture(window, creation, outputDirectory / (prefix + "character-create-identity.png"))) return 1;
            creation.handleInput(text('A'));
            creation.handleInput(text('r'));
            creation.handleInput(text('i'));
            creation.handleInput(text('a'));
            creation.handleInput(key(sf::Keyboard::Enter));
            if (!capture(window, creation, outputDirectory / (prefix + "character-create-abilities.png"))) return 1;
            int stalledRows = 0;
            while (crawl::ControllerTestAccess::remainingPoints(creation) > 0 && stalledRows < 18) {
                const int remaining = crawl::ControllerTestAccess::remainingPoints(creation);
                const int cost = crawl::ControllerTestAccess::selectedCost(creation);
                if (cost > 0 && cost <= remaining) {
                    creation.handleInput(key(sf::Keyboard::Right));
                    stalledRows = 0;
                } else {
                    creation.handleInput(key(sf::Keyboard::Down));
                    ++stalledRows;
                }
            }
            if (crawl::ControllerTestAccess::remainingPoints(creation) != 0) return 1;
            creation.handleInput(key(sf::Keyboard::Enter));
            if (!capture(window, creation, outputDirectory / (prefix + "character-create-confirm.png"))) return 1;
            if (!captureTown("guild-dismiss-confirm", {sf::Keyboard::Num1, sf::Keyboard::Num2})) return 1;
            if (!captureTown("shop", {sf::Keyboard::Num2})) return 1;
            if (!captureTown("shop-buy", {sf::Keyboard::Num2, sf::Keyboard::Num1})) return 1;
            if (!captureTown("shop-sell", {sf::Keyboard::Num2, sf::Keyboard::Num2})) return 1;
            if (!captureTown("shop-sell-confirm", {sf::Keyboard::Num2, sf::Keyboard::Num2, sf::Keyboard::Num1})) return 1;
            if (!captureTown("temple", {sf::Keyboard::Num3})) return 1;
            if (!captureTown("castle", {sf::Keyboard::Num4})) return 1;
            crawl::QuestJournalState journal(game);
            if (!capture(window, journal, outputDirectory / (prefix + "quest-journal.png"))) return 1;
            crawl::SettingsState settings(game);
            if (!capture(window, settings, outputDirectory / (prefix + "settings.png"))) return 1;
            crawl::GameOverState gameOver(game);
            if (!capture(window, gameOver, outputDirectory / (prefix + "game-over.png"))) return 1;
            crawl::VictoryState victory(game);
            if (!capture(window, victory, outputDirectory / (prefix + "victory.png"))) return 1;
            crawl::ShutdownState shutdown(game, crawl::PersistenceStatus::IoError);
            if (!capture(window, shutdown, outputDirectory / (prefix + "shutdown-save-failed.png"))) return 1;
            crawl::ShutdownState recoveryShutdown(game, crawl::PersistenceStatus::RecoveryPending);
            if (!capture(window, recoveryShutdown,
                         outputDirectory / (prefix + "shutdown-recovery-pending.png"))) return 1;
            crawl::CharacterInfoState character(game, false);
            if (!capture(window, character, outputDirectory / (prefix + "character-bag.png"))) return 1;
            character.handleInput(key(sf::Keyboard::Tab));
            if (!capture(window, character, outputDirectory / (prefix + "character-details.png"))) return 1;
            crawl::DungeonState dungeon(game);
            if (!capture(window, dungeon, outputDirectory / (prefix + "dungeon.png"))) return 1;
            if (!hasCyanPlayerMarker(window, dungeon)) return 1;
            crawl::SessionRng::reseedGlobal(0xC0B47U);
            crawl::CombatState combat(game, crawl::EncounterTier::EARLY);
            if (!capture(window, combat, outputDirectory / (prefix + "combat.png"))) return 1;

            crawl::SessionRng::reseedGlobal(0xC0B47U);
            crawl::CombatState skillCombat(game, crawl::EncounterTier::EARLY);
            skillCombat.handleInput(key(sf::Keyboard::Num2));
            if (!capture(window, skillCombat, outputDirectory / (prefix + "combat-skills.png"))) return 1;
            skillCombat.handleInput(key(sf::Keyboard::Num1));
            if (!capture(window, skillCombat, outputDirectory / (prefix + "combat-skill-target.png"))) return 1;
            skillCombat.handleInput(key(sf::Keyboard::Enter));
            if (!capture(window, skillCombat, outputDirectory / (prefix + "combat-skill-confirm.png"))) return 1;

            crawl::SessionRng::reseedGlobal(0xC0B47U);
            crawl::CombatState itemCombat(game, crawl::EncounterTier::EARLY);
            itemCombat.handleInput(key(sf::Keyboard::Num3));
            if (!capture(window, itemCombat, outputDirectory / (prefix + "combat-items.png"))) return 1;
            itemCombat.handleInput(key(sf::Keyboard::Num1));
            if (!capture(window, itemCombat, outputDirectory / (prefix + "combat-item-target.png"))) return 1;
            itemCombat.handleInput(key(sf::Keyboard::Enter));
            if (!capture(window, itemCombat, outputDirectory / (prefix + "combat-item-confirm.png"))) return 1;
        }
    }

    window.close();
    std::filesystem::remove_all(temp);
    std::cout << "Production state raster evidence generated for 5 locales x 3 scales across state/substate views.\n";
    return 0;
}
