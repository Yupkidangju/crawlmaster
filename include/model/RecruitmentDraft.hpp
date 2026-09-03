#ifndef RECRUITMENT_DRAFT_HPP
#define RECRUITMENT_DRAFT_HPP

#include "model/Character.hpp"

#include <memory>
#include <string>
#include <vector>

namespace crawl {

class Party;
class SessionRng;

class RecruitmentDraft {
public:
    RecruitmentDraft(SessionRng& random, std::vector<std::string> names);

    const Character& preview() const;
    void reroll();
    bool confirm(Party& party);

private:
    SessionRng& m_random;
    std::vector<std::string> m_names;
    std::size_t m_nameIndex = 0;
    std::shared_ptr<Character> m_candidate;
    bool m_confirmed = false;

    void createCandidate();
};

} // namespace crawl

#endif // RECRUITMENT_DRAFT_HPP
