#include "model/DungeonWorld.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace crawl {
namespace {

std::uint32_t splitmix32(std::uint32_t value) {
    value += 0x9E3779B9u;
    value = (value ^ (value >> 16U)) * 0x85EBCA6Bu;
    value = (value ^ (value >> 13U)) * 0xC2B2AE35u;
    return value ^ (value >> 16U);
}

std::pair<int, int> chooseObjectiveTile(const DungeonMap& map) {
    std::pair<int, int> selected{1, 1};
    int bestDistance = -1;
    for (int x = 1; x < DungeonMap::MAP_WIDTH - 1; ++x) {
        for (int y = 1; y < DungeonMap::MAP_HEIGHT - 1; ++y) {
            if (map.getTile(x, y) != TileType::EMPTY) continue;
            const int distance = map.getDistanceFromStart(x, y);
            if (distance > bestDistance) {
                bestDistance = distance;
                selected = {x, y};
            }
        }
    }
    if (bestDistance < 0) throw std::runtime_error("퀘스트 목표 타일을 배치할 수 없습니다.");
    return selected;
}

std::string kindToString(WorldObjectKind kind) {
    switch (kind) {
        case WorldObjectKind::QUEST_ITEM: return "quest_item";
        case WorldObjectKind::NPC: return "npc";
        case WorldObjectKind::QUEST_BOSS: return "quest_boss";
    }
    throw std::runtime_error("알 수 없는 월드 오브젝트 종류입니다.");
}

WorldObjectKind kindFromString(const std::string& value) {
    if (value == "quest_item") return WorldObjectKind::QUEST_ITEM;
    if (value == "npc") return WorldObjectKind::NPC;
    if (value == "quest_boss") return WorldObjectKind::QUEST_BOSS;
    throw std::runtime_error("알 수 없는 월드 오브젝트 종류입니다.");
}

std::string stateToString(WorldObjectState state) {
    switch (state) {
        case WorldObjectState::PRESENT: return "present";
        case WorldObjectState::DISCOVERED: return "discovered";
        case WorldObjectState::RESOLVED: return "resolved";
    }
    throw std::runtime_error("알 수 없는 월드 오브젝트 상태입니다.");
}

WorldObjectState stateFromString(const std::string& value) {
    if (value == "present") return WorldObjectState::PRESENT;
    if (value == "discovered") return WorldObjectState::DISCOVERED;
    if (value == "resolved") return WorldObjectState::RESOLVED;
    throw std::runtime_error("알 수 없는 월드 오브젝트 상태입니다.");
}

const std::array<WorldObject, 3>& canonicalObjects() {
    static const std::array<WorldObject, 3> objects = {{
        {"obj_moon_seal", "qst_recover_moon_seal", "key_moon_seal",
         WorldObjectKind::QUEST_ITEM, WorldObjectState::PRESENT, 1, 1, 1},
        {"obj_crypt_warden", "qst_defeat_crypt_warden", "mon_crypt_warden",
         WorldObjectKind::QUEST_BOSS, WorldObjectState::PRESENT, 2, 1, 1},
        {"obj_missing_scout", "qst_find_missing_scout", "npc_missing_scout",
         WorldObjectKind::NPC, WorldObjectState::PRESENT, 3, 1, 1},
    }};
    return objects;
}

} // namespace

void DungeonWorld::generate(std::uint32_t seed) {
    if (seed == 0U) throw std::invalid_argument("월드 seed는 0일 수 없습니다.");
    m_seed = seed;
    m_floors.clear();
    m_objects.clear();
    m_floors.reserve(FLOOR_COUNT);
    for (int floor = 1; floor <= FLOOR_COUNT; ++floor) {
        DungeonMap map;
        const std::uint32_t floorSeed = splitmix32(seed + 0x9E3779B9u * static_cast<std::uint32_t>(floor));
        map.generate(floorSeed, floor);
        m_floors.push_back(std::move(map));
    }
    for (const auto& definition : canonicalObjects()) {
        WorldObject object = definition;
        const auto [x, y] = chooseObjectiveTile(getFloor(object.floor));
        object.x = x;
        object.y = y;
        m_objects.push_back(std::move(object));
    }
    m_generated = true;
}

