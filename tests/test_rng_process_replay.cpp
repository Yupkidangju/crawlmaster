#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "model/Party.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#define CHECK(expr)                                                                     \
    do {                                                                                \
        if (!(expr)) {                                                                  \
            std::cerr << "[FAIL] RngProcessReplay: " #expr " (" << __LINE__ << ")\n"; \
            return 1;                                                                   \
        }                                                                               \
    } while (false)

int main(int argc, char** argv) {
    using namespace crawl;
    if (argc < 5) {
        std::cerr << "Usage: RngProcessReplayTests <write|continue|legacy-quarantine> <p1> <p2> <p3>\n";
        return 2;
    }

    const std::string mode = argv[1];
    const std::string arg1 = argv[2];
    const std::string arg2 = argv[3];
    const std::string arg3 = argv[4];

    if (mode == "write") {
        Party::setDefaultSavePath(arg1);
        LocalizationManager::setDefaultConfigPath(arg3);
        SessionRng::global() = SessionRng(55667788U, 0);
        // 소비 카운트 진행
        for (int i = 0; i < 17; ++i) (void)SessionRng::global().rollDie(20);

        Party party;
        CHECK(party.startNewGame(arg1).succeeded());

        // 저장 직후 이어질 10개 난수를 기대값으로 기록
        SessionRng mirror(party.getLastSessionSeed(), party.getSessionRngDrawCount());
        std::ofstream out(arg2, std::ios::binary | std::ios::trunc);
        for (int i = 0; i < 10; ++i) {
            out << mirror.rollDie(1000) << '\n';
        }
        return 0;
    }

    if (mode == "continue") {
        Party::setDefaultSavePath(arg1);
        LocalizationManager::setDefaultConfigPath(arg3);
        Party loaded;
        CHECK(loaded.loadFromFile(arg1).succeeded());
        SessionRng::global() = SessionRng(loaded.getLastSessionSeed(), loaded.getSessionRngDrawCount());

        std::ifstream in(arg2, std::ios::binary);
        CHECK(in.is_open());
        for (int i = 0; i < 10; ++i) {
            int expected = -1;
            CHECK(static_cast<bool>(in >> expected));
            const int actual = SessionRng::global().rollDie(1000);
            CHECK(actual == expected);
        }
        std::cout << "[PASS] RngReplayCheckpointContinue matched 10/10 independent-process draws.\n";
        return 0;
    }

    if (mode == "legacy-quarantine") {
        // v1 구버전 세이브 복사 후 로드 -> UnsupportedVersion + 원본 보존 격리 확인
        std::filesystem::copy_file(arg1, arg2, std::filesystem::copy_options::overwrite_existing);
        Party p;
        const auto res = p.loadFromFile(arg2);
        CHECK(res.status == PersistenceStatus::UnsupportedVersion);
        CHECK(!std::filesystem::exists(arg2));          // 원본 경로에서 안전 격리 이동됨
        CHECK(std::filesystem::exists(res.path));       // 격리된 사본은 데이터 손실 없이 존재함
        std::cout << "[PASS] LegacySaveQuarantineTest cleanly quarantined v1 save to " << res.path << '\n';
        return 0;
    }

    return 2;
}
