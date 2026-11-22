#include "core/AudioManager.hpp"
#include "core/LocalizationManager.hpp"
#include "core/ResourceLocator.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

#define CHECK(expr)                                                                 \
    do {                                                                            \
        if (!(expr)) {                                                              \
            std::cerr << "[FAIL] AudioContract: " #expr " (" << __LINE__ << ")\n"; \
            return 1;                                                               \
        }                                                                           \
    } while (false)

int main() {
    using namespace crawl;

    // 1. 모든 34개 WAV 실음원이 유효한 PCM 헤더와 비무음 에너지를 갖는지 검증
    const std::array files = {
        "bgm_title.wav", "bgm_town.wav", "bgm_dungeon.wav", "bgm_combat.wav", "bgm_boss.wav",
        "amb_floor1.wav", "amb_floor2.wav", "amb_floor3.wav", "amb_floor4.wav", "amb_floor5.wav",
        "sfx_step.wav", "sfx_turn.wav", "sfx_bump.wav", "sfx_door.wav", "sfx_chest.wav",
        "sfx_trap.wav", "sfx_lever.wav", "sfx_sanctuary.wav", "sfx_sword_hit.wav",
        "sfx_crit.wav", "sfx_miss.wav", "sfx_shield_block.wav", "sfx_break.wav",
        "sfx_fire_spell.wav", "sfx_ice_spell.wav", "sfx_lightning_spell.wav",
        "sfx_heal.wav", "sfx_poison.wav", "sfx_ui_hover.wav", "sfx_ui_click.wav",
        "sfx_ui_error.wav", "sfx_levelup.wav", "sfx_victory.wav", "sfx_defeat.wav"
    };

    for (const char* name : files) {
        const auto p = ResourceLocator::assetPath(std::string("audio/") + name);
        std::ifstream f(p, std::ios::binary);
        CHECK(f.is_open());
        std::vector<char> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        CHECK(bytes.size() > 100);
        CHECK(bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == 'F');
        double sumSq = 0.0;
        std::size_t n = 0;
        for (std::size_t i = 44; i + 1 < bytes.size(); i += 2) {
            const auto lo = static_cast<unsigned char>(bytes[i]);
            const auto hi = static_cast<unsigned char>(bytes[i + 1]);
            const auto s = static_cast<std::int16_t>(lo | (hi << 8));
            const double v = static_cast<double>(s) / 32768.0;
            sumSq += v * v;
            ++n;
        }
        CHECK(n > 0);
        const double rms = std::sqrt(sumSq / static_cast<double>(n));
        CHECK(rms > 0.001);
    }

    // 2. 헤드리스 안전 AudioManager 수명주기 검증
    auto& am = AudioManager::getInstance();
    am.initialize(true);
    CHECK(am.isInitialized());
    CHECK(am.isHeadlessFallback());
    am.applyVolumeConfig(80, 65, 85, false);
    am.playBgm("bgm_dungeon");
    CHECK(am.currentBgm() == "bgm_dungeon");
    am.playAmbience(3);
    CHECK(am.currentAmbience() == "amb_floor3");
    am.playSfx("sfx_sword_hit");
    am.stopAmbience();
    am.stopBgm();
    am.shutdown();

    std::cout << "[PASS] AudioContractTests passed (34 non-silent WAVs + headless AudioManager).\n";
    return 0;
}
