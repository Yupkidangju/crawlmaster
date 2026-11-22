#include "model/Party.hpp"
#include "core/SessionRng.hpp"
#include "model/ItemFactory.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace crawl {

Party::Party() {
    resetToDefault();
}

void Party::setDefaultSavePath(const std::string& filePath) { s_defaultSavePath = filePath; }

const std::string& Party::getDefaultSavePath() {
    if (s_defaultSavePath.empty()) {
        s_defaultSavePath = Persistence::defaultSavePath().string();
    }
    return s_defaultSavePath;
}

bool Party::hasRecoverableSave(const std::string& filePath) {
    return std::filesystem::exists(filePath) || std::filesystem::exists(filePath + ".bak");
}

SavePreviewInfo Party::inspectSavePreview(const std::string& filePath) {
    SavePreviewInfo info;
    std::filesystem::path target(filePath);
    if (!std::filesystem::exists(target)) {
        target = filePath + ".bak";
        if (!std::filesystem::exists(target)) return info;
    }
    try {
        std::ifstream f(target, std::ios::binary);
        if (!f.is_open()) return info;
        nlohmann::json j;
        f >> j;
        if (!j.is_object()) return info;
        const int ver = j.value("schemaVersion", 0);
        if (ver < SAVE_SCHEMA_VERSION) {
            info.legacyIncompatible = true;
            return info;
        }
        if (ver != SAVE_SCHEMA_VERSION) return info;
        info.valid = true;
        info.gold = j.value("gold", 0);
        info.deepestFloor = j.value("deepestFloorReached", 1);
        info.campaignCompleted = j.value("campaignCompleted", false);
        if (j.contains("members") && j["members"].is_array()) {
            info.memberCount = static_cast<int>(j["members"].size());
            for (const auto& m : j["members"]) {
                info.maxLevel = std::max(info.maxLevel, m.value("level", 1));
            }
        }
    } catch (...) {
        info.valid = false;
    }
    return info;
}

bool Party::addMember(std::shared_ptr<Character> member) {
    if (!member || static_cast<int>(m_members.size()) >= MAX_MEMBERS) return false;
    m_members.push_back(std::move(member));
    return true;
}

void Party::removeMember(int index) {
    if (index >= 0 && index < static_cast<int>(m_members.size())) {
        m_members.erase(m_members.begin() + index);
    }
}

const std::vector<std::shared_ptr<Character>>& Party::getMembers() const { return m_members; }

std::shared_ptr<Character> Party::getMember(int index) const {
    if (index < 0 || index >= static_cast<int>(m_members.size())) return nullptr;
    return m_members[static_cast<std::size_t>(index)];
}

int Party::getMemberCount() const { return static_cast<int>(m_members.size()); }

bool Party::hasClassInParty(CharacterClass c) const {
    for (const auto& m : m_members) {
        if (m && !m->isDead() && m->getClass() == c) return true;
    }
    return false;
}

int Party::getGold() const { return m_gold; }
void Party::addGold(int amount) { if (amount > 0) m_gold += amount; }

bool Party::spendGold(int amount) {
    if (amount < 0 || m_gold < amount) return false;
    m_gold -= amount;
    return true;
}

const std::vector<std::shared_ptr<Item>>& Party::getInventory() const { return m_inventory; }

void Party::addItem(std::shared_ptr<Item> item) {
    if (item) m_inventory.push_back(std::move(item));
}

void Party::insertItem(int index, std::shared_ptr<Item> item) {
    if (!item) return;
    const int clamped = std::clamp(index, 0, static_cast<int>(m_inventory.size()));
    m_inventory.insert(m_inventory.begin() + clamped, std::move(item));
}

void Party::removeItem(int index) {
    if (index >= 0 && index < static_cast<int>(m_inventory.size())) {
        m_inventory.erase(m_inventory.begin() + index);
    }
}

const std::vector<std::shared_ptr<Quest>>& Party::getActiveQuests() const { return m_activeQuests; }

void Party::acceptQuest(std::shared_ptr<Quest> quest) {
    if (!quest || hasQuest(quest->getId()) || isQuestCompleted(quest->getId())) return;
    if (quest->getType() == QuestType::RETRIEVE_KEY_ITEM && hasKeyItem(quest->getTargetId())) {
        quest->markObjectiveComplete();
    }
    m_activeQuests.push_back(std::move(quest));
}

