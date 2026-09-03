// [v0.1.0] main.cpp 신규 작성
// Crawlmaster 게임의 진입점. Game 클래스 객체를 생성하고 run 루프를 구동한다.

#include "core/Game.hpp"
#include "core/ResourceLocator.hpp"

#include <SFML/Graphics/Font.hpp>
#include <array>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <map>
#include <set>

namespace {

int verifyResources() {
    const auto assets = crawl::ResourceLocator::assetDirectory();
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

    const std::array fontFiles = {"neodgm.ttf", "NotoSansCJK-Regular.ttc", "UbuntuMono[wght].ttf"};
    for (const char* filename : fontFiles) {
        const auto path = assets / "fonts" / filename;
        sf::Font font;
        if (!font.loadFromFile(path.string())) {
            std::cerr << "[Resource Error] Missing or invalid font: " << path << '\n';
            return 2;
        }
    }
    std::cout << "[Resource OK] " << assets << '\n';
    return 0;
}

} // namespace

// 메인 함수: C++ 표준 진입점
int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--verify-resources") {
        return verifyResources();
    }
    // Game 인스턴스 생성 및 실행
    crawl::Game game;
    game.run();
    
    return 0;
}
