#include "core/LocalizationManager.hpp"
#include "core/ResourceLocator.hpp"

#include <SFML/Graphics.hpp>
#include <array>
#include <filesystem>
#include <iostream>
#include <string>

#define CHECK(expr)                                                               \
    do {                                                                          \
        if (!(expr)) {                                                            \
            std::cerr << "[FAIL] FontRaster: " #expr " (" << __LINE__ << ")\n"; \
            return 1;                                                             \
        }                                                                         \
    } while (false)

int main(int argc, char** argv) {
    using namespace crawl;
    const std::filesystem::path outDir =
        argc > 1 ? std::filesystem::path(argv[1]) : (std::filesystem::temp_directory_path() / "crawlmaster_font_raster");
    std::filesystem::create_directories(outDir);

    sf::Font cjkFont;
    CHECK(cjkFont.loadFromFile(ResourceLocator::assetPath("fonts/NotoSansCJK-Regular.ttc").string()));

    sf::RenderTexture rt;
    if (!rt.create(640, 220)) {
        std::cout << "[SKIP] Offscreen RenderTexture unavailable; font load verified.\n";
        return 0;
    }

    const std::array<std::pair<Language, const char*>, 5> samples = {{
        {Language::KO, "TITLE_SUBTITLE"},
        {Language::EN, "TITLE_SUBTITLE"},
        {Language::JA, "TITLE_SUBTITLE"},
        {Language::ZH_TW, "TITLE_SUBTITLE"},
        {Language::ZH_CN, "TITLE_SUBTITLE"}
    }};

    auto& lm = LocalizationManager::getInstance();
    for (const auto& [lang, key] : samples) {
        lm.setLanguage(lang);
        rt.clear(sf::Color(12, 13, 18));
        sf::Text txt;
        txt.setFont(cjkFont);
        txt.setCharacterSize(24);
        txt.setFillColor(sf::Color(237, 230, 214));
        const std::string s = lm.get(key);
        txt.setString(sf::String::fromUtf8(s.begin(), s.end()));
        txt.setPosition(20.0f, 80.0f);
        rt.draw(txt);
        rt.display();

        const sf::Image img = rt.getTexture().copyToImage();
        int litPixels = 0;
        for (unsigned int y = 0; y < img.getSize().y; ++y) {
            for (unsigned int x = 0; x < img.getSize().x; ++x) {
                const auto c = img.getPixel(x, y);
                if (c.r > 80 || c.g > 80 || c.b > 80) ++litPixels;
            }
        }
        CHECK(litPixels > 120);
        img.saveToFile((outDir / ("font_" + lm.getLanguageString() + ".png")).string());
    }

    lm.setLanguage(Language::KO);
    std::cout << "[PASS] FontRasterTests passed (5 scripts rendered and verified).\n";
    return 0;
}