void Party::completeQuest(const std::string& questId) {
    for (auto it = m_activeQuests.begin(); it != m_activeQuests.end(); ++it) {
        if ((*it)->getId() == questId) {
            (*it)->setCompleted(true);
            m_completedQuestIds.insert(questId);
            m_activeQuests.erase(it);
            return;
        }
    }
}

void Party::abandonQuest(const std::string& questId) {
    for (auto it = m_activeQuests.begin(); it != m_activeQuests.end(); ++it) {
        if ((*it)->getId() == questId) {
            m_activeQuests.erase(it);
            return;
        }
    }
}

bool Party::hasQuest(const std::string& questId) const {
    return getQuest(questId) != nullptr;
}

std::shared_ptr<Quest> Party::getQuest(const std::string& questId) const {
    for (const auto& q : m_activeQuests) {
        if (q && q->getId() == questId) return q;
    }
    return nullptr;
}

void Party::updateQuestKillProgress(const std::string& monsterId, int count) {
    for (auto& q : m_activeQuests) {
        if (q && (q->getType() == QuestType::KILL || q->getType() == QuestType::DEFEAT_BOSS)) {
            q->updateProgress(monsterId, count);
        }
    }
}

void Party::updateQuestCollectProgress() {
    for (auto& q : m_activeQuests) {
        if (!q) continue;
        if (q->getType() == QuestType::RETRIEVE_KEY_ITEM && hasKeyItem(q->getTargetId())) {
            q->markObjectiveComplete();
        }
    }
}

bool Party::markQuestObjectiveComplete(const std::string& questId) {
    if (auto q = getQuest(questId)) return q->markObjectiveComplete();
    return false;
}

bool Party::addKeyItem(const std::string& itemId) {
    if (itemId.empty()) return false;
    const bool inserted = m_keyItems.insert(itemId).second;
    updateQuestCollectProgress();
    return inserted;
}

bool Party::removeKeyItem(const std::string& itemId) { return m_keyItems.erase(itemId) > 0; }
bool Party::hasKeyItem(const std::string& itemId) const { return m_keyItems.contains(itemId); }
const std::unordered_set<std::string>& Party::getKeyItems() const { return m_keyItems; }

DungeonWorld& Party::getWorld() { return m_world; }
const DungeonWorld& Party::getWorld() const { return m_world; }

void Party::resetToDefault() {
    m_members.clear();
    m_inventory.clear();
    m_activeQuests.clear();
    m_completedQuestIds.clear();
    m_keyItems.clear();
    m_bestiary.clear();
    m_gold = 180;
    m_campaignCompleted = false;
    m_deepestFloorReached = 1;
    m_totalTurns = 0;
    m_hasActiveSaveSession = false;
    m_recoveryPending = false;
    m_lastSessionSeed = SessionRng::global().seed();
    m_sessionRngDrawCount = SessionRng::global().drawCount();

    // 기본 4인 전술 파티 (전열 2 + 후열 2)
    auto warrior = std::make_shared<Character>(
        CharacterIdentity{"Aldric", 28, Gender::MALE, CharacterClass::WARRIOR},
        AbilityScore{16, 12, 15, 10, 11, 12});
    auto cleric = std::make_shared<Character>(
        CharacterIdentity{"Seraphina", 26, Gender::FEMALE, CharacterClass::CLERIC},
        AbilityScore{13, 11, 14, 11, 16, 13});
    auto rogue = std::make_shared<Character>(
        CharacterIdentity{"Vesper", 23, Gender::NON_BINARY, CharacterClass::ROGUE},
        AbilityScore{11, 16, 12, 13, 12, 14});
    auto mage = std::make_shared<Character>(
        CharacterIdentity{"Lyra", 25, Gender::FEMALE, CharacterClass::MAGE},
        AbilityScore{9, 13, 12, 17, 13, 12});

    addMember(warrior);
    addMember(cleric);
    addMember(rogue);
    addMember(mage);

    addItem(ItemFactory::createItem("pot_heal"));
    addItem(ItemFactory::createItem("pot_heal"));
    addItem(ItemFactory::createItem("pot_mana"));
    addItem(ItemFactory::createItem("scr_cure"));
    addItem(ItemFactory::createItem("bomb_fire"));

    m_world.generate(m_lastSessionSeed);
}

