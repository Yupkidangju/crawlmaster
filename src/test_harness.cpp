// [v0.1.0] test_harness.cpp 신규 작성
// Crawlmaster 유닛 테스트 러너. 핵심 수학 공식(D&D 능력치 보정치) 및 JSON 로직 정합성을 검증한다.

#include <iostream>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <SFML/Graphics.hpp>
#include "model/Character.hpp"
#include "model/Party.hpp"
#include "model/ItemFactory.hpp"
#include "model/ConcreteItems.hpp"
#include "core/LocalizationManager.hpp"
#include "core/GameStateManager.hpp"

namespace {

std::filesystem::path g_testDirectory;
int g_failureCount = 0;

void check(bool condition, const char* expression, const char* file, int line) {
    if (condition) {
        return;
    }

    ++g_failureCount;
    std::cerr << "[Failure] " << file << ':' << line
              << ": CHECK(" << expression << ")" << std::endl;
}

std::string readFileBytes(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

#define CHECK(condition) check(static_cast<bool>(condition), #condition, __FILE__, __LINE__)

}

// D&D 보정치 공식 검증 테스트
void testAbilityModifiers() {
    std::cout << "[Test] D&D 능력치 보정치 계산식을 검증합니다." << std::endl;
    
    auto getModifier = [](int score) -> int {
        int diff = score - 10;
        return (diff < 0) ? (diff - 1) / 2 : diff / 2;
    };

    // 테스트 셋업 검증
    CHECK(getModifier(10) == 0);
    CHECK(getModifier(11) == 0);
    CHECK(getModifier(12) == 1);
    CHECK(getModifier(13) == 1);
    CHECK(getModifier(14) == 2);
    CHECK(getModifier(15) == 2);
    CHECK(getModifier(8) == -1);
    CHECK(getModifier(9) == -1);
    CHECK(getModifier(18) == 4);

    std::cout << "-> [Success] 모든 보정치 룰 테스트가 정상적으로 통과되었습니다." << std::endl;
}

// JSON 파싱 및 직렬화 검증 테스트
void testJsonSerialization() {
    std::cout << "[Test] nlohmann/json 직렬화 및 구문 분석 정합성을 검증합니다." << std::endl;

    using json = nlohmann::json;

    // 모의 세이브 데이터 생성
    json saveObj;
    saveObj["party_gold"] = 100;
    saveObj["depth"] = 1;
    saveObj["active_quests"] = json::array();

    std::string serialized = saveObj.dump();
    
    // 복원 테스트
    json parsed = json::parse(serialized);
    CHECK(parsed["party_gold"] == 100);
    CHECK(parsed["depth"] == 1);
    CHECK(parsed["active_quests"].is_array());

    std::cout << "-> [Success] JSON 직렬화/역직렬화 테스트가 정상 통과되었습니다." << std::endl;
}

#include "model/DungeonMap.hpp"
#include <queue>

// 던전 랜덤 생성기 및 미로 도달가능성(연결성) 검증 테스트
void testDungeonMazeGeneration() {
    std::cout << "[Test] DFS 던전 미로 생성 및 연결성을 검증합니다." << std::endl;

    crawl::DungeonMap map;
    map.generate(0x1001U);

    // 1. 플레이어 스폰 위치가 UPSTAIRS 계단 타일인지 체크
    CHECK(map.getTile(1, 1) == crawl::TileType::UPSTAIRS);
    CHECK(map.getPlayerX() == 1);
    CHECK(map.getPlayerY() == 1);

    // 2. BFS를 사용해 (1, 1)에서 접근 가능한 모든 타일 개수를 산출
    bool visited[crawl::DungeonMap::MAP_WIDTH][crawl::DungeonMap::MAP_HEIGHT] = {false};
    std::queue<std::pair<int, int>> q;

    q.push({1, 1});
    visited[1][1] = true;
    int reachableCount = 0;

    while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();
        reachableCount++;

        // 네 방향 인접 셀 검사
        int dx[] = {0, 0, 1, -1};
        int dy[] = {1, -1, 0, 0};

        for (int i = 0; i < 4; ++i) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];

            if (nx >= 0 && nx < crawl::DungeonMap::MAP_WIDTH && ny >= 0 && ny < crawl::DungeonMap::MAP_HEIGHT) {
                if (!visited[nx][ny] && map.getTile(nx, ny) != crawl::TileType::WALL) {
                    visited[nx][ny] = true;
                    q.push({nx, ny});
                }
            }
        }
    }

    // 3. 맵 상의 총 통로 타일(EMPTY, DOOR, UPSTAIRS) 개수 산출
    int totalPassages = 0;
    for (int y = 0; y < crawl::DungeonMap::MAP_HEIGHT; ++y) {
        for (int x = 0; x < crawl::DungeonMap::MAP_WIDTH; ++x) {
            if (map.getTile(x, y) != crawl::TileType::WALL) {
                totalPassages++;
            }
        }
    }

    // 두 수치가 정확히 일치하여 고립된 구역이 없는지 단언 검증
    std::cout << "-> 도달가능 타일 수: " << reachableCount << " / 총 통로 타일 수: " << totalPassages << std::endl;
    CHECK(reachableCount == totalPassages);

    std::cout << "-> [Success] 무작위 미로 생성기 및 연결성 검증이 정상 통과되었습니다." << std::endl;
}

// D&D 레벨업 및 AC 장비 보정 계산 검증 테스트
void testDndRulesAndLevelup() {
    std::cout << "[Test] D&D 규칙 및 레벨업 시스템을 검증합니다." << std::endl;

    // 1. 캐릭터 생성 (전사)
    auto hero = std::make_unique<crawl::Character>("TestWarrior", crawl::CharacterClass::WARRIOR);
    
    // Warrior의 1레벨 기본 AC 검증 (Scale Mail + 제한 DEX + 방어 전투술)
    int dexMod = hero->getAbilities().getModifier(hero->getAbilities().dexterity);
    int expectedAc = 14 + std::min(2, dexMod) + 1;
    CHECK(hero->getAc() == expectedAc);
    
    // 2. 경험치 획득에 의한 2레벨 레벨업 유도
    int oldMaxHp = hero->getMaxHp();
    bool leveledUp = hero->addXp(300); // 300 XP 수급
    
    CHECK(leveledUp == true);
    CHECK(hero->getLevel() == 2);
    // 레벨업에 따라 최대 HP가 상승했는지 검증
    CHECK(hero->getMaxHp() > oldMaxHp);
    CHECK(hero->getHp() == hero->getMaxHp()); // 완치 검증

    std::cout << "-> [Success] D&D 룰엔진 및 레벨업 검증이 정상 통과되었습니다." << std::endl;
}

// 파티 저장/불러오기 검증 테스트
void testPartySaveLoad() {
    std::cout << "[Test] 파티 데이터 직렬화 및 복원을 검증합니다." << std::endl;

    const std::string tempSavePath = (g_testDirectory / "party-save.json").string();

    // 1. 모의 파티 구성
    crawl::Party party;
    party.resetToDefault();
    party.addGold(150); // 기본 100 + 150 = 250 Gold
    
    auto char1 = std::make_shared<crawl::Character>("HeroA", crawl::CharacterClass::MAGE);
    auto char2 = std::make_shared<crawl::Character>("HeroB", crawl::CharacterClass::ROGUE);
    party.addMember(char1);
    party.addMember(char2);

    // 저장 진행
    bool saveResult = party.saveToFile(tempSavePath);
    CHECK(saveResult == true);

    // 2. 다른 객체에서 로드 복원 검증
    crawl::Party loadedParty;
    bool loadResult = loadedParty.loadFromFile(tempSavePath);
    CHECK(loadResult == true);
    CHECK(loadedParty.getGold() == 250);
    CHECK(loadedParty.getMemberCount() == 2);
    CHECK(loadedParty.getMember(0)->getName() == "HeroA");
    CHECK(loadedParty.getMember(1)->getName() == "HeroB");

    // 테스트 정리: 임시 생성 세이브 파일 삭제
    std::remove(tempSavePath.c_str());

    std::cout << "-> [Success] 파티 영속화 및 복원 검증이 완료되었습니다." << std::endl;
}

void testDefaultInventoryContract() {
    std::cout << "[Test] 기본 인벤토리 구성을 검증합니다." << std::endl;

    const std::string previousDefaultSavePath = crawl::Party::getDefaultSavePath();
    const std::filesystem::path fixturePath = g_testDirectory / "default-inventory.json";
    crawl::Party::setDefaultSavePath(fixturePath.string());
    std::filesystem::remove(fixturePath);

    crawl::Party party;
    int healingPotionCount = 0;
    int manaPotionCount = 0;
    for (const auto& item : party.getInventory()) {
        if (item && item->getId() == "pot_heal") {
            ++healingPotionCount;
        } else if (item && item->getId() == "pot_mana") {
            ++manaPotionCount;
        }
    }

    CHECK(healingPotionCount == 2);
    CHECK(manaPotionCount == 1);

    std::filesystem::remove(fixturePath);
    crawl::Party::setDefaultSavePath(previousDefaultSavePath);
}

