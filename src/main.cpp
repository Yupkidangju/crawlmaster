#include "core/Game.hpp"
#include "core/ResourceLocator.hpp"
#include "model/MonsterFactory.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Image.hpp>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <vector>

namespace {

bool verifyNonSilentWav(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    std::vector<char> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (bytes.size() < 48 || bytes[0] != 'R' || bytes[1] != 'I' || bytes[2] != 'F' || bytes[3] != 'F') {
        return false;
    }
    double energy = 0.0;
    std::size_t count = 0;
    for (std::size_t i = 44; i + 1 < bytes.size(); i += 2) {
        const auto lo = static_cast<unsigned char>(bytes[i]);
        const auto hi = static_cast<unsigned char>(bytes[i + 1]);
        const auto sample = static_cast<std::int16_t>(lo | (hi << 8));
        const double n = static_cast<double>(sample) / 32768.0;
        energy += n * n;
        ++count;
    }
    if (count == 0) return false;
    const double rms = std::sqrt(energy / static_cast<double>(count));
    return rms > 0.0005;
}

int verifyResources() {
    const auto assets = crawl::ResourceLocator::assetDirectory();

    // 1. 5개국어 JSON 키 & 플레이스홀더 동등성 검증
    const std::array localeFiles = {"ko.json", "en.json", "ja.json", "zh_tw.json", "zh_cn.json"};
    std::set<std::string> referenceKeys;
    std::map<std::string, std::set<std::string>> referencePlaceholders;
    for (const char* filename : localeFiles) {
        const auto path = assets / "lang" / filename;
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "[Resource Error] Missing locale: " << path << '\n';
            return 2;
        }
        try {
            nlohmann::json content;
            file >> content;
            if (!content.is_object()) throw std::runtime_error("locale root is not an object");
            std::set<std::string> keys;
            for (const auto& [key, value] : content.items()) {
                if (!value.is_string()) throw std::runtime_error("locale value is not a string");
                keys.insert(key);
                std::set<std::string> placeholders;
                const std::string text = value.get<std::string>();
                std::size_t position = 0;
                while ((position = text.find('{', position)) != std::string::npos) {
                    const auto end = text.find('}', position + 1);
                    if (end == std::string::npos) throw std::runtime_error("unclosed placeholder");
                    placeholders.insert(text.substr(position + 1, end - position - 1));
                    position = end + 1;
                }
                if (referenceKeys.empty()) {
                    referencePlaceholders[key] = std::move(placeholders);
                } else if (referencePlaceholders[key] != placeholders) {
                    throw std::runtime_error("placeholder set differs for key " + key);
                }
            }
            if (referenceKeys.empty()) {
                referenceKeys = std::move(keys);
            } else if (keys != referenceKeys) {
                throw std::runtime_error("locale key set differs from ko.json");
            }
        } catch (const std::exception& exception) {
            std::cerr << "[Resource Error] Invalid locale " << path << ": " << exception.what() << '\n';
            return 2;
        }
    }

    // 2. 폰트 3종 검증
    const std::array fontFiles = {"neodgm.ttf", "NotoSansCJK-Regular.ttc", "UbuntuMono[wght].ttf"};
    for (const char* filename : fontFiles) {
        const auto path = assets / "fonts" / filename;
        sf::Font font;
        if (!font.loadFromFile(path.string())) {
            std::cerr << "[Resource Error] Missing or invalid font: " << path << '\n';
            return 2;
        }
    }

    // 3. 핵심 3D 텍스처 · 노멀맵 · 배경 · 초상화 · 몬스터 24종 · UI 이미지 디코드 검증
    auto checkImage = [&](const std::string& rel) -> bool {
        sf::Image img;
        const auto p = assets / rel;
        if (!img.loadFromFile(p.string()) || img.getSize().x == 0 || img.getSize().y == 0) {
            std::cerr << "[Resource Error] Missing or invalid texture: " << p << '\n';
            return false;
        }
        return true;
    };