PersistenceResult Party::startNewGame(const std::string& filePath) {
    resetToDefault();
    m_hasActiveSaveSession = true;
    m_recoveryPending = false;
    m_lastSessionSeed = SessionRng::global().seed();
    m_sessionRngDrawCount = SessionRng::global().drawCount();
    m_world.generate(m_lastSessionSeed);
    return saveToFile(filePath);
}

bool Party::isQuestCompleted(const std::string& questId) const {
    return m_completedQuestIds.contains(questId);
}
const std::unordered_set<std::string>& Party::getCompletedQuestIds() const { return m_completedQuestIds; }
bool Party::isCampaignCompleted() const { return m_campaignCompleted; }
void Party::setCampaignCompleted(bool completed) { m_campaignCompleted = completed; }
std::uint32_t Party::getLastSessionSeed() const { return m_lastSessionSeed; }
void Party::setLastSessionSeed(std::uint32_t seed) { m_lastSessionSeed = seed; }
std::uint64_t Party::getSessionRngDrawCount() const { return m_sessionRngDrawCount; }
bool Party::hasActiveSaveSession() const { return m_hasActiveSaveSession; }
bool Party::isRecoveryPending() const { return m_recoveryPending; }
void Party::markRecoveryPending() { m_recoveryPending = true; }

PartyCheckpoint Party::captureCheckpoint() const {
    PartyCheckpoint cp;
    cp.gold = m_gold;
    for (const auto& item : m_inventory) {
        if (item) cp.inventory.push_back(item->clone());
    }
    for (const auto& m : m_members) {
        if (m) cp.members.push_back(Character::fromJson(m->toJson()));
    }
    for (const auto& q : m_activeQuests) {
        if (q) cp.quests.push_back(Quest::fromJson(q->toJson()));
    }
    cp.completedQuestIds = m_completedQuestIds;
    cp.keyItems = m_keyItems;
    cp.bestiary = m_bestiary;
    if (m_world.isGenerated()) cp.world = DungeonWorld::fromJson(m_world.toJson());
    cp.campaignCompleted = m_campaignCompleted;
    cp.deepestFloorReached = m_deepestFloorReached;
    cp.totalTurns = m_totalTurns;
    cp.lastSessionSeed = m_lastSessionSeed;
    cp.sessionRngDrawCount = m_sessionRngDrawCount;
    cp.activeSaveSession = m_hasActiveSaveSession;
    cp.recoveryPending = m_recoveryPending;
    cp.globalRngSeed = SessionRng::global().seed();
    cp.globalRngDrawCount = SessionRng::global().drawCount();
    return cp;
}

void Party::restoreCheckpoint(const PartyCheckpoint& cp) {
    m_gold = cp.gold;
    m_inventory.clear();
    for (const auto& item : cp.inventory) {
        if (item) m_inventory.push_back(item->clone());
    }
    m_members.clear();
    for (const auto& m : cp.members) {
        if (m) m_members.push_back(Character::fromJson(m->toJson()));
    }
    m_activeQuests.clear();
    for (const auto& q : cp.quests) {
        if (q) m_activeQuests.push_back(Quest::fromJson(q->toJson()));
    }
    m_completedQuestIds = cp.completedQuestIds;
    m_keyItems = cp.keyItems;
    m_bestiary = cp.bestiary;
    if (cp.world.isGenerated()) m_world = DungeonWorld::fromJson(cp.world.toJson());
    m_campaignCompleted = cp.campaignCompleted;
    m_deepestFloorReached = cp.deepestFloorReached;
    m_totalTurns = cp.totalTurns;
    m_lastSessionSeed = cp.lastSessionSeed;
    m_sessionRngDrawCount = cp.sessionRngDrawCount;
    m_hasActiveSaveSession = cp.activeSaveSession;
    m_recoveryPending = cp.recoveryPending;
    if (cp.globalRngSeed != 0U) {
        SessionRng::global() = SessionRng(cp.globalRngSeed, cp.globalRngDrawCount);
    }
}

void Party::validateState() const {
    if (m_gold < 0) throw std::runtime_error("gold cannot be negative");
    if (static_cast<int>(m_members.size()) > MAX_MEMBERS) throw std::runtime_error("too many party members");
    if (!m_world.isGenerated()) throw std::runtime_error("dungeon world is not generated");
    m_world.validate();
}

