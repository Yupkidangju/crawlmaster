#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "model/DungeonMap.hpp"
#include "model/DungeonWorld.hpp"
#include "model/ItemFactory.hpp"
#include "model/MonsterFactory.hpp"
#include "model/Party.hpp"
#include "model/Quest.hpp"
#include "core/SessionRng.hpp"

namespace {

int g_failureCount = 0;

void check(bool condition, const char* expression, const char* file, int line) {
    if (condition) {
        return;
    }

    ++g_failureCount;
    std::cerr << "[Failure] " << file << ':' << line
              << ": CHECK(" << expression << ")\n";
}

#define CHECK(condition) check(static_cast<bool>(condition), #condition, __FILE__, __LINE__)

class ScopedTestDirectory {
public:
    ScopedTestDirectory() {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() /
               ("crawlmaster-content-contracts-" + std::to_string(suffix));
        std::filesystem::create_directories(path);
    }

    ~ScopedTestDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }

    std::filesystem::path path;
};

void testCanonicalQuestRegistry() {
    const std::set<std::string> expectedIds = {
        "qst_clear_kobolds",
        "qst_collect_maces",
        "qst_hunt_spiders",
        "qst_recover_moon_seal",
        "qst_defeat_crypt_warden",
        "qst_find_missing_scout",
    };
    const auto registeredIds = crawl::Quest::getCanonicalIds();

    CHECK(std::set<std::string>(registeredIds.begin(), registeredIds.end()) == expectedIds);
    for (const auto& id : expectedIds) {
        const auto quest = crawl::Quest::createCanonical(id);
        CHECK(quest != nullptr);
        if (quest) {
            CHECK(quest->getId() == id);
        }
    }
    CHECK(crawl::Quest::createCanonical("qst_unknown") == nullptr);
    const std::vector<std::string> expectedOfferable = {
        "qst_recover_moon_seal", "qst_defeat_crypt_warden", "qst_find_missing_scout",
    };
    CHECK(crawl::Quest::getOfferableIds() == expectedOfferable);
}

void testObjectiveQuestRequiresFieldCompletionBeforeReport() {
    crawl::SessionRng::reseedGlobal(10101U);
    crawl::Party party;
    party.getWorld().generate(crawl::SessionRng::global().seed());
    auto quest = crawl::Quest::createCanonical("qst_recover_moon_seal");
    CHECK(quest != nullptr);
    if (!quest) return;
    CHECK(quest->getType() == crawl::QuestType::RETRIEVE_KEY_ITEM);
    CHECK(quest->getTargetFloor() == 1);
    party.acceptQuest(quest);
    const int initialGold = party.getGold();
    party.completeQuest(quest->getId());
    CHECK(party.hasQuest(quest->getId()));
    CHECK(party.getGold() == initialGold);

    CHECK(party.markQuestObjectiveComplete(quest->getId()));
    CHECK(quest->isReadyToReport());
    CHECK(party.addKeyItem("key_moon_seal"));
    party.abandonQuest(quest->getId());
    CHECK(party.hasQuest(quest->getId()));
    party.getWorld().findObject("obj_moon_seal")->state = crawl::WorldObjectState::RESOLVED;
    party.completeQuest(quest->getId());
    CHECK(party.isQuestCompleted(quest->getId()));
    CHECK(!party.hasKeyItem("key_moon_seal"));
    CHECK(party.getGold() == initialGold + 75);
}

void testCompletedQuestCannotBeAcceptedOrRewardedTwice() {
    crawl::Party party;
    const auto quest = crawl::Quest::createCanonical("qst_clear_kobolds");
    CHECK(quest != nullptr);
    if (!quest) {
        return;
    }

    const int initialGold = party.getGold();
    const int expectedGold = initialGold + quest->getGoldReward();

    party.acceptQuest(quest);
    party.updateQuestKillProgress(quest->getTargetId(), quest->getTargetCount());
    party.completeQuest(quest->getId());

    CHECK(party.isQuestCompleted(quest->getId()));
    CHECK(!party.hasQuest(quest->getId()));
    CHECK(party.getGold() == expectedGold);

    party.acceptQuest(crawl::Quest::createCanonical(quest->getId()));
    CHECK(!party.hasQuest(quest->getId()));
    party.completeQuest(quest->getId());
    CHECK(party.getGold() == expectedGold);
}

