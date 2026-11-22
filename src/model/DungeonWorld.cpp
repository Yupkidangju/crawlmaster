#include "model/DungeonWorld.hpp"
#include "core/SessionRng.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace crawl {

void DungeonWorld::generate(std::uint32_t seed) {
    m_seed = seed == 0U ? 0xC4A71001U : seed;
    m_floors.clear();
    m_objects.clear();

    SessionRng rng(m_seed);
    for (int f = 1; f <= FLOOR_COUNT; ++f) {
        DungeonMap floorMap;
        floorMap.generate(m_seed ^ (static_cast<std::uint32_t>(f) * 0x9E3779B9U), f);
        m_floors.push_back(std::move(floorMap));
    }

    // 각 층에 도달 가능한 빈 셀을 거리 기준으로 수집해 탐험 기믹·보스·퀘스트 목표 배치
    for (int f = 1; f <= FLOOR_COUNT; ++f) {
        const auto& map = m_floors[static_cast<std::size_t>(f - 1)];
        std::vector<std::pair<int, int>> reachable;
        std::set<std::pair<int, int>> used;
        used.insert({1, 1});

        for (int y = 1; y < DungeonMap::MAP_HEIGHT - 1; ++y) {
            for (int x = 1; x < DungeonMap::MAP_WIDTH - 1; ++x) {
                if (map.getTile(x, y) == TileType::EMPTY && map.getDistanceFromStart(x, y) >= 3) {
                    reachable.emplace_back(x, y);
                }
            }
        }
        std::sort(reachable.begin(), reachable.end(), [&](const auto& a, const auto& b) {
            return map.getDistanceFromStart(a.first, a.second) < map.getDistanceFromStart(b.first, b.second);
        });

        auto pickNearFraction = [&](float frac) -> std::pair<int, int> {
            if (reachable.empty()) return {1, 1};
            const int ideal = std::clamp(static_cast<int>(frac * static_cast<float>(reachable.size() - 1)),
                                         0, static_cast<int>(reachable.size()) - 1);
            for (int delta = 0; delta < static_cast<int>(reachable.size()); ++delta) {
                for (int sign : {1, -1}) {
                    const int idx = ideal + delta * sign;
                    if (idx < 0 || idx >= static_cast<int>(reachable.size())) continue;
                    const auto coord = reachable[static_cast<std::size_t>(idx)];
                    if (!used.contains(coord)) {
                        used.insert(coord);
                        return coord;
                    }
                }
            }
            return reachable.back();
        };

        auto addObj = [&](std::string id, std::string questId, std::string targetId,
                          WorldObjectKind kind, float frac) {
            const auto [ox, oy] = pickNearFraction(frac);
            m_objects.push_back(WorldObject{
                std::move(id), std::move(questId), std::move(targetId),
                kind, WorldObjectState::PRESENT, f, ox, oy
            });
        };

        // 1. 공통 탐험 기믹: 화톳불 성소(중반), 숏컷 레버(초중반), 로어 석판(초반), 보물상자 2개, 함정 2개
        addObj("obj_sanctuary_f" + std::to_string(f), "", "sanctuary", WorldObjectKind::SANCTUARY, 0.52f);
        addObj("obj_lever_f" + std::to_string(f), "", "lever", WorldObjectKind::LEVER, 0.34f);
        addObj("obj_lore_f" + std::to_string(f), "", "LORE_FLOOR_" + std::to_string(f),
               WorldObjectKind::LORE_PLAQUE, 0.16f);
        addObj("obj_chest_a_f" + std::to_string(f), "", "chest", WorldObjectKind::CHEST, 0.42f);
        addObj("obj_chest_b_f" + std::to_string(f), "", "chest", WorldObjectKind::CHEST, 0.78f);
        addObj("obj_trap_a_f" + std::to_string(f), "", "trap_spike", WorldObjectKind::TRAP, 0.25f);
        addObj("obj_trap_b_f" + std::to_string(f), "", "trap_poison", WorldObjectKind::TRAP, 0.66f);

        // 비밀 방 내부에도 고급 보물상자 배치
        for (int y = 1; y < DungeonMap::MAP_HEIGHT - 1; ++y) {
            for (int x = 1; x < DungeonMap::MAP_WIDTH - 1; ++x) {
                if (map.getTile(x, y) == TileType::EMPTY && map.getDistanceFromStart(x, y) < 0) {
                    m_objects.push_back(WorldObject{
                        "obj_secret_chest_f" + std::to_string(f), "", "chest_secret",
                        WorldObjectKind::CHEST, WorldObjectState::PRESENT, f, x, y
                    });
                    break;
                }
            }
        }

        // 2. 층별 퀘스트 및 고정 중보스 배치
        if (f == 1) {
            addObj("obj_sun_chalice", "qst_01_relic", "key_sun_chalice", WorldObjectKind::QUEST_ITEM, 0.62f);
            addObj("obj_crypt_warden", "qst_02_warden", "mon_crypt_warden", WorldObjectKind::QUEST_BOSS, 0.88f);
        } else if (f == 2) {
            addObj("obj_missing_scout", "qst_03_scout", "npc_missing_scout", WorldObjectKind::NPC, 0.58f);
            addObj("obj_bog_hydra", "qst_04_hydra", "mon_bog_hydra", WorldObjectKind::QUEST_BOSS, 0.88f);
        } else if (f == 3) {
            addObj("obj_bone_phylactery", "qst_05_phylactery", "key_bone_phylactery", WorldObjectKind::QUEST_ITEM, 0.60f);
            addObj("obj_ossuary_lich", "qst_06_lich", "mon_ossuary_lich", WorldObjectKind::QUEST_BOSS, 0.88f);
        } else if (f == 4) {
            addObj("obj_iron_colossus", "qst_07_forge_core", "mon_iron_colossus", WorldObjectKind::QUEST_BOSS, 0.88f);
        } else if (f == 5) {
            // 5층 최종 보스도 보스 게이트 외에 퀘스트 목표로 연동
            addObj("obj_sanctum_warden", "qst_08_elder_wyrm", "mon_sanctum_wyrm", WorldObjectKind::QUEST_BOSS, 0.82f);
        }
    }

    (void)rng;
    m_generated = true;
    validate();
}

