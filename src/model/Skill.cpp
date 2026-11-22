#include "model/Skill.hpp"
#include "core/LocalizationManager.hpp"

namespace crawl {

std::string Skill::getName() const {
    return LocalizationManager::getInstance().getContent("SKILL", getId(), "NAME");
}

std::string Skill::getDescription() const {
    return LocalizationManager::getInstance().getContent("SKILL", getId(), "DESC");
}

} // namespace crawl
