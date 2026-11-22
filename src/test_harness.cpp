#include "core/LocalizationManager.hpp"
#include "core/Persistence.hpp"
#include "core/SessionRng.hpp"
#include "model/Character.hpp"
#include "model/CombatRules.hpp"
#include "model/DungeonWorld.hpp"
#include "model/ItemFactory.hpp"
#include "model/MonsterFactory.hpp"
#include "model/Party.hpp"
#include "model/Quest.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#define CHECK(expr)                                                                     \
    do {                                                                                \
        if (!(expr)) {                                                                  \
            std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__ << " -> " #expr "\n"; \
            return 1;                                                                   \
        }                                                                               \
    } while (false)

int main(int /*argc*/, char** /*argv*/) {
    using namespace crawl;
    const auto tempDir = std::filesystem::temp_directory_path() / "crawlmaster_v5_harness";
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);

    const auto savePath = (tempDir / "save.json").string();
    const auto cfgPath = (tempDir / "config.json").string();
    Party::setDefaultSavePath(savePath);
    LocalizationManager::setDefaultConfigPath(cfgPath);

    // 1. 5층 영속 월드 & Save Schema v5 라운드트립
    SessionRng::global() = SessionRng(20261122U, 0);
    Party party;
    const auto newRes = party.startNewGame(savePath);
    CHECK(newRes.succeeded());
    CHECK(party.getWorld().getFloorCount() == 5);
    CHECK(party.getMemberCount() == 4);

    // 장비 강화·접사·진형·도감·퀘스트 변경 후 저장
    auto hero = party.getMember(0);
    CHECK(hero != nullptr);
    hero->setRow(CombatRow::BACK);
    hero->addXp(500); // Lv 3 도달 -> 전직 & 특성 포인트 해금
    CHECK(hero->getLevel() >= 3);
    CHECK(hero->canChooseSpecialization());
    CHECK(hero->chooseSpecialization(Specialization::VANGUARD));
    CHECK(hero->investTalent(TalentNode::VITALITY));

    auto wpn = hero->getEquippedItem(EquipSlot::WEAPON);
    CHECK(wpn != nullptr);
    CHECK(CombatRules::enhanceEquipment(*wpn));
    CombatRules::reforgeEquipment(*wpn, SessionRng::global());
    party.getBestiary().recordDefeat("mon_crypt_warden");
    party.markFloorReached(4);
    party.acceptQuest(Quest::createCanonical("qst_01_relic"));
    CHECK(party.saveToFile(savePath).succeeded());

    // 2. 새 인스턴스에 v5 로드 검증
    Party reloaded;
    const auto loadRes = reloaded.loadFromFile(savePath);
    CHECK(loadRes.status == PersistenceStatus::Loaded);
    CHECK(reloaded.getWorld().getFloorCount() == 5);
    CHECK(reloaded.getDeepestFloorReached() == 4);
    CHECK(reloaded.getMember(0)->getRow() == CombatRow::BACK);
    CHECK(reloaded.getMember(0)->getSpecialization() == Specialization::VANGUARD);
    CHECK(reloaded.getMember(0)->getTalentRank(TalentNode::VITALITY) == 1);
    CHECK(reloaded.getMember(0)->getEquippedItem(EquipSlot::WEAPON)->getUpgradeLevel() == 1);
    CHECK(reloaded.getBestiary().getEntry("mon_crypt_warden").defeated == 1);
    CHECK(reloaded.hasQuest("qst_01_relic"));

    // 3. 손상 파일 격리 + 백업 복구 검증
    CHECK(reloaded.saveToFile(savePath).succeeded()); // .bak 생성 보장
    {
        std::ofstream corrupt(savePath, std::ios::binary | std::ios::trunc);
        corrupt << "{ corrupt json !!!";
    }
    Party recovered;
    const auto recRes = recovered.loadFromFile(savePath);
    CHECK(recRes.status == PersistenceStatus::RecoveredFromBackup);
    CHECK(recovered.getDeepestFloorReached() == 4);

    // 4. recoveryPending write fence 검증
    recovered.markRecoveryPending();
    CHECK(recovered.saveToFile(savePath).status == PersistenceStatus::RecoveryPending);

    // 5. Config Schema v3 라운드트립 검증
    auto& lm = LocalizationManager::getInstance();
    lm.setLanguage(Language::EN);
    lm.setTextScale(125);
    lm.setHighContrast(true);
    lm.setResolution(1600, 900);
    lm.setFullscreen(false);
    lm.setMasterVolume(70);
    lm.setBgmVolume(50);
    lm.setSfxVolume(90);
    lm.setMuted(false);
    CHECK(lm.saveConfig(cfgPath).succeeded());

    lm.setLanguage(Language::KO);
    lm.setMasterVolume(10);
    CHECK(lm.loadConfig(cfgPath).succeeded());
    CHECK(lm.getLanguage() == Language::EN);
    CHECK(lm.getTextScale() == 125);
    CHECK(lm.getWindowWidth() == 1600);
    CHECK(lm.getMasterVolume() == 70);
    CHECK(lm.getBgmVolume() == 50);
    CHECK(lm.getSfxVolume() == 90);

    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);
    std::cout << "[PASS] TestHarness v1.0.0 AAA core checks passed.\n";
    return 0;
}
