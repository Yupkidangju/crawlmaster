#include "controller/BestiaryState.hpp"
#include "controller/CharacterCreationState.hpp"
#include "controller/CharacterInfoState.hpp"
#include "controller/CombatState.hpp"
#include "controller/DungeonState.hpp"
#include "controller/SettingsState.hpp"
#include "controller/TitleState.hpp"
#include "controller/TownState.hpp"
#include "controller/VictoryState.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

#define CHECK(expr)                                                                  \
    do {                                                                             \
        if (!(expr)) {                                                               \
            std::cerr << "[FAIL] UiStateRaster: " #expr " (" << __LINE__ << ")\n"; \
            return 1;                                                                \
        }                                                                            \
    } while (false)

namespace {

struct ImageStats {
    double meanR = 0.0;
    double meanG = 0.0;
    double meanB = 0.0;
    double stdDev = 0.0;
};

ImageStats captureAndVerify(crawl::Game& game, const std::filesystem::path& outFile) {
    auto& win = game.getWindow();
    win.clear(sf::Color(9, 9, 13));
    const auto sz = win.getSize();
    win.setView(game.computeLetterboxView(sz.x, sz.y));
    if (auto* st = game.getStates().getCurrentState()) {
        st->update(sf::seconds(0.05f));
        st->draw(win);
    }
    win.display();

    sf::Texture tex;
    tex.create(sz.x, sz.y);
    tex.update(win);
    const sf::Image img = tex.copyToImage();
    img.saveToFile(outFile.string());

    double sumR = 0.0, sumG = 0.0, sumB = 0.0, sumL = 0.0, sumL2 = 0.0;
    std::size_t n = 0;
    for (unsigned int y = 0; y < img.getSize().y; y += 4) {
        for (unsigned int x = 0; x < img.getSize().x; x += 4) {
            const auto c = img.getPixel(x, y);
            sumR += c.r;
            sumG += c.g;
            sumB += c.b;
            const double l = 0.299 * c.r + 0.587 * c.g + 0.114 * c.b;
            sumL += l;
            sumL2 += l * l;
            ++n;
        }
    }
    const double dn = static_cast<double>(std::max<std::size_t>(1, n));
    const double meanL = sumL / dn;
    const double var = std::max(0.0, sumL2 / dn - meanL * meanL);
    return {sumR / dn, sumG / dn, sumB / dn, std::sqrt(var)};
}

} // namespace