bool DungeonWorld::isGenerated() const { return m_generated; }
std::uint32_t DungeonWorld::getSeed() const { return m_seed; }
int DungeonWorld::getFloorCount() const { return static_cast<int>(m_floors.size()); }

DungeonMap& DungeonWorld::getFloor(int floorNumber) {
    if (floorNumber < 1 || floorNumber > getFloorCount()) throw std::out_of_range("던전 층 범위를 벗어났습니다.");
    return m_floors[static_cast<std::size_t>(floorNumber - 1)];
}

const DungeonMap& DungeonWorld::getFloor(int floorNumber) const {
    if (floorNumber < 1 || floorNumber > getFloorCount()) throw std::out_of_range("던전 층 범위를 벗어났습니다.");
    return m_floors[static_cast<std::size_t>(floorNumber - 1)];
}

std::vector<WorldObject>& DungeonWorld::getObjects() { return m_objects; }
const std::vector<WorldObject>& DungeonWorld::getObjects() const { return m_objects; }

WorldObject* DungeonWorld::findObject(const std::string& id) {
    const auto it = std::find_if(m_objects.begin(), m_objects.end(), [&](const auto& object) { return object.id == id; });
    return it == m_objects.end() ? nullptr : &*it;
}

const WorldObject* DungeonWorld::findObject(const std::string& id) const {
    const auto it = std::find_if(m_objects.begin(), m_objects.end(), [&](const auto& object) { return object.id == id; });
    return it == m_objects.end() ? nullptr : &*it;
}

WorldObject* DungeonWorld::findObjectAt(int floorNumber, int x, int y) {
    const auto it = std::find_if(m_objects.begin(), m_objects.end(), [&](const auto& object) {
        return object.floor == floorNumber && object.x == x && object.y == y;
    });
    return it == m_objects.end() ? nullptr : &*it;
}

const WorldObject* DungeonWorld::findObjectAt(int floorNumber, int x, int y) const {
    const auto it = std::find_if(m_objects.begin(), m_objects.end(), [&](const auto& object) {
        return object.floor == floorNumber && object.x == x && object.y == y;
    });
    return it == m_objects.end() ? nullptr : &*it;
}

nlohmann::json DungeonWorld::toJson() const {
    validate();
    nlohmann::json floors = nlohmann::json::array();
    for (const auto& floor : m_floors) floors.push_back(floor.toJson());
    nlohmann::json objects = nlohmann::json::array();
    for (const auto& object : m_objects) {
        objects.push_back({{"id", object.id}, {"questId", object.questId},
                           {"targetId", object.targetId}, {"kind", kindToString(object.kind)},
                           {"state", stateToString(object.state)}, {"floor", object.floor},
                           {"x", object.x}, {"y", object.y}});
    }
    return {{"version", 1}, {"seed", m_seed}, {"floors", floors}, {"objects", objects}};
}