PersistenceResult Party::saveToFile(const std::string& filePath) {
    if (m_recoveryPending) {
        return {PersistenceStatus::RecoveryPending, filePath,
                "Save blocked while recoveryPending write fence is active."};
    }
    try {
        validateState();
        m_lastSessionSeed = SessionRng::global().seed();
        m_sessionRngDrawCount = SessionRng::global().drawCount();

        nlohmann::json j;
        j["schemaVersion"] = SAVE_SCHEMA_VERSION;
        j["gold"] = m_gold;
        j["campaignCompleted"] = m_campaignCompleted;
        j["deepestFloorReached"] = m_deepestFloorReached;
        j["totalTurns"] = m_totalTurns;
        j["lastSessionSeed"] = m_lastSessionSeed;
        j["sessionRngDrawCount"] = m_sessionRngDrawCount;

        nlohmann::json members = nlohmann::json::array();
        for (const auto& m : m_members) members.push_back(m->toJson());
        j["members"] = std::move(members);

        nlohmann::json inv = nlohmann::json::array();
        for (const auto& item : m_inventory) inv.push_back(item->toJson());
        j["inventory"] = std::move(inv);

        nlohmann::json quests = nlohmann::json::array();
        for (const auto& q : m_activeQuests) quests.push_back(q->toJson());
        j["activeQuests"] = std::move(quests);

        std::vector<std::string> sortedCompleted(m_completedQuestIds.begin(), m_completedQuestIds.end());
        std::sort(sortedCompleted.begin(), sortedCompleted.end());
        j["completedQuests"] = sortedCompleted;

        std::vector<std::string> sortedKeys(m_keyItems.begin(), m_keyItems.end());
        std::sort(sortedKeys.begin(), sortedKeys.end());
        j["keyItems"] = sortedKeys;

        j["bestiary"] = m_bestiary.toJson();
        j["dungeonWorld"] = m_world.toJson();

        const auto result = Persistence::atomicWriteText(filePath, j.dump(2));
        if (result) m_hasActiveSaveSession = true;
        return result;
    } catch (const std::exception& e) {
        return {PersistenceStatus::IoError, filePath, e.what()};
    }
}

