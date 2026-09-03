#include "core/LocalizationManager.hpp"
#include "model/ItemFactory.hpp"
#include "model/MonsterFactory.hpp"
#include "model/Quest.hpp"
#include "model/SkillFactory.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

namespace {

int failures = 0;

void check(bool condition, const char* expression, int line) {
    if (condition) return;
    ++failures;
    std::cerr << "[Failure] line " << line << ": " << expression << '\n';
}

#define CHECK(expression) check(static_cast<bool>(expression), #expression, __LINE__)

const std::array<crawl::Language, 5> languages = {
    crawl::Language::KO, crawl::Language::EN, crawl::Language::JA,
    crawl::Language::ZH_TW, crawl::Language::ZH_CN
};

void testEveryLocaleResolvesCanonicalContent() {
    auto& localization = crawl::LocalizationManager::getInstance();
    std::set<std::string> daggerNames;
    std::set<std::string> koboldNames;

    for (const auto language : languages) {
        localization.setLanguage(language);
        for (const auto& id : crawl::ItemFactory::getRegisteredIds()) {
            const auto item = crawl::ItemFactory::createItem(id);
            CHECK(item != nullptr);
            CHECK(item && !item->getName().empty());
            CHECK(item && !item->getDescription().empty());
            CHECK(item && item->getName().find("ITEM_") == std::string::npos);
        }
        for (const auto& id : crawl::MonsterFactory::getRegisteredIds()) {
            const auto monster = crawl::MonsterFactory::createMonster(id);
            CHECK(monster != nullptr);
            CHECK(monster && !monster->getName().empty());
            CHECK(monster && monster->getName().find("MONSTER_") == std::string::npos);
        }
        for (const auto& id : crawl::SkillFactory::getRegisteredIds()) {
            const auto skill = crawl::SkillFactory::createSkill(id);
            CHECK(skill != nullptr);
            CHECK(skill && !skill->getName().empty());
            CHECK(skill && !skill->getDescription().empty());
            CHECK(skill && skill->getName().find("SKILL_") == std::string::npos);
        }
        for (const auto& id : crawl::Quest::getCanonicalIds()) {
            const auto quest = crawl::Quest::createCanonical(id);
            CHECK(quest != nullptr);
            CHECK(quest && !quest->getName().empty());
            CHECK(quest && !quest->getDescription().empty());
            CHECK(quest && quest->getName().find("QUEST_") == std::string::npos);
        }
        daggerNames.insert(crawl::ItemFactory::createItem("wpn_dagger")->getName());
        koboldNames.insert(crawl::MonsterFactory::createMonster("mon_kobold")->getName());
    }

    CHECK(daggerNames.size() >= 4);
    CHECK(koboldNames.size() >= 4);
}

void testFormatterClosesEveryPlaceholder() {
    auto& localization = crawl::LocalizationManager::getInstance();
    for (const auto language : languages) {
        localization.setLanguage(language);
        CHECK(localization.has("COMBAT_LOG_DAMAGE"));
        const std::string message = localization.format("COMBAT_LOG_DAMAGE", {
            {"target", "Target"}, {"damage", "7"}
        });
        CHECK(message.find("{target}") == std::string::npos);
        CHECK(message.find("{damage}") == std::string::npos);
        CHECK(message.find("Target") != std::string::npos);
        CHECK(message.find('7') != std::string::npos);
    }
}

void testTextScaleContract() {
    auto& localization = crawl::LocalizationManager::getInstance();
    localization.setTextScale(75);
    CHECK(localization.getScaledTextSize(16) == 14U);
    localization.setTextScale(100);
    CHECK(localization.getScaledTextSize(16) == 16U);
    localization.setTextScale(200);
    CHECK(localization.getScaledTextSize(16) == 32U);
}

void testKnownProductionLiteralRegressionsStayRemoved() {
    const std::array files = {
        "src/controller/TownState.cpp", "src/controller/DungeonState.cpp",
        "src/controller/CombatState.cpp", "src/controller/CharacterInfoState.cpp",
        "src/model/Skill.cpp", "src/model/Character.cpp", "src/model/ItemFactory.cpp",
        "src/model/MonsterFactory.cpp", "include/model/ConcreteItems.hpp"
    };
    const std::array forbidden = {
        "menuOss << \"Guild Desk:", "menuOss << \"Shop Menu:",
        "Shop Catalog (Buy):", "menuOss << \"Temple Sanctuary:",
        "addLog(\"> Auto-navigation", "drawText(window, \"Name:",
        "drawText(window, \"Class:", "addLog(\"> Monsters appeared",
        "addLog(\"> Loot acquired:", "logOutput.push_back(\"> ",
        "setString(getMonsterAsciiArt())"
    };
    for (const auto* relative : files) {
        const std::filesystem::path path = std::filesystem::path(CRAWLMASTER_SOURCE_DIR) / relative;
        std::ifstream input(path);
        const std::string source{std::istreambuf_iterator<char>(input),
                                 std::istreambuf_iterator<char>()};
        CHECK(!source.empty());
        for (const auto* fragment : forbidden) CHECK(source.find(fragment) == std::string::npos);
    }
}

} // namespace

int main() {
    testEveryLocaleResolvesCanonicalContent();
    testFormatterClosesEveryPlaceholder();
    testTextScaleContract();
    testKnownProductionLiteralRegressionsStayRemoved();
    if (failures != 0) return 1;
    std::cout << "Localization contract tests passed.\n";
    return 0;
}
