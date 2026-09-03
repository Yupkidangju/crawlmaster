// [v0.9.2] LocalizationManager.hpp 수정: sf::String 반환 헬퍼 선언 추가 및 SFML String 헤더 포함.
// 다국어 i18n 번역 파일(ko.json, en.json 등) 로딩 및 실시간 번역 텍스트 반환, 전역 볼륨 설정 영속 저장을 처리하는 매니저 클래스 정의.

#ifndef LOCALIZATION_MANAGER_HPP
#define LOCALIZATION_MANAGER_HPP

#include <string>
#include <unordered_map>
#include <initializer_list>
#include <utility>
#include <algorithm>
#include <SFML/System/String.hpp>
#include "core/Persistence.hpp"

namespace crawl {

// D3D 규칙에 부합하는 i18n 언어 순서 준수 [한 / 영 / 일 / 중(번체) / 중(간체)]
enum class Language {
    KO = 0,
    EN,
    JA,
    ZH_TW,
    ZH_CN
};

class LocalizationManager {
public:
    static LocalizationManager& getInstance();

    // 언어 설정 및 해당 언어 리소스 로드
    void setLanguage(Language lang);
    Language getLanguage() const;
    std::string getLanguageString() const;
    std::string getLanguageName(Language lang) const;

    // 번역 텍스트 반환 (키가 없으면 키 자체 반환)
    std::string get(const std::string& key) const;
    sf::String getSf(const std::string& key) const;
    bool has(const std::string& key) const;
    std::string format(const std::string& key,
                       std::initializer_list<std::pair<std::string, std::string>> values) const;
    std::string getContent(const std::string& category, const std::string& id,
                           const std::string& field) const;

    // config.json 파일 전역 설정 세이브 / 로드
    static void setDefaultConfigPath(const std::string& filepath);
    static const std::string& getDefaultConfigPath();
    PersistenceResult loadConfig(const std::string& filepath = getDefaultConfigPath());
    PersistenceResult saveConfig(const std::string& filepath = getDefaultConfigPath()) const;

    int getTextScale() const { return m_textScale; }
    bool getHighContrast() const { return m_highContrast; }
    void setTextScale(int scale) { m_textScale = std::clamp(scale, 75, 200); }
    void setHighContrast(bool enabled) { m_highContrast = enabled; }
    unsigned int getScaledTextSize(unsigned int baseSize) const {
        return std::max(14U, baseSize * static_cast<unsigned int>(m_textScale) / 100U);
    }

private:
    LocalizationManager();
    ~LocalizationManager() = default;

    LocalizationManager(const LocalizationManager&) = delete;
    LocalizationManager& operator=(const LocalizationManager&) = delete;

    // 번역 데이터 로드 도우미
    void loadTranslations();

    Language m_currentLang;
    std::unordered_map<std::string, std::string> m_translations;

    int m_textScale;
    bool m_highContrast;
    inline static std::string s_defaultConfigPath;

    std::string getLangFileSuffix(Language lang) const;
};

} // namespace crawl

#endif // LOCALIZATION_MANAGER_HPP