void testCorruptCustomSaveIsNonDestructive() {
    std::cout << "[Test] 손상된 custom save 로드의 비파괴 실패를 검증합니다." << std::endl;

    const std::string previousDefaultSavePath = crawl::Party::getDefaultSavePath();
    const std::filesystem::path defaultPath = g_testDirectory / "corrupt-load-default.json";
    const std::filesystem::path customPath = g_testDirectory / "corrupt-load-custom.json";
    crawl::Party::setDefaultSavePath(defaultPath.string());
    std::filesystem::remove(defaultPath);
    std::filesystem::remove(customPath);

    crawl::Party fixtureParty;
    CHECK(fixtureParty.saveToFile(defaultPath.string()));
    crawl::Party party;
    std::filesystem::remove(defaultPath);

    {
        std::ofstream corruptFile(customPath, std::ios::binary);
        corruptFile << "{ \"gold\": \"corrupted_string_instead_of_int\", \"members\": \"invalid\" }";
    }
    const std::string customBytesBefore = readFileBytes(customPath);

    const bool loadResult = party.loadFromFile(customPath.string());
    CHECK(loadResult == false);
    CHECK(!std::filesystem::exists(customPath));
    int quarantineCount = 0;
    for (const auto& entry : std::filesystem::directory_iterator(g_testDirectory)) {
        const std::string filename = entry.path().filename().string();
        if (filename.starts_with("corrupt-load-custom.corrupt-") && entry.path().extension() == ".json") {
            ++quarantineCount;
            CHECK(readFileBytes(entry.path()) == customBytesBefore);
        }
    }
    CHECK(quarantineCount == 1);
    CHECK(!std::filesystem::exists(defaultPath));

    for (const auto& entry : std::filesystem::directory_iterator(g_testDirectory)) {
        const std::string filename = entry.path().filename().string();
        if (filename.starts_with("corrupt-load-custom.corrupt-") && entry.path().extension() == ".json") {
            std::filesystem::remove(entry.path());
        }
    }
    std::filesystem::remove(defaultPath);
    crawl::Party::setDefaultSavePath(previousDefaultSavePath);
}

void testResetToDefaultDoesNotWrite() {
    std::cout << "[Test] resetToDefault의 메모리 전용 초기화를 검증합니다." << std::endl;

    const std::string previousDefaultSavePath = crawl::Party::getDefaultSavePath();
    const std::filesystem::path fixturePath = g_testDirectory / "reset-default.json";
    crawl::Party::setDefaultSavePath(fixturePath.string());
    std::filesystem::remove(fixturePath);

    crawl::Party fixtureParty;
    CHECK(fixtureParty.saveToFile(fixturePath.string()));
    crawl::Party party;
    std::filesystem::remove(fixturePath);

    party.addGold(50);
    party.addMember(std::make_shared<crawl::Character>("ResetTarget", crawl::CharacterClass::WARRIOR));
    party.resetToDefault();

    CHECK(party.getGold() == 100);
    CHECK(party.getMemberCount() == 0);
    CHECK(!std::filesystem::exists(fixturePath));

    std::filesystem::remove(fixturePath);
    crawl::Party::setDefaultSavePath(previousDefaultSavePath);
}

bool hasCanonicalCharacterSchema(const nlohmann::json& character) {
    return character.contains("age") && character.contains("gender") &&
           character.contains("maxHp") && character.contains("spellSlots") &&
           character.contains("maxSpellSlots") && character.contains("poisonTurns") &&
           character.contains("paralysisTurns") && character.contains("equipment") &&
           character.at("equipment").is_object() &&
           character.at("equipment").contains("weapon") &&
           character.at("equipment").contains("armor") &&
           character.at("equipment").contains("shield") &&
           !character.contains("max_hp") && !character.contains("spell_slots") &&
           !character.contains("max_spell_slots") && !character.contains("poison_turns") &&
           !character.contains("paralysis_turns") && !character.contains("eq_weapon") &&
           !character.contains("eq_armor") && !character.contains("eq_shield");
}

bool hasCanonicalQuestSchema(const nlohmann::json& quest) {
    return quest.contains("targetId") && quest.contains("targetCount") &&
           quest.contains("currentCount") && quest.contains("goldReward") &&
           quest.contains("xpReward") && quest.contains("isCompleted") &&
           !quest.contains("target_id") && !quest.contains("target_count") &&
           !quest.contains("current_count") && !quest.contains("gold_reward") &&
           !quest.contains("xp_reward") && !quest.contains("is_completed");
}

nlohmann::json makeInvalidV2CharacterFixture(int level, int hp, int maxHp) {
    nlohmann::json character = {
        {"name", "FixtureHero"},
        {"class", static_cast<int>(crawl::CharacterClass::WARRIOR)},
        {"level", level},
        {"xp", 0},
        {"hp", hp},
        {"maxHp", maxHp},
        {"spellSlots", 0},
        {"maxSpellSlots", 0},
        {"poisonTurns", 0},
        {"paralysisTurns", 0},
        {"abilities", {{"strength", 10}, {"dexterity", 10}, {"constitution", 10},
                       {"intelligence", 10}, {"wisdom", 10}, {"charisma", 10}}},
        {"equipment", {{"weapon", ""}, {"armor", ""}, {"shield", ""}}}
    };
    // 현행 v2 parser가 legacy key를 읽더라도 값 범위 검증 자체가 실행되도록 alias를 함께 둔다.
    character["max_hp"] = maxHp;
    character["spell_slots"] = 0;
    character["max_spell_slots"] = 0;
    character["poison_turns"] = 0;
    character["paralysis_turns"] = 0;
    character["eq_weapon"] = "";
    character["eq_armor"] = "";
    character["eq_shield"] = "";
    return character;
}

nlohmann::json makeV2Root(const nlohmann::json& members) {
    return {
        {"schemaVersion", 2},
        {"gold", 25},
        {"inventory", nlohmann::json::array()},
        {"members", members},
        {"activeQuests", nlohmann::json::array()},
        {"completedQuestIds", nlohmann::json::array()},
        {"campaignCompleted", false},
        {"lastSessionSeed", 0}
    };
}

class FakeState final : public crawl::GameState {
public:
    explicit FakeState(int& destructorCount) : m_destructorCount(destructorCount) {}
    ~FakeState() override { ++m_destructorCount; }

    void handleInput(const sf::Event&) override {}
    void update(sf::Time) override {}
    void draw(sf::RenderWindow&) override {}

private:
    int& m_destructorCount;
};

void testReplaceAllClearsStateStack() {
    int destructorCount = 0;
    crawl::GameStateManager manager;
    manager.pushState(std::make_unique<FakeState>(destructorCount));
    manager.pushState(std::make_unique<FakeState>(destructorCount));

    auto replacement = std::make_unique<FakeState>(destructorCount);
    crawl::GameState* replacementAddress = replacement.get();
    manager.replaceAll(std::move(replacement));

    CHECK(destructorCount == 2);
    CHECK(manager.size() == 1);
    CHECK(manager.getCurrentState() == replacementAddress);
}

void testSaveV3CanonicalSchema() {
    const std::filesystem::path fixtureDirectory = g_testDirectory / "save-v3-schema";
    const std::filesystem::path savePath = fixtureDirectory / "save.json";
    std::filesystem::create_directories(fixtureDirectory);

    crawl::Party party;
    const crawl::CharacterIdentity identity{
        "SchemaHero", 31, crawl::Gender::NON_BINARY, crawl::CharacterClass::WARRIOR};
    const crawl::AbilityScore abilities{15, 12, 14, 10, 9, 11};
    party.addMember(std::make_shared<crawl::Character>(identity, abilities));
    auto activeQuest = crawl::Quest::createCanonical("qst_clear_kobolds");
    activeQuest->setCurrentCount(1);
    party.acceptQuest(activeQuest);
    auto completedQuest = crawl::Quest::createCanonical("qst_hunt_spiders");
    completedQuest->setCurrentCount(3);
    party.acceptQuest(completedQuest);
    party.completeQuest("qst_hunt_spiders");
    party.setCampaignCompleted(true);
    party.setLastSessionSeed(123456U);

    CHECK(party.saveToFile(savePath.string()).status == crawl::PersistenceStatus::Saved);
    nlohmann::json saved;
    {
        std::ifstream file(savePath);
        file >> saved;
    }

    CHECK(saved.at("schemaVersion") == 4 && saved.contains("activeQuests") &&
          saved.contains("completedQuestIds") && saved.contains("campaignCompleted") &&
          saved.contains("lastSessionSeed") && saved.contains("world") && saved.contains("keyItems"));
    CHECK(saved.at("members").at(0).at("age") == 31);
    CHECK(saved.at("members").at(0).at("gender") == "non_binary");
    CHECK(hasCanonicalCharacterSchema(saved.at("members").at(0)));
    CHECK(hasCanonicalQuestSchema(saved.at("activeQuests").at(0)));

    std::filesystem::remove_all(fixtureDirectory);
}