DungeonWorld DungeonWorld::fromJson(const nlohmann::json& json) {
    if (!json.is_object() || json.value("version", 0) != 1) {
        throw std::runtime_error("지원하지 않는 월드 snapshot입니다.");
    }
    DungeonWorld world;
    world.m_seed = json.at("seed").get<std::uint32_t>();
    if (world.m_seed == 0U) throw std::runtime_error("월드 seed는 0일 수 없습니다.");
    const auto& floors = json.at("floors");
    if (!floors.is_array() || floors.size() != FLOOR_COUNT) {
        throw std::runtime_error("월드는 정확히 3개 층이어야 합니다.");
    }
    for (int floor = 1; floor <= FLOOR_COUNT; ++floor) {
        world.m_floors.push_back(DungeonMap::fromJson(floors.at(floor - 1), floor));
    }

    const auto& objects = json.at("objects");
    if (!objects.is_array() || objects.size() != canonicalObjects().size()) {
        throw std::runtime_error("월드 목표 오브젝트 수가 잘못됐습니다.");
    }
    std::unordered_set<std::string> ids;
    std::unordered_set<std::string> positions;
    for (const auto& entry : objects) {
        WorldObject object;
        object.id = entry.at("id").get<std::string>();
        object.questId = entry.at("questId").get<std::string>();
        object.targetId = entry.at("targetId").get<std::string>();
        object.kind = kindFromString(entry.at("kind").get<std::string>());
        object.state = stateFromString(entry.at("state").get<std::string>());
        object.floor = entry.at("floor").get<int>();
        object.x = entry.at("x").get<int>();
        object.y = entry.at("y").get<int>();
        const auto canonical = std::find_if(canonicalObjects().begin(), canonicalObjects().end(),
            [&](const auto& candidate) { return candidate.id == object.id; });
        if (canonical == canonicalObjects().end() || canonical->questId != object.questId ||
            canonical->targetId != object.targetId || canonical->kind != object.kind ||
            canonical->floor != object.floor) {
            throw std::runtime_error("월드 목표 오브젝트 계약이 잘못됐습니다.");
        }
        if (!ids.insert(object.id).second || object.x < 1 || object.x >= DungeonMap::MAP_WIDTH - 1 ||
            object.y < 1 || object.y >= DungeonMap::MAP_HEIGHT - 1 ||
            world.getFloor(object.floor).getTile(object.x, object.y) != TileType::EMPTY) {
            throw std::runtime_error("월드 목표 오브젝트 위치가 잘못됐습니다.");
        }
        const std::string position = std::to_string(object.floor) + ":" +
            std::to_string(object.x) + ":" + std::to_string(object.y);
        if (!positions.insert(position).second) throw std::runtime_error("월드 목표 위치가 중복됩니다.");
        world.m_objects.push_back(std::move(object));
    }
    world.m_generated = true;
    world.validate();
    return world;
}

void DungeonWorld::validate() const {
    if (!m_generated || m_seed == 0U || m_floors.size() != FLOOR_COUNT ||
        m_objects.size() != canonicalObjects().size()) {
        throw std::runtime_error("월드 기본 구조가 canonical 계약과 다릅니다.");
    }
    for (int floor = 1; floor <= FLOOR_COUNT; ++floor) getFloor(floor).validateForFloor(floor);
    std::unordered_set<std::string> ids;
    std::unordered_set<std::string> positions;
    for (const auto& object : m_objects) {
        const auto canonical = std::find_if(canonicalObjects().begin(), canonicalObjects().end(),
            [&](const auto& candidate) { return candidate.id == object.id; });
        if (canonical == canonicalObjects().end() || canonical->questId != object.questId ||
            canonical->targetId != object.targetId || canonical->kind != object.kind ||
            canonical->floor != object.floor || !ids.insert(object.id).second ||
            object.x < 1 || object.x >= DungeonMap::MAP_WIDTH - 1 ||
            object.y < 1 || object.y >= DungeonMap::MAP_HEIGHT - 1 ||
            getFloor(object.floor).getTile(object.x, object.y) != TileType::EMPTY) {
            throw std::runtime_error("월드 목표 오브젝트 계약이 잘못됐습니다.");
        }
        const std::string position = std::to_string(object.floor) + ":" +
            std::to_string(object.x) + ":" + std::to_string(object.y);
        if (!positions.insert(position).second) throw std::runtime_error("월드 목표 위치가 중복됩니다.");
        if (object.state == WorldObjectState::DISCOVERED &&
            !getFloor(object.floor).isVisited(object.x, object.y)) {
            throw std::runtime_error("발견된 목표는 FOW에서 방문 상태여야 합니다.");
        }
    }
}

} // namespace crawl
