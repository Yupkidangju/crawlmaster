// [v0.8.0] SkillFactory.hpp 신규 작성
// 고유 식별 ID에 맵핑되는 구체 Skill 객체를 동적으로 할당해 반환하는 팩토리 클래스 정의.

#ifndef SKILL_FACTORY_HPP
#define SKILL_FACTORY_HPP

#include "model/Skill.hpp"
#include <memory>
#include <string>
#include <vector>

namespace crawl {

enum class CharacterClass;

class SkillFactory {
public:
    // 지정된 ID에 부합하는 Skill 인스턴스 생성 반환
    static std::shared_ptr<Skill> createSkill(const std::string& id);
    static std::vector<std::string> getRegisteredIds();

    // 클래스 종류와 레벨에 부합하는 사용 가능한 모든 스킬 리스트 생성 반환
    static std::vector<std::shared_ptr<Skill>> getSkillsForClassAndLevel(CharacterClass charClass, int level);
};

} // namespace crawl

#endif // SKILL_FACTORY_HPP