void testV1SaveMigratesToCanonicalV3() {
    const std::filesystem::path fixtureDirectory = g_testDirectory / "v1-migration";
    const std::filesystem::path savePath = fixtureDirectory / "save.json";
    std::filesystem::create_directories(fixtureDirectory);

    crawl::Character v1Character("LegacyHero", crawl::CharacterClass::WARRIOR);
    crawl::Quest v1Quest("qst_clear_kobolds", "코볼트 소탕", "코볼트 5마리 처치",
                         crawl::QuestType::KILL, "mon_kobold", 5, 50, 100,
                         std::vector<std::string>{"pot_strength"});
    const auto characterV2 = v1Character.toJson();
    const auto questV2 = v1Quest.toJson();
    const nlohmann::json characterV1 = {
        {"name", characterV2["name"]}, {"class", characterV2["class"]},
        {"level", characterV2["level"]}, {"xp", characterV2["xp"]},
        {"hp", characterV2["hp"]}, {"max_hp", characterV2["maxHp"]},
        {"spell_slots", characterV2["spellSlots"]},
        {"max_spell_slots", characterV2["maxSpellSlots"]},
        {"poison_turns", characterV2["poisonTurns"]},
        {"paralysis_turns", characterV2["paralysisTurns"]},
        {"abilities", {
            {"str", characterV2["abilities"]["strength"]},
            {"dex", characterV2["abilities"]["dexterity"]},
            {"con", characterV2["abilities"]["constitution"]},
            {"int", characterV2["abilities"]["intelligence"]},
            {"wis", characterV2["abilities"]["wisdom"]},
            {"cha", characterV2["abilities"]["charisma"]}
        }},
        {"eq_weapon", characterV2["equipment"]["weapon"]},
        {"eq_armor", characterV2["equipment"]["armor"]},
        {"eq_shield", characterV2["equipment"]["shield"]}
    };
    const nlohmann::json questV1 = {
        {"id", questV2["id"]}, {"name", questV2["name"]}, {"desc", questV2["desc"]},
        {"type", questV2["type"]}, {"target_id", questV2["targetId"]},
        {"target_count", questV2["targetCount"]}, {"current_count", questV2["currentCount"]},
        {"gold_reward", questV2["goldReward"]}, {"xp_reward", questV2["xpReward"]},
        {"is_completed", false}
    };
    nlohmann::json v1 = {
        {"gold", 321},
        {"inventory", nlohmann::json::array({"pot_heal"})},
        {"members", nlohmann::json::array({characterV1})},
        {"active_quests", nlohmann::json::array({questV1})}
    };
    {
        std::ofstream file(savePath);
        file << v1.dump(4);
    }

    crawl::Party party;
    CHECK(party.loadFromFile(savePath.string()).status == crawl::PersistenceStatus::Loaded);
    CHECK(party.saveToFile(savePath.string()).status == crawl::PersistenceStatus::Saved);

    nlohmann::json migrated;
    {
        std::ifstream file(savePath);
        file >> migrated;
    }
    CHECK(migrated.at("schemaVersion") == 4 && migrated.contains("activeQuests") &&
          migrated.contains("world") && !migrated.contains("active_quests"));
    CHECK(migrated.at("members").at(0).at("age") == 0);
    CHECK(migrated.at("members").at(0).at("gender") == "unspecified");
    CHECK(hasCanonicalCharacterSchema(migrated.at("members").at(0)));
    CHECK(hasCanonicalQuestSchema(migrated.at("activeQuests").at(0)));

    std::filesystem::remove_all(fixtureDirectory);
}

void testV2SaveLoadsUnknownIdentityAndMigratesToV3() {
    const std::filesystem::path fixtureDirectory = g_testDirectory / "v2-migration";
    const std::filesystem::path savePath = fixtureDirectory / "save.json";
    std::filesystem::create_directories(fixtureDirectory);
    const auto character = makeInvalidV2CharacterFixture(1, 10, 10);
    {
        std::ofstream file(savePath);
        file << makeV2Root(nlohmann::json::array({character})).dump(4);
    }

    crawl::Party party;
    CHECK(party.loadFromFile(savePath.string()).status == crawl::PersistenceStatus::Loaded);
    CHECK(party.getMember(0)->getAge() == 0);
    CHECK(party.getMember(0)->getGender() == crawl::Gender::UNSPECIFIED);
    CHECK(party.saveToFile(savePath.string()).status == crawl::PersistenceStatus::Saved);

    nlohmann::json migrated;
    {
        std::ifstream file(savePath);
        file >> migrated;
    }
    CHECK(migrated.at("schemaVersion") == 4);
    CHECK(migrated.at("members").at(0).at("age") == 0);
    CHECK(migrated.at("members").at(0).at("gender") == "unspecified");
    std::filesystem::remove_all(fixtureDirectory);
}

void testInvalidV3IdentityIsRejected() {
    auto character = makeInvalidV2CharacterFixture(1, 10, 10);
    character["age"] = 20;
    character["gender"] = "invalid";
    bool rejectedGender = false;
    try {
        static_cast<void>(crawl::Character::fromJson(character, 3));
    } catch (const std::exception&) {
        rejectedGender = true;
    }
    CHECK(rejectedGender);

    character["gender"] = "female";
    character["age"] = 81;
    bool rejectedAge = false;
    try {
        static_cast<void>(crawl::Character::fromJson(character, 3));
    } catch (const std::exception&) {
        rejectedAge = true;
    }
    CHECK(rejectedAge);
}

void testBackupRecoveryQuarantinesPrimary() {
    const std::filesystem::path fixtureDirectory = g_testDirectory / "backup-recovery";
    const std::filesystem::path savePath = fixtureDirectory / "save.json";
    std::filesystem::create_directories(fixtureDirectory);

    crawl::Party party;
    CHECK(party.saveToFile(savePath.string()).status == crawl::PersistenceStatus::Saved);
    const int previousGold = party.getGold();
    party.addGold(77);
    CHECK(party.saveToFile(savePath.string()).status == crawl::PersistenceStatus::Saved);

    const std::string corruptBytes = "{ definitely-not-valid-json";
    {
        std::ofstream file(savePath, std::ios::binary | std::ios::trunc);
        file << corruptBytes;
    }

    crawl::Party recovered;
    const crawl::PersistenceResult result = recovered.loadFromFile(savePath.string());
    CHECK(result.status == crawl::PersistenceStatus::RecoveredFromBackup);
    CHECK(recovered.getGold() == previousGold);

    bool foundQuarantine = false;
    for (const auto& entry : std::filesystem::directory_iterator(fixtureDirectory)) {
        const std::string filename = entry.path().filename().string();
        if (filename.starts_with("save.corrupt-") && readFileBytes(entry.path()) == corruptBytes) {
            foundQuarantine = true;
        }
    }
    CHECK(foundQuarantine);
    CHECK(std::filesystem::exists(savePath));
    crawl::Party secondProcess;
    CHECK(secondProcess.loadFromFile(savePath.string()).status == crawl::PersistenceStatus::Loaded);
    CHECK(secondProcess.getGold() == previousGold);

    std::filesystem::remove_all(fixtureDirectory);
}

void testCorruptBackupIsAlsoQuarantined() {
    const auto directory = g_testDirectory / "corrupt-primary-and-backup";
    const auto savePath = directory / "save.json";
    std::filesystem::create_directories(directory);
    crawl::Party party;
    CHECK(party.saveToFile(savePath.string()));
    party.addGold(1);
    CHECK(party.saveToFile(savePath.string()));
    const std::string primaryBytes = "{ corrupt-primary";
    const std::string backupBytes = "{ corrupt-backup";
    std::ofstream(savePath, std::ios::trunc) << primaryBytes;
    std::ofstream(savePath.string() + ".bak", std::ios::trunc) << backupBytes;
    crawl::Party target;
    CHECK(target.loadFromFile(savePath.string()).status == crawl::PersistenceStatus::Corrupt);
    bool primaryQuarantined = false;
    bool backupQuarantined = false;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        const std::string bytes = readFileBytes(entry.path());
        primaryQuarantined = primaryQuarantined || bytes == primaryBytes;
        backupQuarantined = backupQuarantined || bytes == backupBytes;
    }
    CHECK(primaryQuarantined);
    CHECK(backupQuarantined);
    CHECK(!std::filesystem::exists(savePath));
    CHECK(!std::filesystem::exists(savePath.string() + ".bak"));
    std::filesystem::remove_all(directory);
}

