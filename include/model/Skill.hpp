#ifndef SKILL_HPP
#define SKILL_HPP

#include "model/Equipment.hpp"
#include <memory>
#include <string>
#include <vector>

namespace crawl {

class Character;
class Monster;

enum class SkillTargetType {
    SINGLE_FOE = 0,
    ALL_FOES = 1,
    SINGLE_ALLY = 2,
    ALL_ALLIES = 3,
    SELF = 4
};

class Skill {
public:
    virtual ~Skill() = default;

    [[nodiscard]] virtual std::string getId() const = 0;
    [[nodiscard]] virtual std::string getName() const;
    [[nodiscard]] virtual std::string getDescription() const;
    [[nodiscard]] virtual int getRequiredLevel() const = 0;
    [[nodiscard]] virtual bool isSpell() const = 0;
    [[nodiscard]] virtual int getSpellLevel() const = 0;
    [[nodiscard]] virtual SkillTargetType getTargetType() const = 0;
    [[nodiscard]] virtual DamageType getDamageType() const { return DamageType::SLASHING; }
    [[nodiscard]] virtual int getBaseStagger() const { return 1; }

    virtual bool execute(Character& caster,
                         std::vector<std::shared_ptr<Character>>& allies,
                         std::vector<std::shared_ptr<Monster>>& foes,
                         int targetIdx,
                         std::vector<std::string>& logOutput) = 0;
};

} // namespace crawl

#endif // SKILL_HPP