PersistenceResult Party::loadFromFile(const std::string& filePath) {
    auto loadCandidate = [this](const std::filesystem::path& candidate,
                                PersistenceStatus successStatus) -> PersistenceResult {
        if (!std::filesystem::exists(candidate)) {
            return {PersistenceStatus::NotFound, candidate, "Save file not found."};
        }
        std::error_code ec;
        const auto sz = std::filesystem::file_size(candidate, ec);
        if (ec) return {PersistenceStatus::IoError, candidate, ec.message()};
        if (sz > 4U * 1024U * 1024U) {
            return {PersistenceStatus::Corrupt, candidate, "Save file exceeds 4 MiB limit."};
        }

        std::ifstream file(candidate, std::ios::binary);
        if (!file.is_open()) {
            return {PersistenceStatus::IoError, candidate, "Unable to open save file."};
        }

        try {
            nlohmann::json j;
            file >> j;
            if (!j.is_object()) throw std::runtime_error("save root must be object");
            const int schema = j.value("schemaVersion", 0);
            if (schema < SAVE_SCHEMA_VERSION || schema > SAVE_SCHEMA_VERSION) {
                return {PersistenceStatus::UnsupportedVersion, candidate,
                        "Incompatible save schema v" + std::to_string(schema) + " (requires v5)."};
            }

            Party loaded;
            loaded.m_members.clear();
            loaded.m_inventory.clear();
            loaded.m_activeQuests.clear();
            loaded.m_completedQuestIds.clear();
            loaded.m_keyItems.clear();

            loaded.m_gold = j.at("gold").get<int>();
            loaded.m_campaignCompleted = j.value("campaignCompleted", false);
            loaded.m_deepestFloorReached = std::clamp(j.value("deepestFloorReached", 1), 1, DungeonWorld::FLOOR_COUNT);
            loaded.m_totalTurns = std::max(0, j.value("totalTurns", 0));
            loaded.m_lastSessionSeed = j.at("lastSessionSeed").get<std::uint32_t>();
            loaded.m_sessionRngDrawCount = j.at("sessionRngDrawCount").get<std::uint64_t>();

            for (const auto& mj : j.at("members")) {
                loaded.m_members.push_back(Character::fromJson(mj));
            }
            for (const auto& ij : j.at("inventory")) {
                loaded.m_inventory.push_back(ItemFactory::createItemFromJson(ij));
            }
            for (const auto& qj : j.at("activeQuests")) {
                loaded.m_activeQuests.push_back(Quest::fromJson(qj));
            }
            for (const auto& cid : j.at("completedQuests")) {
                loaded.m_completedQuestIds.insert(cid.get<std::string>());
            }
            for (const auto& ki : j.at("keyItems")) {
                loaded.m_keyItems.insert(ki.get<std::string>());
            }
            loaded.m_bestiary = Bestiary::fromJson(j.value("bestiary", nlohmann::json::object()));
            loaded.m_world = DungeonWorld::fromJson(j.at("dungeonWorld"));
            loaded.validateState();

            m_members = std::move(loaded.m_members);
            m_inventory = std::move(loaded.m_inventory);
            m_activeQuests = std::move(loaded.m_activeQuests);
            m_completedQuestIds = std::move(loaded.m_completedQuestIds);
            m_keyItems = std::move(loaded.m_keyItems);
            m_bestiary = std::move(loaded.m_bestiary);
            m_world = std::move(loaded.m_world);
            m_gold = loaded.m_gold;
            m_campaignCompleted = loaded.m_campaignCompleted;
            m_deepestFloorReached = loaded.m_deepestFloorReached;
            m_totalTurns = loaded.m_totalTurns;
            m_lastSessionSeed = loaded.m_lastSessionSeed;
            m_sessionRngDrawCount = loaded.m_sessionRngDrawCount;
            m_hasActiveSaveSession = true;
            m_recoveryPending = false;
            return {successStatus, candidate, {}};
        } catch (const std::exception& e) {
            return {PersistenceStatus::Corrupt, candidate, e.what()};
        }
    };

    const std::filesystem::path primary(filePath);
    const std::filesystem::path backup(filePath + ".bak");

    auto primaryRes = loadCandidate(primary, PersistenceStatus::Loaded);
    if (primaryRes) return primaryRes;

    // 클린 브레이크 정책: 구버전(v1~v4) 세이브는 원본 손실 없이 격리하고 UnsupportedVersion 반환
    if (primaryRes.status == PersistenceStatus::UnsupportedVersion) {
        std::filesystem::path qPath;
        const auto qRes = Persistence::quarantine(primary, qPath);
        if (qRes.status == PersistenceStatus::IoError) return qRes;
        if (std::filesystem::exists(backup)) {
            std::filesystem::path bqPath;
            Persistence::quarantine(backup, bqPath);
        }
        return {PersistenceStatus::UnsupportedVersion, qPath, primaryRes.message};
    }

    auto tryBackup = [&]() -> PersistenceResult {
        if (!std::filesystem::exists(backup)) return {PersistenceStatus::NotFound, backup, "No backup."};
        auto bakRes = loadCandidate(backup, PersistenceStatus::RecoveredFromBackup);
        if (bakRes) {
            std::ifstream bf(backup, std::ios::binary);
            const std::string bytes{std::istreambuf_iterator<char>(bf), std::istreambuf_iterator<char>()};
            Persistence::atomicWriteText(primary, bytes);
            return bakRes;
        }
        if (bakRes.status == PersistenceStatus::Corrupt || bakRes.status == PersistenceStatus::UnsupportedVersion) {
            std::filesystem::path bq;
            const auto q = Persistence::quarantine(backup, bq);
            if (q.status == PersistenceStatus::IoError) return q;
            return {bakRes.status, bq, bakRes.message};
        }
        return bakRes;
    };

    if (primaryRes.status == PersistenceStatus::NotFound) {
        if (std::filesystem::exists(backup)) return tryBackup();
        return primaryRes;
    }
    if (primaryRes.status != PersistenceStatus::Corrupt) return primaryRes;

    std::filesystem::path qPath;
    const auto qRes = Persistence::quarantine(primary, qPath);
    if (qRes.status == PersistenceStatus::IoError) return qRes;

    if (std::filesystem::exists(backup)) {
        auto bakRes = tryBackup();
        if (bakRes) return bakRes;
    }
    return {PersistenceStatus::Corrupt, qPath, primaryRes.message};
}

} // namespace crawl