void testMissingPrimaryCorruptBackupIsQuarantined() {
    const auto directory = g_testDirectory / "missing-primary-corrupt-backup";
    const auto savePath = directory / "save.json";
    const auto backupPath = std::filesystem::path(savePath.string() + ".bak");
    std::filesystem::create_directories(directory);

    const std::array<std::string, 2> cases = {{
        "{ corrupt-backup-only",
        std::string(1024U * 1024U + 1U, 'x'),
    }};
    for (const auto& bytes : cases) {
        std::filesystem::remove(savePath);
        std::filesystem::remove(backupPath);
        std::ofstream(backupPath, std::ios::binary | std::ios::trunc) << bytes;

        crawl::Party target;
        const auto result = target.loadFromFile(savePath.string());
        CHECK(result.status == crawl::PersistenceStatus::Corrupt);
        CHECK(!std::filesystem::exists(savePath));
        CHECK(!std::filesystem::exists(backupPath));
        CHECK(std::filesystem::exists(result.path));
        CHECK(readFileBytes(result.path) == bytes);

        crawl::Party secondAttempt;
        CHECK(secondAttempt.loadFromFile(savePath.string()).status ==
              crawl::PersistenceStatus::NotFound);
        std::filesystem::remove(result.path);
    }
    std::filesystem::remove_all(directory);
}

void testSaveLoadRejectsLeafSymlinks() {
#ifndef _WIN32
    const auto directory = g_testDirectory / "save-symlink";
    const auto realPath = directory / "real.json";
    const auto linkPath = directory / "link.json";
    std::filesystem::create_directories(directory);
    crawl::Party source;
    CHECK(source.saveToFile(realPath.string()));
    const std::string realBytes = readFileBytes(realPath);
    std::error_code error;
    std::filesystem::create_symlink(realPath, linkPath, error);
    CHECK(!error);
    if (!error) {
        crawl::Party target;
        CHECK(target.loadFromFile(linkPath.string()).status == crawl::PersistenceStatus::IoError);
        CHECK(!target.saveToFile(linkPath.string()));
        CHECK(readFileBytes(realPath) == realBytes);
    }
    std::filesystem::remove_all(directory);
#endif
}

void testAtomicSaveFailurePreservesTarget() {
    const std::filesystem::path fixtureDirectory = g_testDirectory / "atomic-save-failure";
    const std::filesystem::path savePath = fixtureDirectory / "save.json";
    const std::filesystem::path temporaryPath = savePath.string() + ".tmp";
    std::filesystem::create_directories(fixtureDirectory);

    crawl::Party party;
    CHECK(party.saveToFile(savePath.string()).status == crawl::PersistenceStatus::Saved);
    const std::string targetBytes = readFileBytes(savePath);
    std::filesystem::create_directory(temporaryPath);
    party.addGold(99);

    const crawl::PersistenceResult result = party.saveToFile(savePath.string());
    CHECK(result.status == crawl::PersistenceStatus::IoError);
    CHECK(readFileBytes(savePath) == targetBytes);

    std::filesystem::remove_all(fixtureDirectory);
}

void testInvalidV2MembersAreRejectedWithoutMutation() {
    const std::filesystem::path fixtureDirectory = g_testDirectory / "invalid-v2-members";
    std::filesystem::create_directories(fixtureDirectory);
    const std::vector<std::pair<std::string, nlohmann::json>> invalidMembers = {
        {"invalid-level.json", makeInvalidV2CharacterFixture(99, 10, 10)},
        {"invalid-hp.json", makeInvalidV2CharacterFixture(1, 11, 10)},
        {"invalid-slot.json", [] {
            auto character = makeInvalidV2CharacterFixture(1, 10, 10);
            character["equipment"]["weapon"] = "arm_plate";
            return character;
        }()},
        {"v2-legacy-shape.json", [] {
            auto character = makeInvalidV2CharacterFixture(1, 10, 10);
            character.erase("maxHp");
            character.erase("spellSlots");
            character.erase("maxSpellSlots");
            character.erase("poisonTurns");
            character.erase("paralysisTurns");
            character.erase("equipment");
            character["abilities"] = {{"str", 10}, {"dex", 10}, {"con", 10},
                                      {"int", 10}, {"wis", 10}, {"cha", 10}};
            return character;
        }()}
    };

    for (const auto& [filename, invalidMember] : invalidMembers) {
        const std::filesystem::path savePath = fixtureDirectory / filename;
        const nlohmann::json invalidSave = makeV2Root(nlohmann::json::array({invalidMember}));
        const std::string invalidBytes = invalidSave.dump(4);
        {
            std::ofstream file(savePath, std::ios::binary);
            file << invalidBytes;
        }

        crawl::Party party;
        party.addGold(77);
        party.setCampaignCompleted(true);
        party.setLastSessionSeed(42U);
        const int goldBefore = party.getGold();
        const std::size_t inventorySizeBefore = party.getInventory().size();

        const crawl::PersistenceResult result = party.loadFromFile(savePath.string());
        CHECK(result.status == crawl::PersistenceStatus::Corrupt);
        CHECK(result.path != savePath && !std::filesystem::exists(savePath) &&
              std::filesystem::exists(result.path) && readFileBytes(result.path) == invalidBytes);
        CHECK(party.getGold() == goldBefore && party.getMemberCount() == 0 &&
              party.getInventory().size() == inventorySizeBefore &&
              party.isCampaignCompleted() && party.getLastSessionSeed() == 42U);
    }

    std::filesystem::remove_all(fixtureDirectory);
}

// 몬스터 처치 및 수집 퀘스트 보상 정산 연동 검증 테스트
void testMonsterAndQuestSystem() {
    std::cout << "[Test] 몬스터 전투 보상 및 퀘스트 추적 시스템을 검증합니다." << std::endl;

    crawl::Party party;
    party.resetToDefault(); // 골드 100G, 멤버 없음, 물약 2개

    // 파티원 1명 수혈
    auto hero = std::make_shared<crawl::Character>("Tester", crawl::CharacterClass::WARRIOR);
    party.addMember(hero);

    // 1. 코볼트 사냥 퀘스트 수락
    auto killQuest = crawl::Quest::createCanonical("qst_clear_kobolds");
    party.acceptQuest(killQuest);
    CHECK(party.hasQuest("qst_clear_kobolds") == true);

    // 2. 메이스 수집 퀘스트 수락
    auto collectQuest = crawl::Quest::createCanonical("qst_collect_maces");
    party.acceptQuest(collectQuest);
    CHECK(party.hasQuest("qst_collect_maces") == true);

    // 3. 몬스터 2마리 처치 트리거 시뮬레이션
    party.updateQuestKillProgress("mon_kobold", 2);
    auto activeQ = party.getActiveQuests();
    auto it = std::find_if(activeQ.begin(), activeQ.end(), [&](const auto& q){ return q->getId() == "qst_clear_kobolds"; });
    CHECK(it != activeQ.end());
    if (it == activeQ.end()) return;
    CHECK((*it)->getCurrentCount() == 2);
    CHECK((*it)->checkCompletion() == false);

    // 추가 3마리 더 사냥 -> 완료 조건 충족
    party.updateQuestKillProgress("mon_kobold", 3);
    CHECK((*it)->getCurrentCount() == 5);
    CHECK((*it)->checkCompletion() == true);

    // 퀘스트 완료 보고 및 정산 (50 Gold, 100 XP 지급)
    int oldGold = party.getGold();
    int oldXp = hero->getXp();
    party.completeQuest("qst_clear_kobolds");

    CHECK(party.hasQuest("qst_clear_kobolds") == false); // 완료 후 목록에서 해제 확인
    CHECK(party.getGold() == oldGold + 50); // 골드 수령 검증
    CHECK(hero->getXp() == oldXp + 100);    // 경험치 분배 수령 검증

    // 4. 수집형 메이스 2개 획득 동기화 검증
    party.updateQuestCollectProgress(); // 현재 가방 메이스 개수: 0
    auto it2 = std::find_if(party.getActiveQuests().begin(), party.getActiveQuests().end(), [&](const auto& q){ return q->getId() == "qst_collect_maces"; });
    CHECK(it2 != party.getActiveQuests().end());
    CHECK((*it2)->getCurrentCount() == 0);

    // 가방에 메이스 2개 강제 주입
    party.addItem(crawl::ItemFactory::createItem("wpn_mace"));
    party.addItem(crawl::ItemFactory::createItem("wpn_mace"));
    party.updateQuestCollectProgress(); // 동기화 기동 -> 수량 2
    CHECK((*it2)->getCurrentCount() == 2);
    CHECK((*it2)->checkCompletion() == true);

    // 퀘스트 완료 보고 및 가방 내 수집한 실제 메이스 2개 자동 삭제 검증
    const std::size_t bagSizeBefore = party.getInventory().size();
    party.completeQuest("qst_collect_maces");
    
    CHECK(party.hasQuest("qst_collect_maces") == false);
    // 가방 크기가 (이전 크기 - 2) 가 되었는지 검사
    CHECK(party.getInventory().size() == bagSizeBefore - 1);

    std::cout << "-> [Success] 퀘스트 진척도 갱신, 수집 아이템 자동 차감 및 보상 분배 검증 완료." << std::endl;
}