void testNonShopNonStarterItemsHaveAcquisitionSources() {
    const std::set<std::string> shopIds = {
        "wpn_dagger", "wpn_longsword", "wpn_mace", "arm_leather",
        "arm_scale", "arm_chain", "shd_round", "pot_heal",
    };
    const std::set<std::string> starterOnlyIds = {
        "wpn_staff",
        "arm_robe",
    };

    const auto registeredItemIds = crawl::ItemFactory::getRegisteredIds();
    const std::set<std::string> allItemIds(registeredItemIds.begin(), registeredItemIds.end());
    CHECK(registeredItemIds.size() == allItemIds.size());
    CHECK(allItemIds.size() == 19);
    const auto catalog = crawl::ItemFactory::getShopCatalog();
    CHECK(catalog.size() == shopIds.size());
    for (const auto& item : catalog) CHECK(item && shopIds.contains(item->getId()));

    std::set<std::string> lootOrQuestRewardIds;
    for (const auto& monsterId : crawl::MonsterFactory::getRegisteredIds()) {
        const auto dropIds = crawl::MonsterFactory::getDropItemIds(monsterId);
        lootOrQuestRewardIds.insert(dropIds.begin(), dropIds.end());
    }
    for (const auto& questId : crawl::Quest::getCanonicalIds()) {
        const auto quest = crawl::Quest::createCanonical(questId);
        CHECK(quest != nullptr);
        if (!quest) {
            continue;
        }
        const auto rewardIds = quest->getRewardItemIds();
        lootOrQuestRewardIds.insert(rewardIds.begin(), rewardIds.end());
    }

    std::size_t advancedItemCount = 0;
    for (const auto& itemId : allItemIds) {
        CHECK(crawl::ItemFactory::createItem(itemId) != nullptr);
        if (!shopIds.contains(itemId) && !starterOnlyIds.contains(itemId)) {
            ++advancedItemCount;
            CHECK(lootOrQuestRewardIds.contains(itemId));
        }
    }
    CHECK(advancedItemCount == 9);
}

void testEveryMonsterDropReferencesARegisteredItem() {
    const auto registeredMonsterIds = crawl::MonsterFactory::getRegisteredIds();
    CHECK(!registeredMonsterIds.empty());

    for (const auto& monsterId : registeredMonsterIds) {
        CHECK(crawl::MonsterFactory::createMonster(monsterId) != nullptr);
        for (const auto& dropId : crawl::MonsterFactory::getDropItemIds(monsterId)) {
            CHECK(crawl::ItemFactory::createItem(dropId) != nullptr);
        }
    }
}

void testSeededDungeonLandmarksAreReachableAndBossGateIsFarthest() {
    constexpr std::uint32_t seed = 0xC0FFEEu;
    crawl::DungeonMap map;
    map.generate(seed);

    const int startX = map.getPlayerX();
    const int startY = map.getPlayerY();
    std::array<std::array<int, crawl::DungeonMap::MAP_HEIGHT>, crawl::DungeonMap::MAP_WIDTH>
        distance{};
    for (auto& column : distance) {
        column.fill(-1);
    }

    std::queue<std::pair<int, int>> pending;
    pending.push({startX, startY});
    distance[startX][startY] = 0;
    int farthestDistance = 0;

    constexpr std::array<std::pair<int, int>, 4> directions = {
        std::pair{0, -1}, std::pair{1, 0}, std::pair{0, 1}, std::pair{-1, 0},
    };
    while (!pending.empty()) {
        const auto [x, y] = pending.front();
        pending.pop();
        farthestDistance = std::max(farthestDistance, distance[x][y]);

        for (const auto& [dx, dy] : directions) {
            const int nextX = x + dx;
            const int nextY = y + dy;
            if (nextX < 0 || nextX >= crawl::DungeonMap::MAP_WIDTH ||
                nextY < 0 || nextY >= crawl::DungeonMap::MAP_HEIGHT ||
                distance[nextX][nextY] >= 0 || !map.isWalkable(nextX, nextY)) {
                continue;
            }
            distance[nextX][nextY] = distance[x][y] + 1;
            pending.push({nextX, nextY});
        }
    }

    int doorCount = 0;
    int bossGateCount = 0;
    int doorDistance = -1;
    int bossGateDistance = -1;
    for (int x = 0; x < crawl::DungeonMap::MAP_WIDTH; ++x) {
        for (int y = 0; y < crawl::DungeonMap::MAP_HEIGHT; ++y) {
            if (map.getTile(x, y) == crawl::TileType::DOOR) {
                ++doorCount;
                doorDistance = distance[x][y];
            } else if (map.getTile(x, y) == crawl::TileType::BOSS_GATE) {
                ++bossGateCount;
                bossGateDistance = distance[x][y];
            }
        }
    }

    CHECK(doorCount == 1);
    CHECK(bossGateCount == 1);
    CHECK(doorDistance > 0);
    CHECK(bossGateDistance > doorDistance);
    CHECK(bossGateDistance == farthestDistance);
}

