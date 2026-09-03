// [v0.4.0] MonsterFactory.hpp 신규 작성
// 몬스터 ID를 통해 인카운터 몬스터 객체를 생성해 반환하는 팩토리 클래스 정의.

#ifndef MONSTER_FACTORY_HPP
#define MONSTER_FACTORY_HPP

#include "Monster.hpp"
#include <memory>
#include <vector>

namespace crawl {

class SessionRng;

enum class EncounterTier {
    EARLY,
    MIDDLE,
    LATE
};

// MonsterFactory 클래스: 던전 출현 몬스터 생성 담당
class MonsterFactory {
public:
    // 고유 ID에 따른 새 몬스터 객체 생성 (std::shared_ptr 반환)
    static std::shared_ptr<Monster> createMonster(const std::string& id);

    // 던전 1층에서 출현할 수 있는 몬스터 리스트 획득 (랜덤 스폰용)
    static std::shared_ptr<Monster> createRandomMonster();
    static std::shared_ptr<Monster> createRandomMonster(EncounterTier tier, SessionRng& random);
    static std::vector<std::string> getEncounterPool(EncounterTier tier);
    static std::vector<std::string> getRegisteredIds();
    static std::vector<std::string> getDropItemIds(const std::string& monsterId);
};

} // namespace crawl

#endif // MONSTER_FACTORY_HPP
