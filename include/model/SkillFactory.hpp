#ifndef SKILL_FACTORY_HPP
#define SKILL_FACTORY_HPP

#include "model/Character.hpp"
#include "model/Skill.hpp"
#include <memory>
#include <string>
#include <vector>

namespace crawl {

class SkillFactory {
public:
    static std::shared_ptr<Skill> createSkill(const std::string& id);
    static std::vector<std::string> getRegisteredIds();
    static std::vector<std::shared_ptr<Skill>> getSkillsForClassAndLevel(
        CharacterClass charClass, int level, Specialization spec = Specialization::NONE);
};

} // namespace crawl

#endif // SKILL_FACTORY_HPP