void testCampaignCompletionRoundTripsThroughSave() {
    ScopedTestDirectory directory;
    const auto savePath = directory.path / "campaign.json";

    crawl::Party savedParty;
    savedParty.setCampaignCompleted(true);
    CHECK(savedParty.saveToFile(savePath.string()).succeeded());

    crawl::Party loadedParty;
    CHECK(!loadedParty.isCampaignCompleted());
    CHECK(loadedParty.loadFromFile(savePath.string()).succeeded());
    CHECK(loadedParty.isCampaignCompleted());
}

void testPersistentWorldHasThreeReachableFloorsAndRoundTrips() {
    constexpr std::uint32_t seed = 0x1234ABCDu;
    crawl::DungeonWorld world;
    world.generate(seed);

    CHECK(world.isGenerated());
    CHECK(world.getSeed() == seed);
    CHECK(world.getFloorCount() == 3);
    CHECK(world.getFloor(1).getTile(1, 1) == crawl::TileType::UPSTAIRS);
    CHECK(world.getFloor(2).getTile(1, 1) == crawl::TileType::UPSTAIRS);
    CHECK(world.getFloor(3).getTile(1, 1) == crawl::TileType::UPSTAIRS);

    int downStairs = 0;
    int finalGates = 0;
    for (int floor = 1; floor <= world.getFloorCount(); ++floor) {
        for (int x = 0; x < crawl::DungeonMap::MAP_WIDTH; ++x) {
            for (int y = 0; y < crawl::DungeonMap::MAP_HEIGHT; ++y) {
                if (world.getFloor(floor).getTile(x, y) == crawl::TileType::DOWNSTAIRS) ++downStairs;
                if (world.getFloor(floor).getTile(x, y) == crawl::TileType::BOSS_GATE) ++finalGates;
            }
        }
    }
    CHECK(downStairs == 2);
    CHECK(finalGates == 1);
    CHECK(world.getObjects().size() == 3);

    auto& floorOne = world.getFloor(1);
    int visitedX = 1;
    int visitedY = 1;
    for (int x = 1; x < crawl::DungeonMap::MAP_WIDTH - 1; ++x) {
        for (int y = 1; y < crawl::DungeonMap::MAP_HEIGHT - 1; ++y) {
            if (floorOne.isWalkable(x, y)) {
                visitedX = x;
                visitedY = y;
            }
        }
    }
    floorOne.setVisited(visitedX, visitedY, true);
    floorOne.setStepped(visitedX, visitedY, true);
    auto* item = world.findObject("obj_moon_seal");
    CHECK(item != nullptr);
    if (item) {
        floorOne.setVisited(item->x, item->y, true);
        item->state = crawl::WorldObjectState::DISCOVERED;
    }

    const auto serialized = world.toJson();
    auto restored = crawl::DungeonWorld::fromJson(serialized);
    CHECK(restored.getSeed() == seed);
    CHECK(restored.getFloor(1).isVisited(visitedX, visitedY));
    CHECK(restored.getFloor(1).isStepped(visitedX, visitedY));
    const auto* restoredItem = restored.findObject("obj_moon_seal");
    CHECK(restoredItem != nullptr);
    if (restoredItem) CHECK(restoredItem->state == crawl::WorldObjectState::DISCOVERED);
    CHECK(restored.toJson() == serialized);
}

