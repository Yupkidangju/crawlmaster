#include "core/LocalizationManager.hpp"
#include "core/ResourceLocator.hpp"
#include "model/ItemFactory.hpp"
#include "model/MonsterFactory.hpp"
#include "model/Quest.hpp"
#include "model/SkillFactory.hpp"

#include <SFML/Graphics.hpp>

#include <array>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool condition, const char* expression, int line) {
    if (condition) return;
    ++failures;
    std::cerr << "[Failure] line " << line << ": " << expression << '\n';
}

#define CHECK(expression) check(static_cast<bool>(expression), #expression, __LINE__)

std::string suffix(crawl::Language language) {
    switch (language) {
        case crawl::Language::KO: return "ko";
        case crawl::Language::EN: return "en";
        case crawl::Language::JA: return "ja";
        case crawl::Language::ZH_TW: return "zh_tw";
        case crawl::Language::ZH_CN: return "zh_cn";
    }
    return "unknown";
}

bool isWhitespace(sf::Uint32 codepoint) {
    return codepoint == ' ' || codepoint == '\n' || codepoint == '\r' || codepoint == '\t';
}

bool sameGlyphRect(const sf::Glyph& left, const sf::Glyph& right) {
    return left.textureRect == right.textureRect;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) return 2;
    const std::filesystem::path outputDirectory(argv[1]);
    std::filesystem::create_directories(outputDirectory);

    const std::array<crawl::Language, 5> languages = {
        crawl::Language::KO, crawl::Language::EN, crawl::Language::JA,
        crawl::Language::ZH_TW, crawl::Language::ZH_CN
    };
    const std::array<int, 3> scales = {75, 100, 200};
    auto& localization = crawl::LocalizationManager::getInstance();

    for (const auto language : languages) {
        localization.setLanguage(language);
        sf::Font font;
        const char* fontFile = language == crawl::Language::KO || language == crawl::Language::EN
            ? "neodgm.ttf" : "NotoSansCJK-Regular.ttc";
        CHECK(font.loadFromFile(crawl::ResourceLocator::assetPath("fonts/" + std::string(fontFile)).string()));

        const auto item = crawl::ItemFactory::createItem("wpn_greatsword");
        const auto monster = crawl::MonsterFactory::createMonster("mon_goblin_shaman");
        const auto skill = crawl::SkillFactory::createSkill("spl_prayer_of_healing");
        const auto quest = crawl::Quest::createCanonical("qst_hunt_spiders");
        const std::vector<std::string> samples = {
            localization.get("TITLE_SUBTITLE"),
            localization.get("TOWN_CAMP_WELCOME"),
            localization.get("TOWN_CAMP_OPTION_1"),
            localization.get("GUILD_PREVIEW_GUIDE"),
            item->getName() + " — " + item->getDescription(),
            monster->getName(),
            skill->getName() + " — " + skill->getDescription(),
            quest->getName() + " — " + quest->getDescription(),
            localization.format("COMBAT_LOG_ATTACK_HIT", {
                {"actor", "Ragnar"}, {"roll", "17"}, {"ac", "14"}}),
            localization.format("COMBAT_LOG_DAMAGE", {{"target", monster->getName()}, {"damage", "12"}})
        };

        for (const int scale : scales) {
            localization.setTextScale(scale);
            const unsigned int size = localization.getScaledTextSize(16);
            const sf::Glyph replacementGlyph = font.getGlyph(0x10FFFFU, size, false);
            sf::RenderTexture canvas;
            CHECK(canvas.create(1600, 900));
            canvas.clear(sf::Color(5, 11, 5));
            float y = 24.0f;
            for (const auto& sample : samples) {
                const sf::String unicode = sf::String::fromUtf8(sample.begin(), sample.end());
                for (const sf::Uint32 codepoint : unicode) {
                    if (!isWhitespace(codepoint)) {
                        const sf::Glyph glyph = font.getGlyph(codepoint, size, false);
                        CHECK(glyph.advance > 0.0f);
                        CHECK(!sameGlyphRect(glyph, replacementGlyph));
                    }
                }
                sf::Text text(unicode, font, size);
                text.setFillColor(sf::Color(210, 255, 210));
                text.setPosition(24.0f, y);
                canvas.draw(text);
                y += static_cast<float>(size) * 1.7f;
            }
            canvas.display();
            const auto output = outputDirectory /
                (suffix(language) + "-scale-" + std::to_string(scale) + ".png");
            CHECK(canvas.getTexture().copyToImage().saveToFile(output.string()));
            CHECK(std::filesystem::file_size(output) > 1024U);
        }
    }

    localization.setLanguage(crawl::Language::KO);
    localization.setTextScale(100);
    if (failures != 0) return 1;
    std::cout << "Font raster evidence generated for 5 locales x 3 scales.\n";
    return 0;
}