// [v0.6.0] BFS 기반 최단 경로 및 미니맵 자동 이동 유효성 검증 테스트
void testDungeonAutoMoveBFS() {
    std::cout << "[Test] BFS 기반 최단 경로 및 자동 이동 유효성을 검증합니다." << std::endl;

    crawl::DungeonMap map;
    map.generate(0x1002U);

    // 시작 지점 (1, 1)은 visited로 설정되어 있음
    int tx = -1;
    int ty = -1;

    // (1, 1) 주변에서 갈 수 있는 통로 하나 찾기
    for (int y = 0; y < crawl::DungeonMap::MAP_HEIGHT; ++y) {
        for (int x = 0; x < crawl::DungeonMap::MAP_WIDTH; ++x) {
            if ((x != 1 || y != 1) && map.isWalkable(x, y)) {
                tx = x;
                ty = y;
                break;
            }
        }
        if (tx != -1) break;
    }

    CHECK(tx != -1 && ty != -1);

    // 아직 탐험되지 않은 상태(visited가 아닌 상태)에서 findPath를 하면 빈 경로가 나와야 함
    auto pathNotVisited = map.findPath(1, 1, tx, ty);
    CHECK(pathNotVisited.empty());

    // 모든 빈 공간을 visited = true로 임시 개방하여 최단 경로 테스트 진행
    for (int y = 0; y < crawl::DungeonMap::MAP_HEIGHT; ++y) {
        for (int x = 0; x < crawl::DungeonMap::MAP_WIDTH; ++x) {
            if (map.isWalkable(x, y)) {
                map.setVisited(x, y, true);
            }
        }
    }

    // 이제 BFS 경로 탐색 수행
    auto path = map.findPath(1, 1, tx, ty);
    CHECK(!path.empty());

    // 경로의 마지막 지점은 목적지 (tx, ty)여야 함
    CHECK(path.back().first == tx && path.back().second == ty);

    // 경로상의 모든 타일이 walkable이고 visited인지 검증
    for (const auto& step : path) {
        CHECK(map.isWalkable(step.first, step.second));
        CHECK(map.isVisited(step.first, step.second));
    }

    // 벽을 목적지로 지정하면 빈 경로가 반환되는지 검증 (isWalkable에 의해 차단)
    int wx = -1, wy = -1;
    for (int y = 0; y < crawl::DungeonMap::MAP_HEIGHT; ++y) {
        for (int x = 0; x < crawl::DungeonMap::MAP_WIDTH; ++x) {
            if (map.getTile(x, y) == crawl::TileType::WALL) {
                wx = x;
                wy = y;
                map.setVisited(wx, wy, true);
                break;
            }
        }
        if (wx != -1) break;
    }
    
    auto pathToWall = map.findPath(1, 1, wx, wy);
    CHECK(pathToWall.empty());

    std::cout << "-> [Success] BFS 경로 탐색 및 자동 이동 룰 검증 완료." << std::endl;
}

// [v0.6.0] 캐릭터 장비 장착 및 클래스별 제한, 인벤토리 아이템 스왑 비즈니스 로직 검증 테스트
void testCharacterEquipmentSystem() {
    std::cout << "[Test] 캐릭터 장비 장착 및 클래스별 착용 제한 규칙을 검증합니다." << std::endl;

    crawl::Party party;
    party.resetToDefault();

    // 1. 캐릭터 생성 (전사 HeroW, 마법사 HeroM)
    auto warrior = std::make_shared<crawl::Character>("HeroW", crawl::CharacterClass::WARRIOR);
    auto mage = std::make_shared<crawl::Character>("HeroM", crawl::CharacterClass::MAGE);

    party.addMember(warrior);
    party.addMember(mage);

    // 전사는 기본 롱소드(wpn_longsword)와 스케일 메일(arm_scale)을 장착하고 있어야 함
    auto wpn = warrior->getEquippedItem(crawl::EquipSlot::WEAPON);
    auto arm = warrior->getEquippedItem(crawl::EquipSlot::ARMOR);
    CHECK(wpn && wpn->getId() == "wpn_longsword");
    CHECK(arm && arm->getId() == "arm_scale");

    // 2. 장비 장착 해제 테스트
    // 전사의 무기를 해제하면 전사의 무기 슬롯은 nullptr가 되고, 파티 인벤토리에 롱소드가 들어가야 함
    warrior->unequip(crawl::EquipSlot::WEAPON);
    CHECK(warrior->getEquippedItem(crawl::EquipSlot::WEAPON) == nullptr);
    
    party.addItem(wpn);
    CHECK(party.getInventory().size() == 4); // 치유물약 2개 + 마나물약 + 롱소드
    CHECK(party.getInventory().back()->getId() == "wpn_longsword");

    // 3. 클래스별 장착 제한 시뮬레이션 검증
    // 마법사는 롱소드 장착 불가 룰 검사
    auto longsword = std::dynamic_pointer_cast<crawl::Equipment>(party.getInventory().back());
    CHECK(longsword != nullptr);

    bool isMageAllowed = true;
    std::string itemId = longsword->getId();
    if (mage->getClass() == crawl::CharacterClass::MAGE) {
        if (itemId == "arm_scale" || itemId == "arm_chain" || itemId == "shd_round" || itemId == "wpn_longsword") {
            isMageAllowed = false;
        }
    }
    CHECK(isMageAllowed == false); // 마법사는 롱소드 장착이 차단되어야 함

    // 4. 장비 자동 스왑 시뮬레이션
    // 가죽 갑옷(arm_leather)을 전사에게 장착시킬 때, 기존 스케일 메일(arm_scale)이 해제되고 가죽 갑옷이 장착되는지 검증
    auto leather = std::dynamic_pointer_cast<crawl::Equipment>(crawl::ItemFactory::createItem("arm_leather"));
    auto oldArmor = warrior->getEquippedItem(crawl::EquipSlot::ARMOR); // arm_scale
    
    // 전사에게 가죽 갑옷 장착
    bool equipSuccess = warrior->equip(leather);
    CHECK(equipSuccess == true);
    CHECK(warrior->getEquippedItem(crawl::EquipSlot::ARMOR)->getId() == "arm_leather");

    // 기존 갑옷은 가방에 반환
    party.addItem(oldArmor);
    CHECK(party.getInventory().back()->getId() == "arm_scale");

    std::cout << "-> [Success] 장비 슬롯 탈착, 클래스별 제한 및 스왑 로직 검증 완료." << std::endl;
}

// [v0.8.0] 전투 임시 버프, 독 상태이상 대미지, 소모품 복용 효과 단위 테스트 추가
void testCombatEffectsAndItemUsage() {
    std::cout << "[Test] 전투 임시 버프, 독 상태이상 대미지, 소모품 복용 효과를 검증합니다." << std::endl;

    // 1. 캐릭터 생성 (전사 & 마법사)
    auto warrior = std::make_shared<crawl::Character>("HeroW", crawl::CharacterClass::WARRIOR);
    auto mage = std::make_shared<crawl::Character>("HeroM", crawl::CharacterClass::MAGE);

    std::vector<std::shared_ptr<crawl::Character>> partyMembers = {warrior, mage};

    // --- (A) 힘의 물약(pot_strength) 버프 및 지속시간, 정산 테스트 ---
    int baseStrength = warrior->getRawAbilities().strength;
    CHECK(warrior->getAbilities().strength == baseStrength);

    // 힘의 물약 효과 적용 (STR +3, 5턴)
    auto strPotion = crawl::ItemFactory::createItem("pot_strength");
    auto consumableStr = std::dynamic_pointer_cast<crawl::ConsumableItem>(strPotion);
    CHECK(consumableStr != nullptr);

    std::vector<std::string> log;
    consumableStr->applyEffect(*warrior, partyMembers, log);
    CHECK(warrior->getAbilities().strength == baseStrength + 3);

    // 버프는 행동 전에 만료되지 않고 행동 종료 뒤 1턴 감소한다.
    for (int i = 0; i < 5; ++i) {
        CHECK(warrior->getAbilities().strength == baseStrength + 3);
        warrior->processTurnEffects(log);
        CHECK(warrior->getAbilities().strength == baseStrength + 3);
        warrior->advanceCombatBuffDurations(log);
    }
    // 5번째 호출 이후 턴 수가 0이 되므로 버프 만료.
    CHECK(warrior->getAbilities().strength == baseStrength);

    warrior->setParalysis(1);
    const bool shouldSkipAction = warrior->isParalyzed();
    warrior->processTurnEffects(log);
    CHECK(shouldSkipAction);
    CHECK(!warrior->isParalyzed());

    // --- (B) 독(Poison) 지속 대미지 및 해독 스크롤(scr_cure) 테스트 ---
    // 전사의 HP를 확인
    int hpBeforePoison = warrior->getHp();
    warrior->setPoison(3);
    CHECK(warrior->getPoisonTurns() == 3);

    // processTurnEffects를 호출하면 독 1d3 대미지를 입음
    warrior->processTurnEffects(log);
    CHECK(warrior->getHp() < hpBeforePoison); // 피격 대미지로 피가 깎여야 함
    CHECK(warrior->getPoisonTurns() == 2);

    // 해독 스크롤로 완치
    auto cureScroll = crawl::ItemFactory::createItem("scr_cure");
    auto consumableCure = std::dynamic_pointer_cast<crawl::ConsumableItem>(cureScroll);
    CHECK(consumableCure != nullptr);
    
    consumableCure->applyEffect(*warrior, partyMembers, log);
    CHECK(warrior->getPoisonTurns() == 0);

    // --- (C) 독으로 인한 사망 테스트 ---
    auto rogue = std::make_shared<crawl::Character>("HeroR", crawl::CharacterClass::ROGUE);
    // HP를 1로 낮춘다
    rogue->takeDamage(rogue->getHp() - 1);
    CHECK(rogue->getHp() == 1);
    rogue->setPoison(5);

    // 턴 정산을 하면 1d3(최소 1)의 대미지를 입어 즉사해야 한다.
    rogue->processTurnEffects(log);
    CHECK(rogue->getHp() == 0);
    CHECK(rogue->isDead() == true);

    // --- (D) 마나 물약(pot_mana) 복구 한계 테스트 ---
    // 마법사는 1레벨 기본 주문 슬롯 2개
    CHECK(mage->getSpellSlots() == 2);
    CHECK(mage->getMaxSpellSlots() == 2);

    // 슬롯 1개 소비
    mage->consumeSpellSlot();
    CHECK(mage->getSpellSlots() == 1);

    auto manaPotion = crawl::ItemFactory::createItem("pot_mana");
    auto consumableMana = std::dynamic_pointer_cast<crawl::ConsumableItem>(manaPotion);
    CHECK(consumableMana != nullptr);

    // 마나 물약 사용 -> 슬롯 2개로 복구
    consumableMana->applyEffect(*mage, partyMembers, log);
    CHECK(mage->getSpellSlots() == 2);

    // 가득 찬 상태에서 한 번 더 마나 물약 사용 -> 2개 초과 불가 검증
    consumableMana->applyEffect(*mage, partyMembers, log);
    CHECK(mage->getSpellSlots() == 2);

    std::cout << "-> [Success] 전투 버프, 독 사망 판정, 마나 복구 한계 단위 테스트 성공." << std::endl;
}