void testSchemaV3DeterministicallyMigratesToPersistentWorld() {
    ScopedTestDirectory directory;
    const auto savePath = directory.path / "legacy-v3.json";
    crawl::SessionRng::reseedGlobal(0x13572468U);
    crawl::Party source;
    CHECK(source.startNewGame(savePath.string()));

    nlohmann::json legacy;
    {
        std::ifstream input(savePath);
        input >> legacy;
    }
    legacy["schemaVersion"] = 3;
    legacy.erase("world");
    legacy.erase("keyItems");
    {
        std::ofstream output(savePath, std::ios::trunc);
        output << legacy.dump(4);
    }

    crawl::Party firstLoad;
    crawl::Party secondLoad;
    CHECK(firstLoad.loadFromFile(savePath.string()));
    CHECK(secondLoad.loadFromFile(savePath.string()));
    CHECK(firstLoad.getWorld().getSeed() == 0x13572468U);
    CHECK(firstLoad.getWorld().toJson() == secondLoad.getWorld().toJson());
    CHECK(firstLoad.saveToFile(savePath.string()));
    nlohmann::json migrated;
    {
        std::ifstream input(savePath);
        input >> migrated;
    }
    CHECK(migrated.at("schemaVersion") == 4);
    CHECK(migrated.contains("world"));
    CHECK(migrated.contains("keyItems"));
}

void testNewGameIsTheOnlyWorldResetBoundary() {
    ScopedTestDirectory directory;
    const auto savePath = directory.path / "new-game.json";
    crawl::SessionRng::reseedGlobal(1111U);
    crawl::Party party;
    CHECK(party.startNewGame(savePath.string()));
    const auto firstWorld = party.getWorld().toJson();
    party.acceptQuest(crawl::Quest::createCanonical("qst_recover_moon_seal"));
    CHECK(party.markQuestObjectiveComplete("qst_recover_moon_seal"));
    CHECK(party.addKeyItem("key_moon_seal"));
    party.getWorld().findObject("obj_moon_seal")->state = crawl::WorldObjectState::RESOLVED;
    CHECK(party.saveToFile(savePath.string()));

    crawl::SessionRng::reseedGlobal(2222U);
    CHECK(party.startNewGame(savePath.string()));
    CHECK(party.getWorld().getSeed() == 2222U);
    CHECK(party.getWorld().toJson() != firstWorld);
    CHECK(party.getKeyItems().empty());
    CHECK(party.getActiveQuests().empty());
    CHECK(party.getCompletedQuestIds().empty());
    CHECK(party.getWorld().findObject("obj_moon_seal")->state == crawl::WorldObjectState::PRESENT);
}

void testMalformedWorldSnapshotIsRejected() {
    crawl::DungeonWorld world;
    world.generate(0xCAFEBABEu);
    auto malformed = world.toJson();
    malformed["floors"][0]["tiles"][0] = "....................";
    bool rejected = false;
    try {
        static_cast<void>(crawl::DungeonWorld::fromJson(malformed));
    } catch (const std::exception&) {
        rejected = true;
    }
    CHECK(rejected);
}

void testQuestProgressClampsNegativeAndOverflow() {
    auto quest = crawl::Quest::createCanonical("qst_clear_kobolds");
    CHECK(quest != nullptr);
    if (!quest) return;
    quest->setCurrentCount(-10);
    CHECK(quest->getCurrentCount() == 0);
    quest->updateProgress("mon_kobold", -5);
    CHECK(quest->getCurrentCount() == 0);
    quest->updateProgress("mon_kobold", std::numeric_limits<int>::max());
    CHECK(quest->getCurrentCount() == quest->getTargetCount());
}

void testPartyRejectsNullAndNonCanonicalQuestDefinitions() {
    crawl::Party party;
    CHECK(!party.addMember(nullptr));
    CHECK(party.getMemberCount() == 0);
    auto custom = std::make_shared<crawl::Quest>(
        "qst_custom", "CUSTOM_NAME", "CUSTOM_DESC", crawl::QuestType::FIND_NPC,
        "npc_custom", 1, 1, 1, std::vector<std::string>{}, 1);
    party.acceptQuest(custom);
    CHECK(!party.hasQuest("qst_custom"));

    auto tampered = std::make_shared<crawl::Quest>(
        "qst_find_missing_scout", "QUEST_QST_FIND_MISSING_SCOUT_NAME",
        "QUEST_QST_FIND_MISSING_SCOUT_DESC", crawl::QuestType::FIND_NPC,
        "npc_missing_scout", 1, 9999, 400,
        std::vector<std::string>{"scr_cure", "pot_greater_heal"}, 3);
    party.acceptQuest(tampered);
    CHECK(!party.hasQuest("qst_find_missing_scout"));
    auto renamed = std::make_shared<crawl::Quest>(
        "qst_find_missing_scout", "CUSTOM_NAME", "QUEST_QST_FIND_MISSING_SCOUT_DESC",
        crawl::QuestType::FIND_NPC, "npc_missing_scout", 1, 200, 400,
        std::vector<std::string>{"scr_cure", "pot_greater_heal"}, 3);
    party.acceptQuest(renamed);
    CHECK(!party.hasQuest("qst_find_missing_scout"));
}