int main(int argc, char** argv) {
    using namespace crawl;
    const std::filesystem::path outDir =
        argc > 1 ? std::filesystem::path(argv[1])
                 : (std::filesystem::temp_directory_path() / "crawlmaster_ui_raster");
    std::filesystem::create_directories(outDir);

    const auto tempData = outDir / "_tmp_data";
    std::filesystem::create_directories(tempData);
    Party::setDefaultSavePath((tempData / "save.json").string());
    LocalizationManager::setDefaultConfigPath((tempData / "config.json").string());
    SessionRng::global() = SessionRng(20261122U, 0);

    auto& lm = LocalizationManager::getInstance();
    lm.setLanguage(Language::KO);
    lm.setTextScale(100);
    lm.setHighContrast(false);
    lm.setResolution(1280, 720);
    lm.setFullscreen(false);

    Game game(false);
    game.getParty().startNewGame();
    game.getParty().markFloorReached(5);
    game.getParty().getMember(0)->addXp(600);
    game.getParty().getBestiary().recordDefeat("mon_crypt_warden");
    game.getParty().getBestiary().recordDefeat("mon_elder_wyrm");

    // 1. Title
    game.getStates().replaceAll(std::make_unique<TitleState>(game));
    const auto sTitle = captureAndVerify(game, outDir / "01_title_ko.png");
    CHECK(sTitle.stdDev > 15.0);

    // 2. Town Hub & Blacksmith
    game.getStates().replaceAll(std::make_unique<TownState>(game));
    const auto sTown = captureAndVerify(game, outDir / "02_town_hub.png");
    CHECK(sTown.stdDev > 15.0);

    sf::Event ev{};
    ev.type = sf::Event::KeyPressed;
    ev.key.code = sf::Keyboard::Num5;
    game.getStates().getCurrentState()->handleInput(ev);
    const auto sForge = captureAndVerify(game, outDir / "03_town_blacksmith.png");
    CHECK(sForge.stdDev > 15.0);

    // 3. Character Creation
    game.getStates().pushState(std::make_unique<CharacterCreationState>(game));
    const auto sCreate = captureAndVerify(game, outDir / "04_char_creation.png");
    CHECK(sCreate.stdDev > 15.0);

    // 4. All 5 Dungeon Biomes 3D Views!
    ImageStats biomeStats[5];
    for (int f = 1; f <= 5; ++f) {
        game.getStates().replaceAll(std::make_unique<DungeonState>(game, f));
        biomeStats[f - 1] = captureAndVerify(game, outDir / ("0" + std::to_string(4 + f) + "_dungeon_" + std::to_string(f) + "f.png"));
        CHECK(biomeStats[f - 1].stdDev > 14.0);
    }

    // 5. Combat vs Final Boss (Elder Wyrm) on Floor 5 3D Sanctum!
    EncounterSpec bossSpec;
    bossSpec.tier = EncounterTier::SANCTUM;
    bossSpec.fixedMonsterId = "mon_elder_wyrm";
    bossSpec.bossBattle = true;
    bossSpec.campaignFinal = true;
    bossSpec.floorNumber = 5;
    game.getStates().pushState(std::make_unique<CombatState>(game, bossSpec));
    const auto sBoss = captureAndVerify(game, outDir / "10_combat_elder_wyrm.png");
    CHECK(sBoss.stdDev > 16.0);

    // 6. Combat Floor 1 group encounter
    EncounterSpec f1Spec;
    f1Spec.tier = EncounterTier::EARLY;
    f1Spec.floorNumber = 1;
    game.getStates().replaceAll(std::make_unique<DungeonState>(game, 1));
    game.getStates().pushState(std::make_unique<CombatState>(game, f1Spec));
    const auto sCombat1 = captureAndVerify(game, outDir / "11_combat_floor1.png");
    CHECK(sCombat1.stdDev > 16.0);

    // 7. Character Sheet & Talents + Bestiary + Settings (EN + High Contrast) + Victory
    game.getStates().pushState(std::make_unique<CharacterInfoState>(game, 0));
    ev.key.code = sf::Keyboard::Tab;
    game.getStates().getCurrentState()->handleInput(ev);
    const auto sInfo = captureAndVerify(game, outDir / "12_char_info_talents.png");
    CHECK(sInfo.stdDev > 14.0);

    game.getStates().pushState(std::make_unique<BestiaryState>(game));
    const auto sBest = captureAndVerify(game, outDir / "13_bestiary.png");
    CHECK(sBest.stdDev > 14.0);

    lm.setLanguage(Language::EN);
    lm.setTextScale(150);
    lm.setHighContrast(true);
    game.getStates().pushState(std::make_unique<SettingsState>(game));
    const auto sSet = captureAndVerify(game, outDir / "14_settings_en_hc.png");
    CHECK(sSet.stdDev > 14.0);

    lm.setLanguage(Language::JA);
    lm.setTextScale(100);
    lm.setHighContrast(false);
    game.getStates().replaceAll(std::make_unique<VictoryState>(game));
    const auto sVic = captureAndVerify(game, outDir / "15_victory_ja.png");
    CHECK(sVic.stdDev > 14.0);

    lm.setLanguage(Language::KO);
    game.completeShutdown();
    std::filesystem::remove_all(tempData);
    std::cout << "[PASS] UiStateRasterTests passed (15 AAA visual frames captured and verified in "
              << outDir << ").\n";
    return 0;
}