    const std::array requiredTextures = {
        "textures/backgrounds/title_bg.png", "textures/backgrounds/town_bg.png",
        "textures/portraits/hero_warrior.png", "textures/portraits/hero_mage.png",
        "textures/portraits/hero_rogue.png", "textures/portraits/hero_cleric.png",
        "textures/biomes/wall_catacombs.png", "textures/biomes/wall_catacombs_n.png",
        "textures/biomes/floor_catacombs.png", "textures/biomes/floor_catacombs_n.png",
        "textures/biomes/ceiling_catacombs.png",
        "textures/biomes/wall_cistern.png", "textures/biomes/wall_cistern_n.png",
        "textures/biomes/floor_cistern.png", "textures/biomes/floor_cistern_n.png",
        "textures/biomes/ceiling_cistern.png",
        "textures/biomes/wall_ossuary.png", "textures/biomes/wall_ossuary_n.png",
        "textures/biomes/floor_ossuary.png", "textures/biomes/floor_ossuary_n.png",
        "textures/biomes/ceiling_ossuary.png",
        "textures/biomes/wall_foundry.png", "textures/biomes/wall_foundry_n.png",
        "textures/biomes/floor_foundry.png", "textures/biomes/floor_foundry_n.png",
        "textures/biomes/ceiling_foundry.png",
        "textures/biomes/wall_sanctum.png", "textures/biomes/wall_sanctum_n.png",
        "textures/biomes/floor_sanctum.png", "textures/biomes/floor_sanctum_n.png",
        "textures/biomes/ceiling_sanctum.png",
        "textures/biomes/door_wood.png", "textures/biomes/door_locked.png",
        "textures/biomes/boss_gate.png", "textures/biomes/stairs_up.png",
        "textures/biomes/stairs_down.png", "textures/biomes/secret_wall.png",
        "textures/ui/panel_9slice.png", "textures/ui/panel_gold_9slice.png",
        "textures/ui/particle_glow.png", "textures/ui/particle_slash.png",
        "textures/ui/vignette.png"
    };
    for (const char* rel : requiredTextures) {
        if (!checkImage(rel)) return 2;
    }
    for (const auto& monId : crawl::MonsterFactory::getRegisteredIds()) {
        if (!checkImage("textures/monsters/" + monId + ".png")) return 2;
    }

    // 4. GLSL 셰이더 3종 존재 검증
    for (const char* sh : {"shaders/dungeon_lit.vert", "shaders/dungeon_lit.frag", "shaders/post_bloom.frag"}) {
        if (!std::filesystem::is_regular_file(assets / sh)) {
            std::cerr << "[Resource Error] Missing shader: " << (assets / sh) << '\n';
            return 2;
        }
    }

    // 5. 오디오 34종 무음 아님(RMS) 검증
    const std::array audioFiles = {
        "bgm_title.wav", "bgm_town.wav", "bgm_dungeon.wav", "bgm_combat.wav", "bgm_boss.wav",
        "amb_floor1.wav", "amb_floor2.wav", "amb_floor3.wav", "amb_floor4.wav", "amb_floor5.wav",
        "sfx_step.wav", "sfx_turn.wav", "sfx_bump.wav", "sfx_door.wav", "sfx_chest.wav",
        "sfx_trap.wav", "sfx_lever.wav", "sfx_sanctuary.wav", "sfx_sword_hit.wav",
        "sfx_crit.wav", "sfx_miss.wav", "sfx_shield_block.wav", "sfx_break.wav",
        "sfx_fire_spell.wav", "sfx_ice_spell.wav", "sfx_lightning_spell.wav",
        "sfx_heal.wav", "sfx_poison.wav", "sfx_ui_hover.wav", "sfx_ui_click.wav",
        "sfx_ui_error.wav", "sfx_levelup.wav", "sfx_victory.wav", "sfx_defeat.wav"
    };
    for (const char* af : audioFiles) {
        const auto p = assets / "audio" / af;
        if (!verifyNonSilentWav(p)) {
            std::cerr << "[Resource Error] Missing or silent audio: " << p << '\n';
            return 2;
        }
    }

    std::cout << "[Resource OK] " << assets << " (5 locales, 3 fonts, 81+ textures, 3 shaders, 34 audio files)\n";
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--verify-resources") {
        return verifyResources();
    }
    crawl::Game game;
    game.run();
    return 0;
}