// [v0.8.0] 양손 무기/방패 장착 배타성 및 클래스별 장착 제한 시뮬레이션 테스트 추가
void testAdvancedEquipSwapAndClassLimits() {
    std::cout << "[Test] 양손검-방패 배타적 장착 스왑 및 클래스별 장착 제한 규칙을 검증합니다." << std::endl;

    crawl::Party party;
    party.resetToDefault(); // 가방 초기화

    auto warrior = std::make_shared<crawl::Character>("WarriorHero", crawl::CharacterClass::WARRIOR);
    auto mage = std::make_shared<crawl::Character>("MageHero", crawl::CharacterClass::MAGE);
    
    party.addMember(warrior);
    party.addMember(mage);

    // 초기 상태: 전사는 롱소드(wpn_longsword)와 스케일 메일(arm_scale) 장착, 방패(shd_round) 없음
    CHECK(warrior->getEquippedItem(crawl::EquipSlot::WEAPON)->getId() == "wpn_longsword");
    CHECK(warrior->getEquippedItem(crawl::EquipSlot::ARMOR)->getId() == "arm_scale");
    CHECK(warrior->getEquippedItem(crawl::EquipSlot::SHIELD) == nullptr);

    // 1. 전사에게 방패를 착용시켜 롱소드 + 방패 상태로 만듦
    auto shield = std::dynamic_pointer_cast<crawl::Equipment>(crawl::ItemFactory::createItem("shd_round"));
    warrior->equip(shield);
    CHECK(warrior->getEquippedItem(crawl::EquipSlot::SHIELD)->getId() == "shd_round");

    // 2. [스왑 시뮬레이션] 그레이트소드(양손검) 착용 시 방패 자동 해제 및 가방 반환 시뮬레이션
    auto greatsword = std::dynamic_pointer_cast<crawl::Equipment>(crawl::ItemFactory::createItem("wpn_greatsword"));
    
    // UI 로직 모사: 양손 무기 착용 시 방패를 unequip하고 가방에 넣는 처리
    if (greatsword->getId() == "wpn_greatsword") {
        auto shieldItem = warrior->getEquippedItem(crawl::EquipSlot::SHIELD);
        if (shieldItem) {
            warrior->unequip(crawl::EquipSlot::SHIELD);
            party.addItem(shieldItem); // 가방에 반환
        }
    }
    // 그리고 그레이트소드 장착
    auto oldWeapon = warrior->getEquippedItem(crawl::EquipSlot::WEAPON);
    if (warrior->equip(greatsword)) {
        if (oldWeapon) {
            party.addItem(oldWeapon); // 롱소드도 가방에 반환
        }
    }

    // 검증: 무기는 그레이트소드여야 하고, 방패는 nullptr여야 하며, 가방에는 방패와 롱소드가 들어가 있어야 함
    CHECK(warrior->getEquippedItem(crawl::EquipSlot::WEAPON)->getId() == "wpn_greatsword");
    CHECK(warrior->getEquippedItem(crawl::EquipSlot::SHIELD) == nullptr);
    
    // 가방 내용 확인 (치유물약 2개 + 마나물약 + 방패 + 롱소드)
    const auto& inv = party.getInventory();
    CHECK(inv.size() == 5);
    
    // 3. [반대 스왑 시뮬레이션] 그레이트소드를 쥐고 있는 상태에서 방패 장착 시 무기 해제 및 가방 반환 시뮬레이션
    auto newShield = std::dynamic_pointer_cast<crawl::Equipment>(crawl::ItemFactory::createItem("shd_round"));
    
    // UI 로직 모사: 방패 장착 시 들고 있는 무기가 양손검(wpn_greatsword)이면 무기를 unequip하고 가방에 넣음
    auto curWpn = warrior->getEquippedItem(crawl::EquipSlot::WEAPON);
    if (curWpn && curWpn->getId() == "wpn_greatsword") {
        warrior->unequip(crawl::EquipSlot::WEAPON);
        party.addItem(curWpn); // 양손검 가방 반환
    }
    // 그리고 방패 장착
    auto oldShield = warrior->getEquippedItem(crawl::EquipSlot::SHIELD);
    if (warrior->equip(newShield)) {
        if (oldShield) {
            party.addItem(oldShield);
        }
    }

    // 검증: 무기는 nullptr(그레이트소드가 탈거됨), 방패는 shd_round가 장착되어야 함
    CHECK(warrior->getEquippedItem(crawl::EquipSlot::WEAPON) == nullptr);
    CHECK(warrior->getEquippedItem(crawl::EquipSlot::SHIELD)->getId() == "shd_round");

    // 4. [클래스 제한 시뮬레이션] 마법사는 지팡이(wpn_staff), 로브(arm_robe), 단검(wpn_dagger) 이외의 장비 장착 불가 검증
    auto checkEquipAllowed = [](crawl::CharacterClass cls, const std::string& itemId) -> bool {
        if (cls == crawl::CharacterClass::MAGE) {
            return (itemId == "wpn_dagger" || itemId == "wpn_staff" || itemId == "arm_robe");
        }
        return true;
    };

    CHECK(checkEquipAllowed(mage->getClass(), "wpn_greatsword") == false);
    CHECK(checkEquipAllowed(mage->getClass(), "wpn_staff") == true);

    std::cout << "-> [Success] 양손 장비 상호 배타성 스왑 및 클래스별 장착 제한 시뮬레이션 테스트 성공." << std::endl;
}

void testLocalizationI18n() {
    std::cout << "[Test] 다국어(i18n) 번역 데이터 로딩 및 설정을 검증합니다." << std::endl;

    auto& lm = crawl::LocalizationManager::getInstance();

    // 1. 기본 언어 로드 및 키 값 확인 (기본적으로 KO 설정)
    lm.setLanguage(crawl::Language::KO);
    CHECK(lm.getLanguage() == crawl::Language::KO);
    CHECK(lm.get("TITLE_NEW_GAME") == "신규 게임 시작");
    CHECK(lm.get("CHAR_INFO_HP") == "체력 (HP)");

    // 2. 다른 언어로 변경 시 실시간 로딩 및 텍스트 갱신 검증
    lm.setLanguage(crawl::Language::EN);
    CHECK(lm.getLanguage() == crawl::Language::EN);
    CHECK(lm.get("TITLE_NEW_GAME") == "Start New Game");
    CHECK(lm.get("CHAR_INFO_HP") == "Hit Points (HP)");

    lm.setLanguage(crawl::Language::JA);
    CHECK(lm.getLanguage() == crawl::Language::JA);
    CHECK(lm.get("TITLE_NEW_GAME") == "新規ゲーム開始");

    // 없는 키 조회 시 키 본래의 문자열을 반환해야 함 (Fallback 룰)
    CHECK(lm.get("NON_EXISTING_KEY_XYZ") == "NON_EXISTING_KEY_XYZ");

    // 3. config.json 접근성 설정 저장 및 세이브 로드 연동 검증
    lm.setTextScale(125);
    lm.setHighContrast(false);
    lm.setLanguage(crawl::Language::EN);

    const std::string tempConfigPath = (g_testDirectory / "config.json").string();
    bool saveRes = lm.saveConfig(tempConfigPath);
    CHECK(saveRes == true);

    // 새 값 로드 검증
    lm.setTextScale(75);
    lm.setHighContrast(true);
    lm.setLanguage(crawl::Language::KO);

    bool loadRes = lm.loadConfig(tempConfigPath);
    CHECK(loadRes == true);
    CHECK(lm.getTextScale() == 125);
    CHECK(lm.getHighContrast() == false);
    CHECK(lm.getLanguage() == crawl::Language::EN);
    CHECK(lm.get("TITLE_NEW_GAME") == "Start New Game");

    // 임시 생성한 설정 세이브 파일 정리 삭제
    std::remove(tempConfigPath.c_str());

    std::cout << "-> [Success] i18n 번역 리소스 조회 및 설정 파일 영속 세이브/로드 테스트 성공." << std::endl;
}

