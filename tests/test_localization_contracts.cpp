#include "core/LocalizationManager.hpp"
#include "model/ItemFactory.hpp"
#include "model/MonsterFactory.hpp"
#include "model/Quest.hpp"
#include "model/SkillFactory.hpp"

#include <array>
#include <iostream>
#include <string>

#define CHECK(expr)                                                                        \
    do {                                                                                   \
        if (!(expr)) {                                                                     \
            std::cerr << "[FAIL] LocalizationContract: " #expr " (" << __LINE__ << ")\n"; \
            return 1;                                                                      \
        }                                                                                  \
    } while (false)

int main() {
    using namespace crawl;
    auto& lm = LocalizationManager::getInstance();
    const std::array<Language, 5> langs = {
        Language::KO, Language::EN, Language::JA, Language::ZH_TW, Language::ZH_CN
    };

    for (const auto lang : langs) {
        lm.setLanguage(lang);
        CHECK(lm.has("TITLE_NEW_GAME"));
        CHECK(lm.has("TOWN_FORGE_TITLE"));
        CHECK(lm.has("BESTIARY_TITLE"));
        CHECK(lm.has("BOSS_ELDER_WYRM_PHASE2"));
        for (int f = 1; f <= 5; ++f) {
            CHECK(lm.has("BIOME_" + std::to_string(f) + "_NAME"));
            CHECK(lm.has("LORE_FLOOR_" + std::to_string(f)));
        }
        for (const auto& id : ItemFactory::getRegisteredIds()) {
            const auto item = ItemFactory::createItem(id);
            CHECK(item->getName().find("ITEM_") == std::string::npos);
            CHECK(item->getDescription().find("ITEM_") == std::string::npos);
        }
        for (const auto& id : MonsterFactory::getRegisteredIds()) {
            const auto m = MonsterFactory::createMonster(id);
            CHECK(m->getName().find("MONSTER_") == std::string::npos);
        }
        for (const auto& id : SkillFactory::getRegisteredIds()) {
            const auto sk = SkillFactory::createSkill(id);
            CHECK(sk->getName().find("SKILL_") == std::string::npos);
        }
        for (const auto& id : Quest::getCanonicalIds()) {
            const auto q = Quest::createCanonical(id);
            CHECK(q->getName().find("QUEST_") == std::string::npos);
        }
    }

    lm.setLanguage(Language::KO);
    std::cout << "[PASS] LocalizationContractTests passed (5 locales x full content).\n";
    return 0;
}
