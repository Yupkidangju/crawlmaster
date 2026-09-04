#ifndef RECRUITMENT_DRAFT_HPP
#define RECRUITMENT_DRAFT_HPP

#include "model/Character.hpp"

#include <string>

namespace crawl {

class Party;
class SessionRng;

class RecruitmentDraft {
public:
    explicit RecruitmentDraft(SessionRng& random);

    bool setName(const std::string& name);
    bool setAge(int age);
    bool setGender(Gender gender);
    bool setClass(CharacterClass characterClass);

    const CharacterIdentity& identity() const;
    const AbilityScore& baseAbilities() const;
    const AbilityScore& abilities() const;
    int remainingPoints() const;
    int increaseCost(Ability ability) const;
    bool increase(Ability ability);
    bool decrease(Ability ability);
    void reroll();
    bool isReady() const;
    std::shared_ptr<Character> createCandidate() const;
    bool confirm(Party& party);

    static bool isValidName(const std::string& name);

private:
    SessionRng& m_random;
    CharacterIdentity m_identity;
    AbilityScore m_baseAbilities;
    AbilityScore m_abilities;
    int m_remainingPoints = 10;
    bool m_confirmed = false;

    static int& select(AbilityScore& scores, Ability ability);
    static int select(const AbilityScore& scores, Ability ability);
    void rollAbilities();
};

} // namespace crawl

#endif // RECRUITMENT_DRAFT_HPP