void testObjectiveReportRequiresResolvedWorldObjectAndUpdatesRetainedPointer() {
    ScopedTestDirectory directory;
    crawl::SessionRng::reseedGlobal(3333U);
    crawl::Party party;
    CHECK(party.startNewGame((directory.path / "report-guard.json").string()));
    auto quest = crawl::Quest::createCanonical("qst_find_missing_scout");
    party.acceptQuest(quest);
    CHECK(party.markQuestObjectiveComplete(quest->getId()));
    party.completeQuest(quest->getId());
    CHECK(party.hasQuest(quest->getId()));
    CHECK(!quest->isCompleted());
    party.getWorld().findObject("obj_missing_scout")->state = crawl::WorldObjectState::RESOLVED;
    party.completeQuest(quest->getId());
    CHECK(!party.hasQuest(quest->getId()));
    CHECK(party.isQuestCompleted(quest->getId()));
    CHECK(quest->isCompleted());
}

void testCompletedQuestIdsAreWrittenInCanonicalOrder() {
    ScopedTestDirectory directory;
    const auto savePath = directory.path / "ordered.json";
    crawl::SessionRng::reseedGlobal(4444U);
    crawl::Party source;
    CHECK(source.startNewGame(savePath.string()));
    nlohmann::json json;
    {
        std::ifstream input(savePath);
        input >> json;
    }
    json["completedQuestIds"] = nlohmann::json::array({
        "qst_find_missing_scout", "qst_defeat_crypt_warden", "qst_recover_moon_seal"});
    for (auto& object : json["world"]["objects"]) object["state"] = "resolved";
    {
        std::ofstream output(savePath, std::ios::trunc);
        output << json.dump(4);
    }
    crawl::Party loaded;
    CHECK(loaded.loadFromFile(savePath.string()));
    CHECK(loaded.saveToFile(savePath.string()));
    {
        std::ifstream input(savePath);
        input >> json;
    }
    CHECK(json["completedQuestIds"] == nlohmann::json::array({
        "qst_defeat_crypt_warden", "qst_find_missing_scout", "qst_recover_moon_seal"}));
}

void testMutableWorldIsValidatedBeforeSave() {
    ScopedTestDirectory directory;
    crawl::SessionRng::reseedGlobal(5555U);
    crawl::Party party;
    CHECK(party.startNewGame((directory.path / "world.json").string()));
    auto* object = party.getWorld().findObject("obj_moon_seal");
    CHECK(object != nullptr);
    if (!object) return;
    object->x = 1;
    object->y = 1;
    CHECK(!party.saveToFile((directory.path / "world.json").string()));
}

void testSeedlessLegacyMigrationIgnoresProcessEntropy() {
    ScopedTestDirectory directory;
    nlohmann::json legacy;
    {
        std::ifstream input(std::filesystem::path(__FILE__).parent_path() / "fixtures/save_v1.json");
        input >> legacy;
    }
    const auto firstPath = directory.path / "legacy-a.json";
    const auto secondPath = directory.path / "legacy-b.json";
    {
        std::ofstream output(firstPath);
        output << legacy.dump(4);
    }
    {
        std::ofstream output(secondPath);
        output << legacy.dump(4);
    }
    crawl::SessionRng::reseedGlobal(1U);
    crawl::Party first;
    CHECK(first.loadFromFile(firstPath.string()));
    const auto firstWorld = first.getWorld().toJson();
    const auto derivedSeed = first.getLastSessionSeed();
    CHECK(derivedSeed != 0U);
    CHECK(crawl::SessionRng::global().seed() == derivedSeed);
    CHECK(crawl::SessionRng::global().drawCount() == 0U);

    crawl::SessionRng::reseedGlobal(999999U);
    crawl::Party second;
    CHECK(second.loadFromFile(secondPath.string()));
    CHECK(second.getLastSessionSeed() == derivedSeed);
    CHECK(second.getWorld().toJson() == firstWorld);
    CHECK(crawl::SessionRng::global().seed() == derivedSeed);
}

