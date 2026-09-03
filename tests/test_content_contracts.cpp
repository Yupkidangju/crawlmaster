#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "model/DungeonMap.hpp"
#include "model/ItemFactory.hpp"
#include "model/MonsterFactory.hpp"
#include "model/Party.hpp"
#include "model/Quest.hpp"

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

} // namespace

int main() {
    testCanonicalQuestRegistry();
    testCompletedQuestCannotBeAcceptedOrRewardedTwice();
    testNonShopNonStarterItemsHaveAcquisitionSources();
    testEveryMonsterDropReferencesARegisteredItem();
    testSeededDungeonLandmarksAreReachableAndBossGateIsFarthest();
    testCampaignCompletionRoundTripsThroughSave();

    if (g_failureCount != 0) {
        std::cerr << "Content contract tests failed: " << g_failureCount << " check(s).\n";
        return 1;
    }

    std::cout << "Content contract tests passed.\n";
    return 0;
}
