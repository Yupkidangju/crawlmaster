#ifndef MONSTER_FACTORY_HPP
#define MONSTER_FACTORY_HPP

#include "model/Monster.hpp"
#include <memory>
#include <string>
#include <vector>

namespace crawl {

class SessionRng;

enum class EncounterTier {
    EARLY = 0,   // Floor 1
    MIDDLE = 1,  // Floor 2
    LATE = 2,    // Floor 3
    DEEP = 3,    // Floor 4
    SANCTUM = 4  // Floor 5
};

class MonsterFactory {
public:
    static std::shared_ptr<Monster> createMonster(const std::string& id);
    static std::shared_ptr<Monster> createRandomMonster();
    static std::shared_ptr<Monster> createRandomMonster(EncounterTier tier, SessionRng& random);
    static EncounterTier tierForFloor(int floorNumber);
    static std::vector<std::string> getEncounterPool(EncounterTier tier);
    static std::vector<std::string> getRegisteredIds();
    static std::vector<std::string> getDropItemIds(const std::string& monsterId);
    static int getMonsterFloor(const std::string& monsterId);
};

} // namespace crawl

#endif // MONSTER_FACTORY_HPP