void testV4RootAndQuestFieldsAreRequired() {
    ScopedTestDirectory directory;
    const auto basePath = directory.path / "base.json";
    crawl::SessionRng::reseedGlobal(6666U);
    crawl::Party source;
    CHECK(source.startNewGame(basePath.string()));
    source.acceptQuest(crawl::Quest::createCanonical("qst_find_missing_scout"));
    CHECK(source.saveToFile(basePath.string()));
    nlohmann::json base;
    {
        std::ifstream input(basePath);
        input >> base;
    }
    const std::vector<std::string> rootKeys = {
        "schemaVersion", "gold", "inventory", "keyItems", "members", "activeQuests", "completedQuestIds",
        "campaignCompleted", "lastSessionSeed", "sessionRngDrawCount", "world"};
    for (const auto& key : rootKeys) {
        auto malformed = base;
        malformed.erase(key);
        const auto path = directory.path / ("missing-root-" + key + ".json");
        std::ofstream(path) << malformed.dump(4);
        crawl::Party target;
        CHECK(target.loadFromFile(path.string()).status == crawl::PersistenceStatus::Corrupt);
    }
    const std::vector<std::string> questKeys = {
        "id", "name", "desc", "type", "targetId", "targetCount", "currentCount",
        "goldReward", "xpReward", "isCompleted", "rewardItemIds", "readyToReport", "targetFloor"};
    for (const auto& key : questKeys) {
        auto malformed = base;
        malformed["activeQuests"][0].erase(key);
        const auto path = directory.path / ("missing-quest-" + key + ".json");
        std::ofstream(path) << malformed.dump(4);
        crawl::Party target;
        CHECK(target.loadFromFile(path.string()).status == crawl::PersistenceStatus::Corrupt);
    }
}

void testV4QuestKeyAndLedgerCombinationsAreStrict() {
    ScopedTestDirectory directory;
    const auto basePath = directory.path / "base.json";
    crawl::SessionRng::reseedGlobal(7777U);
    crawl::Party source;
    CHECK(source.startNewGame(basePath.string()));
    source.acceptQuest(crawl::Quest::createCanonical("qst_recover_moon_seal"));
    CHECK(source.saveToFile(basePath.string()));
    nlohmann::json base;
    {
        std::ifstream input(basePath);
        input >> base;
    }

    auto orphanKey = base;
    orphanKey["keyItems"] = nlohmann::json::array({"key_moon_seal"});
    const auto orphanPath = directory.path / "orphan-key.json";
    std::ofstream(orphanPath) << orphanKey.dump(4);
    crawl::Party orphanTarget;
    CHECK(orphanTarget.loadFromFile(orphanPath.string()).status == crawl::PersistenceStatus::Corrupt);

    auto duplicateLedger = base;
    duplicateLedger["activeQuests"] = nlohmann::json::array();
    duplicateLedger["completedQuestIds"] = nlohmann::json::array({"qst_clear_kobolds", "qst_clear_kobolds"});
    const auto duplicatePath = directory.path / "duplicate-ledger.json";
    std::ofstream(duplicatePath) << duplicateLedger.dump(4);
    crawl::Party duplicateTarget;
    CHECK(duplicateTarget.loadFromFile(duplicatePath.string()).status == crawl::PersistenceStatus::Corrupt);

    auto unknownLedger = base;
    unknownLedger["activeQuests"] = nlohmann::json::array();
    unknownLedger["completedQuestIds"] = nlohmann::json::array({"qst_unknown"});
    const auto unknownPath = directory.path / "unknown-ledger.json";
    std::ofstream(unknownPath) << unknownLedger.dump(4);
    crawl::Party unknownTarget;
    CHECK(unknownTarget.loadFromFile(unknownPath.string()).status == crawl::PersistenceStatus::Corrupt);
}

