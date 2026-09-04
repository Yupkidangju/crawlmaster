#ifndef DUNGEON_WORLD_HPP
#define DUNGEON_WORLD_HPP

#include "model/DungeonMap.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace crawl {

enum class WorldObjectKind { QUEST_ITEM, NPC, QUEST_BOSS };
enum class WorldObjectState { PRESENT, DISCOVERED, RESOLVED };

struct WorldObject {
    std::string id;
    std::string questId;
    std::string targetId;
    WorldObjectKind kind = WorldObjectKind::QUEST_ITEM;
    WorldObjectState state = WorldObjectState::PRESENT;
    int floor = 1;
    int x = 1;
    int y = 1;
};

class DungeonWorld {
public:
    static constexpr int FLOOR_COUNT = 3;

    void generate(std::uint32_t seed);
    [[nodiscard]] bool isGenerated() const;
    [[nodiscard]] std::uint32_t getSeed() const;
    [[nodiscard]] int getFloorCount() const;
    DungeonMap& getFloor(int floorNumber);
    const DungeonMap& getFloor(int floorNumber) const;
    std::vector<WorldObject>& getObjects();
    const std::vector<WorldObject>& getObjects() const;
    WorldObject* findObject(const std::string& id);
    const WorldObject* findObject(const std::string& id) const;
    WorldObject* findObjectAt(int floorNumber, int x, int y);
    const WorldObject* findObjectAt(int floorNumber, int x, int y) const;

    nlohmann::json toJson() const;
    static DungeonWorld fromJson(const nlohmann::json& json);
    void validate() const;

private:
    std::uint32_t m_seed = 0;
    bool m_generated = false;
    std::vector<DungeonMap> m_floors;
    std::vector<WorldObject> m_objects;
};

} // namespace crawl

#endif