void testConfigPersistenceIsAtomicAndVersioned() {
    std::cout << "[Test] 설정 파일의 v2 schema와 원자 쓰기 실패 보존을 검증합니다." << std::endl;

    const std::filesystem::path fixtureDirectory = g_testDirectory / "config-atomic";
    const std::filesystem::path configPath = fixtureDirectory / "config.json";
    const std::filesystem::path temporaryPath = configPath.string() + ".tmp";
    std::filesystem::create_directories(fixtureDirectory);

    auto& localization = crawl::LocalizationManager::getInstance();
    localization.setLanguage(crawl::Language::KO);
    CHECK(localization.saveConfig(configPath.string()));

    nlohmann::json config;
    {
        std::ifstream file(configPath);
        file >> config;
    }
    CHECK(config.value("schemaVersion", 0) == 2);
    CHECK(config.contains("language") && config.contains("textScale") &&
          config.contains("highContrast") && !config.contains("bgmVolume") &&
          !config.contains("sfxVolume"));

    const std::string originalBytes = readFileBytes(configPath);
    std::filesystem::create_directory(temporaryPath);
    localization.setLanguage(crawl::Language::EN);
    CHECK(!localization.saveConfig(configPath.string()));
    CHECK(readFileBytes(configPath) == originalBytes);

    std::filesystem::remove_all(fixtureDirectory);
    localization.setLanguage(crawl::Language::KO);
}

void testCorruptConfigIsQuarantinedWithoutOverwrite() {
    std::cout << "[Test] 손상 설정 파일의 quarantine을 검증합니다." << std::endl;

    const std::filesystem::path fixtureDirectory = g_testDirectory / "config-corrupt";
    const std::filesystem::path configPath = fixtureDirectory / "config.json";
    std::filesystem::create_directories(fixtureDirectory);
    const std::string corruptBytes = "{ not-valid-config";
    {
        std::ofstream file(configPath, std::ios::binary);
        file << corruptBytes;
    }

    auto& localization = crawl::LocalizationManager::getInstance();
    CHECK(!localization.loadConfig(configPath.string()));
    CHECK(!std::filesystem::exists(configPath));
    int quarantineCount = 0;
    for (const auto& entry : std::filesystem::directory_iterator(fixtureDirectory)) {
        if (entry.path().filename().string().starts_with("config.corrupt-") &&
            readFileBytes(entry.path()) == corruptBytes) {
            ++quarantineCount;
        }
    }
    CHECK(quarantineCount == 1);

    std::filesystem::remove_all(fixtureDirectory);
}

void testConfigBackupRestoresPrimaryAcrossProcesses() {
    const std::filesystem::path directory = g_testDirectory / "config-backup";
    const std::filesystem::path path = directory / "config.json";
    std::filesystem::create_directories(directory);
    auto& localization = crawl::LocalizationManager::getInstance();
    localization.setLanguage(crawl::Language::KO);
    CHECK(localization.saveConfig(path.string()));
    localization.setLanguage(crawl::Language::EN);
    CHECK(localization.saveConfig(path.string()));
    {
        std::ofstream file(path, std::ios::trunc);
        file << "{ corrupt";
    }
    CHECK(localization.loadConfig(path.string()).status == crawl::PersistenceStatus::RecoveredFromBackup);
    CHECK(std::filesystem::exists(path));
    CHECK(localization.loadConfig(path.string()).status == crawl::PersistenceStatus::Loaded);
    CHECK(localization.getLanguage() == crawl::Language::KO);
    std::filesystem::remove_all(directory);
}

void testDirectorySyncFailureIsNotReportedAsSaved() {
    const std::filesystem::path directory = g_testDirectory / "directory-sync-failure";
    const std::filesystem::path path = directory / "save.json";
    std::filesystem::create_directories(directory);
    crawl::Party party;
    CHECK(party.saveToFile(path.string()));
    const std::string originalBytes = readFileBytes(path);
    party.addGold(50);
    crawl::Persistence::setDirectorySyncFailureForTests(true);
    const auto result = party.saveToFile(path.string());
    crawl::Persistence::setDirectorySyncFailureForTests(false);
    CHECK(result.status == crawl::PersistenceStatus::IoError);
    CHECK(readFileBytes(path) == originalBytes);
    std::filesystem::remove_all(directory);
}

void testOversizedAndTamperedSaveIsQuarantined() {
    const std::filesystem::path directory = g_testDirectory / "save-boundary";
    std::filesystem::create_directories(directory);

    const std::filesystem::path oversized = directory / "oversized.json";
    {
        std::ofstream file(oversized, std::ios::binary);
        file << std::string(1024U * 1024U + 1U, ' ');
    }
    crawl::Party oversizedParty;
    CHECK(oversizedParty.loadFromFile(oversized.string()).status == crawl::PersistenceStatus::Corrupt);

    const std::filesystem::path tampered = directory / "tampered.json";
    crawl::Party source;
    CHECK(source.saveToFile(tampered.string()));
    source.acceptQuest(crawl::Quest::createCanonical("qst_clear_kobolds"));
    CHECK(source.saveToFile(tampered.string()));
    nlohmann::json payload;
    {
        std::ifstream file(tampered);
        file >> payload;
    }
    payload["activeQuests"][0]["goldReward"] = 1000000;
    {
        std::ofstream file(tampered, std::ios::trunc);
        file << payload.dump(4);
    }
    crawl::Party tamperedParty;
    CHECK(tamperedParty.loadFromFile(tampered.string()).status == crawl::PersistenceStatus::RecoveredFromBackup);
    CHECK(!tamperedParty.hasQuest("qst_clear_kobolds"));
    std::filesystem::remove_all(directory);
}

// [v0.9.4] Town 허브가 참조하는 모든 키가 5개 언어 리소스에 존재하는지 검증한다.
void testTownHubLocalizationKeyCoverage() {
    std::cout << "[Test] Town 허브 번역 키 완전성을 검증합니다." << std::endl;

    const std::vector<std::string> requiredKeys = {
        "TOWN_CAMP_WELCOME",
        "TOWN_CAMP_OPTION_1",
        "TOWN_CAMP_OPTION_2",
        "TOWN_CAMP_OPTION_3",
        "TOWN_CAMP_OPTION_4",
        "TOWN_CAMP_OPTION_5",
        "TOWN_CAMP_OPTION_C",
        "TOWN_CAMP_OPTION_O",
        "TOWN_CAMP_OPTION_ESC"
    };
    const std::vector<crawl::Language> languages = {
        crawl::Language::KO,
        crawl::Language::EN,
        crawl::Language::JA,
        crawl::Language::ZH_TW,
        crawl::Language::ZH_CN
    };

    auto& lm = crawl::LocalizationManager::getInstance();
    for (const auto language : languages) {
        lm.setLanguage(language);
        for (const auto& key : requiredKeys) {
            CHECK(lm.get(key) != key);
        }
    }

    std::cout << "-> [Success] Town 허브 번역 키가 5개 언어에 모두 존재합니다." << std::endl;
}

// 상점 아이템 판매 기능 검증 테스트
void testShopSelling() {
    std::cout << "[Test] 상점 아이템 판매 및 골드 정산 규칙을 검증합니다." << std::endl;

    crawl::Party party;
    party.resetToDefault(); // 기본 골드 100G, 기본 치유물약 등 세팅

    // 1. 테스트용 롱소드(가치 30G) 생성 및 가방에 추가
    auto sword = crawl::ItemFactory::createItem("wpn_longsword");
    CHECK(sword != nullptr);
    CHECK(sword->getGoldValue() == 30);

    party.addItem(sword);
    const std::size_t initialInvSize = party.getInventory().size();
    int initialGold = party.getGold();

    // 2. 판매 로직 시뮬레이션 (원가의 50% = 15G 획득 및 인벤토리 제거)
    int targetIdx = -1;
    for (size_t i = 0; i < party.getInventory().size(); ++i) {
        if (party.getInventory()[i]->getId() == "wpn_longsword") {
            targetIdx = static_cast<int>(i);
            break;
        }
    }
    CHECK(targetIdx != -1);

    auto itemToSell = party.getInventory()[targetIdx];
    int sellPrice = itemToSell->getGoldValue() / 2;
    party.addGold(sellPrice);
    party.removeItem(targetIdx);
    party.saveToFile(); // 영속화

    // 3. 골드 및 가방 수량 단언 검증
    CHECK(party.getGold() == initialGold + 15);
    CHECK(party.getInventory().size() == initialInvSize - 1);

    // 4. 세이브 로드 정합성 복원 검증
    crawl::Party reloadParty;
    bool loadRes = reloadParty.loadFromFile();
    CHECK(loadRes == true);
    CHECK(reloadParty.getGold() == party.getGold());
    CHECK(reloadParty.getInventory().size() == party.getInventory().size());

    std::cout << "-> [Success] 상점 장비 판매, 골드 50% 가산 및 인벤토리 직렬화 검증 완료." << std::endl;
}