void testMapLandmarkAndDiscoverySemanticsAreStrict() {
    crawl::DungeonWorld world;
    world.generate(8888U);
    auto base = world.toJson();

    auto noDoor = base;
    auto& row = noDoor["floors"][0]["tiles"];
    bool replacedDoor = false;
    for (auto& encoded : row) {
        std::string value = encoded.get<std::string>();
        const auto position = value.find('D');
        if (position != std::string::npos) {
            value[position] = '.';
            encoded = value;
            replacedDoor = true;
            break;
        }
    }
    CHECK(replacedDoor);
    bool rejectedDoor = false;
    try { static_cast<void>(crawl::DungeonWorld::fromJson(noDoor)); }
    catch (const std::exception&) { rejectedDoor = true; }
    CHECK(rejectedDoor);

    auto duplicateDoor = base;
    bool addedDoor = false;
    for (auto& encoded : duplicateDoor["floors"][0]["tiles"]) {
        std::string value = encoded.get<std::string>();
        const auto position = value.find('.');
        if (position != std::string::npos) {
            value[position] = 'D';
            encoded = value;
            addedDoor = true;
            break;
        }
    }
    CHECK(addedDoor);
    bool rejectedDuplicateDoor = false;
    try { static_cast<void>(crawl::DungeonWorld::fromJson(duplicateDoor)); }
    catch (const std::exception&) { rejectedDuplicateDoor = true; }
    CHECK(rejectedDuplicateDoor);

    auto badEntry = base;
    badEntry["floors"][0]["visited"][1] = "00000000000000000000";
    badEntry["floors"][0]["stepped"][1] = "00000000000000000000";
    bool rejectedEntry = false;
    try { static_cast<void>(crawl::DungeonWorld::fromJson(badEntry)); }
    catch (const std::exception&) { rejectedEntry = true; }
    CHECK(rejectedEntry);

    auto hiddenDiscovery = base;
    hiddenDiscovery["objects"][0]["state"] = "discovered";
    const int objectX = hiddenDiscovery["objects"][0]["x"].get<int>();
    const int objectY = hiddenDiscovery["objects"][0]["y"].get<int>();
    std::string visitedRow = hiddenDiscovery["floors"][0]["visited"][objectY].get<std::string>();
    visitedRow[static_cast<std::size_t>(objectX)] = '0';
    hiddenDiscovery["floors"][0]["visited"][objectY] = visitedRow;
    bool rejectedDiscovery = false;
    try { static_cast<void>(crawl::DungeonWorld::fromJson(hiddenDiscovery)); }
    catch (const std::exception&) { rejectedDiscovery = true; }
    CHECK(rejectedDiscovery);

    auto earlyGate = base;
    int gateX = -1;
    int gateY = -1;
    int nearX = -1;
    int nearY = -1;
    const auto& floorThree = world.getFloor(3);
    for (int x = 1; x < crawl::DungeonMap::MAP_WIDTH - 1; ++x) {
        for (int y = 1; y < crawl::DungeonMap::MAP_HEIGHT - 1; ++y) {
            if (floorThree.getTile(x, y) == crawl::TileType::BOSS_GATE) {
                gateX = x; gateY = y;
            } else if (floorThree.getTile(x, y) == crawl::TileType::EMPTY &&
                       floorThree.getDistanceFromStart(x, y) > 0 &&
                       floorThree.getDistanceFromStart(x, y) < 5 &&
                       world.findObjectAt(3, x, y) == nullptr) {
                nearX = x; nearY = y;
            }
        }
    }
    CHECK(gateX >= 0 && nearX >= 0);
    if (gateX >= 0 && nearX >= 0) {
        std::string gateRow = earlyGate["floors"][2]["tiles"][gateY].get<std::string>();
        std::string nearRow = earlyGate["floors"][2]["tiles"][nearY].get<std::string>();
        gateRow[static_cast<std::size_t>(gateX)] = '.';
        nearRow[static_cast<std::size_t>(nearX)] = 'B';
        earlyGate["floors"][2]["tiles"][gateY] = gateRow;
        earlyGate["floors"][2]["tiles"][nearY] = nearRow;
        bool rejectedEarlyGate = false;
        try { static_cast<void>(crawl::DungeonWorld::fromJson(earlyGate)); }
        catch (const std::exception&) { rejectedEarlyGate = true; }
        CHECK(rejectedEarlyGate);
    }
}

