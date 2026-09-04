#include "controller/TitleState.hpp"
#include "controller/TownState.hpp"
#include "controller/ShutdownState.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "model/Party.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {

std::string readBytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

sf::Event key(sf::Keyboard::Key code) {
    sf::Event event{};
    event.type = sf::Event::KeyPressed;
    event.key.code = code;
    return event;
}

int writeCheckpoint(const std::filesystem::path& savePath,
                    const std::filesystem::path& expectedPath) {
    std::filesystem::remove(savePath);
    std::filesystem::remove(savePath.string() + ".bak");
    std::filesystem::remove(expectedPath);
    crawl::Party::setDefaultSavePath(savePath.string());
    crawl::SessionRng::reseedGlobal(0x6A09E667U);
    for (int index = 0; index < 29; ++index) {
        static_cast<void>(crawl::SessionRng::global().rollRange(1, 997));
    }
    crawl::Party party;
    if (!party.startNewGame()) return 1;

    std::ofstream expected(expectedPath);
    if (!expected) return 1;
    for (int index = 0; index < 16; ++index) {
        expected << crawl::SessionRng::global().rollRange(1, 1'000'000) << '\n';
    }
    return expected ? 0 : 1;
}

int continueCheckpoint(const std::filesystem::path& savePath,
                       const std::filesystem::path& expectedPath,
                       const std::filesystem::path& configPath) {
    std::ifstream expectedInput(expectedPath);
    std::vector<int> expected;
    int value = 0;
    while (expectedInput >> value) expected.push_back(value);
    if (expected.size() != 16U) return 1;

    crawl::Party::setDefaultSavePath(savePath.string());
    crawl::LocalizationManager::setDefaultConfigPath(configPath.string());
    crawl::SessionRng::reseedGlobal(7U);
    crawl::Game game(true);
    game.getStates().replaceAll(std::make_unique<crawl::TitleState>(game));
    auto* title = dynamic_cast<crawl::TitleState*>(game.getStates().getCurrentState());
    if (!title) return 1;
    title->handleInput(key(sf::Keyboard::Down));
    title->handleInput(key(sf::Keyboard::Enter));
    if (!dynamic_cast<crawl::TownState*>(game.getStates().getCurrentState())) return 1;

    for (const int expectedValue : expected) {
        if (crawl::SessionRng::global().rollRange(1, 1'000'000) != expectedValue) return 1;
    }

#ifndef _WIN32
    const auto probeDirectory = savePath.parent_path() / "recovery-fence-process-probe";
    std::filesystem::create_directories(probeDirectory);
    const auto targetPath = probeDirectory / "target.json";
    const auto linkPath = probeDirectory / "load-io.json";
    crawl::Party::setDefaultSavePath(targetPath.string());
    crawl::Game recoveryGame(true);
    if (!recoveryGame.getParty().startNewGame()) return 1;
    const std::string targetBefore = readBytes(targetPath);
    std::error_code error;
    std::filesystem::create_symlink(targetPath, linkPath, error);
    if (error) return 1;
    crawl::Party::setDefaultSavePath(linkPath.string());
    recoveryGame.getStates().replaceAll(std::make_unique<crawl::TitleState>(recoveryGame));
    auto* recoveryTitle = dynamic_cast<crawl::TitleState*>(
        recoveryGame.getStates().getCurrentState());
    if (!recoveryTitle) return 1;
    recoveryTitle->handleInput(key(sf::Keyboard::Down));
    recoveryTitle->handleInput(key(sf::Keyboard::Enter));
    if (!recoveryGame.getParty().isRecoveryPending()) return 1;
    recoveryGame.requestShutdown();
    auto* shutdown = dynamic_cast<crawl::ShutdownState*>(
        recoveryGame.getStates().getCurrentState());
    if (!shutdown || recoveryGame.isShutdownApproved()) return 1;
    if (readBytes(targetPath) != targetBefore) return 1;
    shutdown->handleInput(key(sf::Keyboard::Enter));
    if (recoveryGame.isShutdownApproved() ||
        !recoveryGame.getParty().isRecoveryPending() ||
        readBytes(targetPath) != targetBefore) return 1;
    shutdown->handleInput(key(sf::Keyboard::Escape));
    if (!recoveryGame.isShutdownApproved() || readBytes(targetPath) != targetBefore) return 1;
    std::filesystem::remove_all(probeDirectory);
#endif

    std::cout << "Independent-process RNG checkpoint replay passed.\n";
    return 0;
}

int migrateSeedlessLegacy(const std::string& mode,
                          const std::filesystem::path& legacyPath,
                          const std::filesystem::path& outputPath) {
    crawl::SessionRng::reseedGlobal(mode == "migrate-a" ? 1U : 0xDEADBEEFU);
    crawl::Party party;
    if (!party.loadFromFile(legacyPath.string())) return 1;
    nlohmann::json evidence = {
        {"seed", party.getLastSessionSeed()},
        {"rngSeed", crawl::SessionRng::global().seed()},
        {"rngDrawCount", crawl::SessionRng::global().drawCount()},
        {"world", party.getWorld().toJson()},
    };
    std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
    output << evidence.dump(4);
    return output ? 0 : 1;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 5) return 2;
    const std::string mode = argv[1];
    if (mode == "write") return writeCheckpoint(argv[2], argv[3]);
    if (mode == "continue") return continueCheckpoint(argv[2], argv[3], argv[4]);
    if (mode == "migrate-a" || mode == "migrate-b") {
        return migrateSeedlessLegacy(mode, argv[2], argv[3]);
    }
    return 2;
}