bool DungeonWorld::isGenerated() const { return m_generated; }
std::uint32_t DungeonWorld::getSeed() const { return m_seed; }
int DungeonWorld::getFloorCount() const { return static_cast<int>(m_floors.size()); }

DungeonMap& DungeonWorld::getFloor(int floorNumber) {
    if (floorNumber < 1 || floorNumber > static_cast<int>(m_floors.size())) {
        throw std::out_of_range("invalid floor number");
    }
    return m_floors[static_cast<std::size_t>(floorNumber - 1)];
}

const DungeonMap& DungeonWorld::getFloor(int floorNumber) const {
    if (floorNumber < 1 || floorNumber > static_cast<int>(m_floors.size())) {
        throw std::out_of_range("invalid floor number");
    }
    return m_floors[static_cast<std::size_t>(floorNumber - 1)];
}

std::vector<WorldObject>& DungeonWorld::getObjects() { return m_objects; }
const std::vector<WorldObject>& DungeonWorld::getObjects() const { return m_objects; }

WorldObject* DungeonWorld::findObject(const std::string& id) {
    for (auto& obj : m_objects) {
        if (obj.id == id) return &obj;
    }
    return nullptr;
}

const WorldObject* DungeonWorld::findObject(const std::string& id) const {
    for (const auto& obj : m_objects) {
        if (obj.id == id) return &obj;
    }
    return nullptr;
}

WorldObject* DungeonWorld::findObjectAt(int floorNumber, int x, int y) {
    for (auto& obj : m_objects) {
        if (obj.floor == floorNumber && obj.x == x && obj.y == y) return &obj;
    }
    return nullptr;
}

const WorldObject* DungeonWorld::findObjectAt(int floorNumber, int x, int y) const {
    for (const auto& obj : m_objects) {
        if (obj.floor == floorNumber && obj.x == x && obj.y == y) return &obj;
    }
    return nullptr;
}

nlohmann::json DungeonWorld::toJson() const {
    nlohmann::json floorsJson = nlohmann::json::array();
    for (const auto& f : m_floors) floorsJson.push_back(f.toJson());

    nlohmann::json objsJson = nlohmann::json::array();
    for (const auto& o : m_objects) {
        objsJson.push_back({
            {"id", o.id},
            {"questId", o.questId},
            {"targetId", o.targetId},
            {"kind", static_cast<int>(o.kind)},
            {"state", static_cast<int>(o.state)},
            {"floor", o.floor},
            {"x", o.x},
            {"y", o.y}
        });
    }
    return nlohmann::json{
        {"seed", m_seed},
        {"floors", std::move(floorsJson)},
        {"objects", std::move(objsJson)}
    };
}

DungeonWorld DungeonWorld::fromJson(const nlohmann::json& json) {
    if (!json.is_object()) throw std::runtime_error("dungeonWorld must be object");
    DungeonWorld world;
    world.m_seed = json.at("seed").get<std::uint32_t>();
    const auto& floorsJson = json.at("floors");
    if (!floorsJson.is_array() || static_cast<int>(floorsJson.size()) != FLOOR_COUNT) {
        throw std::runtime_error("dungeonWorld must contain 5 floors");
    }
    for (int i = 0; i < FLOOR_COUNT; ++i) {
        world.m_floors.push_back(DungeonMap::fromJson(floorsJson[static_cast<std::size_t>(i)], i + 1));
    }
    const auto& objsJson = json.at("objects");
    if (!objsJson.is_array()) throw std::runtime_error("world objects must be array");
    for (const auto& o : objsJson) {
        WorldObject obj;
        obj.id = o.at("id").get<std::string>();
        obj.questId = o.value("questId", "");
        obj.targetId = o.at("targetId").get<std::string>();
        const int kv = o.at("kind").get<int>();
        const int sv = o.at("state").get<int>();
        if (kv < 0 || kv > static_cast<int>(WorldObjectKind::LORE_PLAQUE) ||
            sv < 0 || sv > static_cast<int>(WorldObjectState::RESOLVED)) {
            throw std::runtime_error("invalid world object kind or state");
        }
        obj.kind = static_cast<WorldObjectKind>(kv);
        obj.state = static_cast<WorldObjectState>(sv);
        obj.floor = o.at("floor").get<int>();
        obj.x = o.at("x").get<int>();
        obj.y = o.at("y").get<int>();
        world.m_objects.push_back(std::move(obj));
    }
    world.m_generated = true;
    world.validate();
    return world;
}

void DungeonWorld::validate() const {
    if (!m_generated || static_cast<int>(m_floors.size()) != FLOOR_COUNT) {
        throw std::runtime_error("world does not have 5 floors");
    }
    for (int f = 1; f <= FLOOR_COUNT; ++f) {
        m_floors[static_cast<std::size_t>(f - 1)].validateForFloor(f);
    }
    for (const auto& obj : m_objects) {
        if (obj.floor < 1 || obj.floor > FLOOR_COUNT ||
            obj.x < 1 || obj.x >= DungeonMap::MAP_WIDTH - 1 ||
            obj.y < 1 || obj.y >= DungeonMap::MAP_HEIGHT - 1) {
            throw std::runtime_error("world object out of bounds: " + obj.id);
        }
    }
}

} // namespace crawl