void testFailedLoadPreservesMemorySessionAndGlobalRng() {
    ScopedTestDirectory directory;
    const auto validPath = directory.path / "valid.json";
    const auto corruptPath = directory.path / "corrupt.json";
    crawl::SessionRng::reseedGlobal(9999U);
    crawl::Party party;
    CHECK(party.startNewGame(validPath.string()));
    auto member = std::make_shared<crawl::Character>("Stable", crawl::CharacterClass::WARRIOR);
    CHECK(party.addMember(member));
    party.addGold(44);
    const auto checkpointSeed = crawl::SessionRng::global().seed();
    const auto checkpointDraws = crawl::SessionRng::global().drawCount();
    std::ofstream(corruptPath) << "{broken";
    CHECK(party.loadFromFile(corruptPath.string()).status == crawl::PersistenceStatus::Corrupt);
    CHECK(party.getGold() == 144);
    CHECK(party.hasActiveSaveSession());
    CHECK(party.getMember(0) == member);
    CHECK(crawl::SessionRng::global().seed() == checkpointSeed);
    CHECK(crawl::SessionRng::global().drawCount() == checkpointDraws);
}

void testLoadRestoresGlobalRngAndSaveBytesAreIdempotent() {
    ScopedTestDirectory directory;
    const auto savePath = directory.path / "idempotent.json";
    crawl::SessionRng::reseedGlobal(0xAABBCCDDU);
    crawl::Party source;
    CHECK(source.startNewGame(savePath.string()));
    static_cast<void>(crawl::SessionRng::global().rollRange(1, 100));
    CHECK(source.saveToFile(savePath.string()));
    std::ifstream firstInput(savePath, std::ios::binary);
    const std::string firstBytes{std::istreambuf_iterator<char>(firstInput), {}};
    const auto savedSeed = source.getLastSessionSeed();
    const auto savedDraws = source.getSessionRngDrawCount();
    crawl::SessionRng expected(savedSeed, savedDraws);
    const int expectedNext = expected.rollRange(1, 1000);

    crawl::SessionRng::reseedGlobal(42U);
    crawl::Party loaded;
    CHECK(loaded.loadFromFile(savePath.string()));
    CHECK(crawl::SessionRng::global().seed() == savedSeed);
    CHECK(crawl::SessionRng::global().drawCount() == savedDraws);
    CHECK(crawl::SessionRng::global().rollRange(1, 1000) == expectedNext);
    crawl::SessionRng::global() = crawl::SessionRng(savedSeed, savedDraws);
    CHECK(loaded.saveToFile(savePath.string()));
    std::ifstream secondInput(savePath, std::ios::binary);
    const std::string secondBytes{std::istreambuf_iterator<char>(secondInput), {}};
    CHECK(secondBytes == firstBytes);
}

} // namespace

int main() {
    testCanonicalQuestRegistry();
    testObjectiveQuestRequiresFieldCompletionBeforeReport();
    testCompletedQuestCannotBeAcceptedOrRewardedTwice();
    testNonShopNonStarterItemsHaveAcquisitionSources();
    testEveryMonsterDropReferencesARegisteredItem();
    testSeededDungeonLandmarksAreReachableAndBossGateIsFarthest();
    testCampaignCompletionRoundTripsThroughSave();
    testPersistentWorldHasThreeReachableFloorsAndRoundTrips();
    testSchemaV3DeterministicallyMigratesToPersistentWorld();
    testNewGameIsTheOnlyWorldResetBoundary();
    testMalformedWorldSnapshotIsRejected();
    testQuestProgressClampsNegativeAndOverflow();
    testPartyRejectsNullAndNonCanonicalQuestDefinitions();
    testObjectiveReportRequiresResolvedWorldObjectAndUpdatesRetainedPointer();
    testCompletedQuestIdsAreWrittenInCanonicalOrder();
    testMutableWorldIsValidatedBeforeSave();
    testSeedlessLegacyMigrationIgnoresProcessEntropy();
    testV4RootAndQuestFieldsAreRequired();
    testV4QuestKeyAndLedgerCombinationsAreStrict();
    testMapLandmarkAndDiscoverySemanticsAreStrict();
    testFailedLoadPreservesMemorySessionAndGlobalRng();
    testLoadRestoresGlobalRngAndSaveBytesAreIdempotent();

    if (g_failureCount != 0) {
        std::cerr << "Content contract tests failed: " << g_failureCount << " check(s).\n";
        return 1;
    }

    std::cout << "Content contract tests passed.\n";
    return 0;
}
