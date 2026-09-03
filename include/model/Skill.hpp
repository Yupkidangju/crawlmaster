// [v0.8.0] Skill.hpp 신규 작성
// 캐릭터 및 몬스터 전투에서 사용될 주문(Spell)과 특수 기술(Skill)의 추상 공통 인터페이스 정의.
// 단일 대상, 전체 대상, 자가 시전 및 회복/버프/디버프 등 다양한 효과 시전을 수용할 수 있게 설계됨.

#ifndef SKILL_HPP
#define SKILL_HPP

#include <string>
#include <memory>
#include <vector>

namespace crawl {

// 전방 선언을 통해 헤더 순환 참조 방지
class Character;
class Monster;

// 스킬 및 주문의 대상 설정 타입
enum class SkillTargetType {
    SINGLE_FOE,    // 적 단일 대상
    ALL_FOES,      // 적 전체 대상
    SINGLE_ALLY,   // 아군 단일 대상
    ALL_ALLIES,    // 아군 전체 대상
    SELF           // 시전자 자신
};

// Skill: 모든 액티브 기술 및 주문의 추상 베이스 클래스
class Skill {
public:
    virtual ~Skill() = default;

    // 스킬 고유 ID 반환 (예: skl_slash, spl_fireball)
    virtual std::string getId() const = 0;

    // 현재 locale의 스킬 이름 반환
    virtual std::string getName() const;

    // 현재 locale의 스킬 상세 설명 반환
    virtual std::string getDescription() const;

    // 습득에 요구되는 최소 캐릭터 레벨 반환
    virtual int getRequiredLevel() const = 0;

    // 마법 주문 슬롯을 소모하는 마법인지 여부 반환
    virtual bool isSpell() const = 0;

    // 주문일 경우 주문 레벨 반환 (비주문인 경우 0)
    virtual int getSpellLevel() const = 0;

    // 대상 설정 범위 반환
    virtual SkillTargetType getTargetType() const = 0;

    // 전투 중 스킬/주문 효과 실행 가상 함수
    // caster: 시전자 캐릭터
    // allies: 살아있는 모든 파티 대원 리스트
    // foes: 전투 중인 모든 몬스터 목록
    // targetIdx: 선택된 타겟 인덱스 (SINGLE 타입인 경우 유효)
    // logOutput: 전투 로그가 누적되는 텍스트 출력 스트림
    virtual bool execute(Character& caster,
                         std::vector<std::shared_ptr<Character>>& allies,
                         std::vector<std::shared_ptr<Monster>>& foes,
                         int targetIdx,
                         std::vector<std::string>& logOutput) = 0;
};

} // namespace crawl

#endif // SKILL_HPP