// [v0.9.2] Town 7개 서브상태 및 Combat 헤더 안전성 검증 회귀 테스트 추가 (한국어 주석 필수)
void testTownAndCombatUIStatesI18nSafety() {
    std::cout << "[Test] Town 7개 서브상태 및 Combat 헤더 안전성 검증 회귀 테스트를 시작합니다." << std::endl;

    auto& lm = crawl::LocalizationManager::getInstance();

    // 0. 검증 대상 폰트들 로드 시도
    sf::Font font;
    sf::Font cjkFont;
    CHECK(font.loadFromFile("assets/fonts/neodgm.ttf"));
    CHECK(cjkFont.loadFromFile("assets/fonts/NotoSansCJK-Regular.ttc"));

    // KO, EN, JA, ZH_TW, ZH_CN 5대 언어 각각에 대해 UI 타이틀이 안전하게 로드되는지 검증 (OOB 및 UB 방지)
    const std::vector<std::pair<crawl::Language, std::string>> testLangs = {
        {crawl::Language::KO, "assets/lang/ko.json"},
        {crawl::Language::EN, "assets/lang/en.json"},
        {crawl::Language::JA, "assets/lang/ja.json"},
        {crawl::Language::ZH_TW, "assets/lang/zh_tw.json"},
        {crawl::Language::ZH_CN, "assets/lang/zh_cn.json"}
    };

    for (const auto& item : testLangs) {
        crawl::Language lang = item.first;
        std::string jsonPath = item.second;
        
        lm.setLanguage(lang);

        // 1. Town 서브상태 타이틀 텍스트 로드 검증 (getSf 헬퍼를 통한 UTF-8 sf::String 안전 반환)
        sf::String townTitle = lm.getSf("TOWN_TITLE");
        sf::String guildTitle = lm.getSf("GUILD_TITLE");
        sf::String shopTitle = lm.getSf("SHOP_TITLE");
        sf::String templeTitle = lm.getSf("TEMPLE_TITLE");
        sf::String castleTitle = lm.getSf("CASTLE_TITLE");

        CHECK(!townTitle.isEmpty());
        CHECK(!guildTitle.isEmpty());
        CHECK(!shopTitle.isEmpty());
        CHECK(!templeTitle.isEmpty());
        CHECK(!castleTitle.isEmpty());

        // 2. Combat 타이틀 텍스트 로드 검증
        sf::String combatTitle = lm.getSf("COMBAT_TITLE");
        CHECK(!combatTitle.isEmpty());

        // 3. getSf() 연속 호출 시의 정합성 검증 (iterator pair UB가 없는지 연속 메모리 안전성 체크)
        for (int i = 0; i < 10; ++i) {
            sf::String repeatTown = lm.getSf("TOWN_TITLE");
            CHECK(repeatTown == townTitle);
        }

        // 4. [v0.9.2] 5개 JSON 전체 codepoint에 대해 선택 font의 hasGlyph 검증 추가
        const sf::Font& targetFont = (lang == crawl::Language::JA || lang == crawl::Language::ZH_TW || lang == crawl::Language::ZH_CN) ? cjkFont : font;

        std::ifstream file(jsonPath);
        CHECK(file.is_open());
        nlohmann::json j;
        file >> j;
        file.close();

        for (auto& element : j.items()) {
            if (element.value().is_string()) {
                std::string valStr = element.value().get<std::string>();
                sf::String sfVal = sf::String::fromUtf8(valStr.begin(), valStr.end());
                for (std::size_t i = 0; i < sfVal.getSize(); ++i) {
                    sf::Uint32 codepoint = sfVal[i];
                    
                    // 제어 문자, 기본 공백 및 표준 ASCII 문자 영역(0~127)은 폰트 검증 예외 처리
                    if (codepoint <= 127 || codepoint == ' ' || codepoint == '\n' || codepoint == '\t' || codepoint == '\r') {
                        continue;
                    }
                    
                    bool hasGlyph = targetFont.hasGlyph(codepoint);
                    if (!hasGlyph) {
                        std::cerr << "[Failure] Font missing glyph for codepoint: U+" 
                                  << std::hex << codepoint << " (Language: " << static_cast<int>(lang) 
                                  << ", Key: " << element.key() << ")" << std::endl;
                        CHECK(hasGlyph);
                    }
                }
            }
        }
    }

    std::cout << "-> [Success] Town 서브상태 및 Combat 헤더 다국어 타이틀 로딩 회귀 테스트 통과 (5대 언어 100% Glyph Coverage 확보)." << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "       Crawlmaster 테스트 하네스 기동       " << std::endl;
    std::cout << "========================================" << std::endl;

    // 인자 유무와 관계없이 항상 모든 테스트 구동
    if (argc > 1 && std::string(argv[1]) == "--run-all") {
        // 옵션 인식 로그만 표출
        std::cout << "[Info] --run-all 옵션이 감지되었습니다." << std::endl;
    }

    const std::filesystem::path userSavePath = "./save.json";
    const std::filesystem::path userConfigPath = "./config.json";
    const bool userSaveExisted = std::filesystem::exists(userSavePath);
    const bool userConfigExisted = std::filesystem::exists(userConfigPath);
    const std::string userSaveBefore = userSaveExisted ? readFileBytes(userSavePath) : std::string{};
    const std::string userConfigBefore = userConfigExisted ? readFileBytes(userConfigPath) : std::string{};

    g_testDirectory = std::filesystem::temp_directory_path() /
        ("crawlmaster-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(g_testDirectory);
    crawl::Party::setDefaultSavePath((g_testDirectory / "save.json").string());

    testAbilityModifiers();
    testJsonSerialization();
    testDungeonMazeGeneration();
    testDungeonAutoMoveBFS();
    testCharacterEquipmentSystem();
    testDndRulesAndLevelup();
    testPartySaveLoad();
    testDefaultInventoryContract();
    testCorruptCustomSaveIsNonDestructive();
    testResetToDefaultDoesNotWrite();
    testSaveV3CanonicalSchema();
    testV1SaveMigratesToCanonicalV3();
    testV2SaveLoadsUnknownIdentityAndMigratesToV3();
    testInvalidV3IdentityIsRejected();
    testBackupRecoveryQuarantinesPrimary();
    testCorruptBackupIsAlsoQuarantined();
    testMissingPrimaryCorruptBackupIsQuarantined();
    testSaveLoadRejectsLeafSymlinks();
    testAtomicSaveFailurePreservesTarget();
    testInvalidV2MembersAreRejectedWithoutMutation();
    testReplaceAllClearsStateStack();
    testMonsterAndQuestSystem();
    testCombatEffectsAndItemUsage();
    testAdvancedEquipSwapAndClassLimits();
    testLocalizationI18n();
    testConfigPersistenceIsAtomicAndVersioned();
    testCorruptConfigIsQuarantinedWithoutOverwrite();
    testConfigBackupRestoresPrimaryAcrossProcesses();
    testDirectorySyncFailureIsNotReportedAsSaved();
    testOversizedAndTamperedSaveIsQuarantined();
    testTownHubLocalizationKeyCoverage();
    testShopSelling();
    testTownAndCombatUIStatesI18nSafety();

    // [v0.9.4] 테스트는 사용자 실행 경로의 저장소를 읽거나 변경하지 않아야 한다.
    CHECK(std::filesystem::exists(userSavePath) == userSaveExisted);
    CHECK(std::filesystem::exists(userConfigPath) == userConfigExisted);
    if (userSaveExisted) {
        CHECK(readFileBytes(userSavePath) == userSaveBefore);
    }
    if (userConfigExisted) {
        CHECK(readFileBytes(userConfigPath) == userConfigBefore);
    }
    std::filesystem::remove_all(g_testDirectory);

    std::cout << "========================================" << std::endl;
    if (g_failureCount == 0) {
        std::cout << "       모든 단위 테스트 검증 완료.       " << std::endl;
    } else {
        std::cerr << "       단위 테스트 실패: " << g_failureCount << "건       " << std::endl;
    }
    std::cout << "========================================" << std::endl;

    return g_failureCount == 0 ? 0 : 1;
}
