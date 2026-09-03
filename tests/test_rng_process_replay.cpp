#include "controller/TitleState.hpp"
#include "controller/TownState.hpp"
#include "core/Game.hpp"
#include "core/LocalizationManager.hpp"
#include "core/SessionRng.hpp"
#include "model/Party.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

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
    std::cout << "Independent-process RNG checkpoint replay passed.\n";
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 5) return 2;
    const std::string mode = argv[1];
    if (mode == "write") return writeCheckpoint(argv[2], argv[3]);
    if (mode == "continue") return continueCheckpoint(argv[2], argv[3], argv[4]);
    return 2;
}
