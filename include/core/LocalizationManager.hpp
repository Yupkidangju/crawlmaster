#ifndef LOCALIZATION_MANAGER_HPP
#define LOCALIZATION_MANAGER_HPP

#include <SFML/System/String.hpp>
#include <algorithm>
#include <initializer_list>
#include <string>
#include <unordered_map>
#include <utility>
#include "core/Persistence.hpp"

namespace crawl {

// D3D 다국어 순서: 한국어 / 영어 / 일본어 / 중국어 번체 / 중국어 간체
enum class Language {
    KO = 0,
    EN,
    JA,
    ZH_TW,
    ZH_CN
};

class LocalizationManager {
public:
    static constexpr int CONFIG_SCHEMA_VERSION = 3;

    static LocalizationManager& getInstance();

    void setLanguage(Language lang);
    [[nodiscard]] Language getLanguage() const;
    [[nodiscard]] std::string getLanguageString() const;
    [[nodiscard]] std::string getLanguageName(Language lang) const;

    [[nodiscard]] std::string get(const std::string& key) const;
    [[nodiscard]] sf::String getSf(const std::string& key) const;
    [[nodiscard]] bool has(const std::string& key) const;
    [[nodiscard]] std::string format(
        const std::string& key,
        std::initializer_list<std::pair<std::string, std::string>> values) const;
    [[nodiscard]] std::string getContent(const std::string& category, const std::string& id,
                                         const std::string& field) const;

    static void setDefaultConfigPath(const std::string& filepath);
    static const std::string& getDefaultConfigPath();
    PersistenceResult loadConfig(const std::string& filepath = getDefaultConfigPath());
    PersistenceResult saveConfig(const std::string& filepath = getDefaultConfigPath()) const;

    // 접근성
    [[nodiscard]] int getTextScale() const { return m_textScale; }
    [[nodiscard]] bool getHighContrast() const { return m_highContrast; }
    void setTextScale(int scale) { m_textScale = std::clamp(scale, 75, 200); }
    void setHighContrast(bool enabled) { m_highContrast = enabled; }
    [[nodiscard]] unsigned int getScaledTextSize(unsigned int baseSize) const {
        return std::max(11U, baseSize * static_cast<unsigned int>(m_textScale) / 100U);
    }

    // 디스플레이 & 그래픽 (Config v3)
    [[nodiscard]] int getWindowWidth() const { return m_windowWidth; }
    [[nodiscard]] int getWindowHeight() const { return m_windowHeight; }
    [[nodiscard]] bool isFullscreen() const { return m_fullscreen; }
    [[nodiscard]] int getGraphicsQuality() const { return m_graphicsQuality; }
    void setResolution(int width, int height) {
        m_windowWidth = std::clamp(width, 960, 3840);
        m_windowHeight = std::clamp(height, 540, 2160);
    }
    void setFullscreen(bool enabled) { m_fullscreen = enabled; }
    void setGraphicsQuality(int quality) { m_graphicsQuality = std::clamp(quality, 0, 1); }

    // 오디오 (Config v3)
    [[nodiscard]] int getMasterVolume() const { return m_masterVolume; }
    [[nodiscard]] int getBgmVolume() const { return m_bgmVolume; }
    [[nodiscard]] int getSfxVolume() const { return m_sfxVolume; }
    [[nodiscard]] bool isMuted() const { return m_muted; }
    void setMasterVolume(int pct) { m_masterVolume = std::clamp(pct, 0, 100); }
    void setBgmVolume(int pct) { m_bgmVolume = std::clamp(pct, 0, 100); }
    void setSfxVolume(int pct) { m_sfxVolume = std::clamp(pct, 0, 100); }
    void setMuted(bool muted) { m_muted = muted; }

private:
    LocalizationManager();
    ~LocalizationManager() = default;
    LocalizationManager(const LocalizationManager&) = delete;
    LocalizationManager& operator=(const LocalizationManager&) = delete;

    void loadTranslations();
    [[nodiscard]] std::string getLangFileSuffix(Language lang) const;

    Language m_currentLang = Language::KO;
    std::unordered_map<std::string, std::string> m_translations;

    int m_textScale = 100;
    bool m_highContrast = false;
    int m_windowWidth = 1280;
    int m_windowHeight = 720;
    bool m_fullscreen = false;
    int m_graphicsQuality = 1; // 1 = High (Shaders+PostFX), 0 = Medium
    int m_masterVolume = 80;
    int m_bgmVolume = 65;
    int m_sfxVolume = 85;
    bool m_muted = false;

    inline static std::string s_defaultConfigPath;
};

} // namespace crawl

#endif // LOCALIZATION_MANAGER_HPP
