// [v0.9.2] LocalizationManager.cpp 수정: 기본 설정 볼륨값을 BGM 60, SFX 80으로 정렬 및 getSf 헬퍼 구현, config 키 camelCase 변환.
// assets/lang/ 하위의 JSON 언어 데이터를 동적 파싱하여 다국어를 런타임에 서비스하고 config.json 설정을 관리하는 기능 구현.

#include "core/LocalizationManager.hpp"
#include "core/ResourceLocator.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace crawl {

LocalizationManager::LocalizationManager()
    : m_currentLang(Language::KO), m_textScale(100), m_highContrast(true) {
    // 1. 기본 번역 로드
    loadTranslations();
}

LocalizationManager& LocalizationManager::getInstance() {
    static LocalizationManager instance;
    return instance;
}

void LocalizationManager::setLanguage(Language lang) {
    m_currentLang = lang;
    loadTranslations();
}

Language LocalizationManager::getLanguage() const {
    return m_currentLang;
}

std::string LocalizationManager::getLanguageString() const {
    return getLangFileSuffix(m_currentLang);
}

std::string LocalizationManager::getLanguageName(Language lang) const {
    switch (lang) {
        case Language::KO:    return "한국어";
        case Language::EN:    return "English";
        case Language::JA:    return "日本語";
        case Language::ZH_TW: return "繁體中文";
        case Language::ZH_CN: return "简体中文";
    }
    return "Unknown";
}

std::string LocalizationManager::get(const std::string& key) const {
    auto it = m_translations.find(key);
    if (it != m_translations.end()) {
        return it->second;
    }
    // 키가 존재하지 않으면 디버깅이 수월하도록 키 자체를 반환
    return key;
}

sf::String LocalizationManager::getSf(const std::string& key) const {
    std::string utf8Str = get(key);
    return sf::String::fromUtf8(utf8Str.begin(), utf8Str.end());
}

bool LocalizationManager::has(const std::string& key) const {
    return m_translations.contains(key);
}

std::string LocalizationManager::format(
    const std::string& key,
    std::initializer_list<std::pair<std::string, std::string>> values) const {
    std::string result = get(key);
    for (const auto& [name, value] : values) {
        const std::string placeholder = "{" + name + "}";
        std::size_t position = 0;
        while ((position = result.find(placeholder, position)) != std::string::npos) {
            result.replace(position, placeholder.size(), value);
            position += value.size();
        }
    }
    return result;
}

std::string LocalizationManager::getContent(const std::string& category,
                                            const std::string& id,
                                            const std::string& field) const {
    std::string key = category + "_" + id + "_" + field;
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return get(key);
}

void LocalizationManager::loadTranslations() {
    std::string filename = ResourceLocator::assetPath(
        "lang/" + getLangFileSuffix(m_currentLang) + ".json").string();
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[i18n Error] 번역 리소스 파일을 로드할 수 없습니다: " << filename << std::endl;
        m_translations.clear();
        return;
    }

    try {
        nlohmann::json j;
        file >> j;
        m_translations.clear();
        for (auto& [key, value] : j.items()) {
            m_translations[key] = value.get<std::string>();
        }
    } catch (const std::exception& e) {
        std::cerr << "[i18n Parse Error] JSON 파싱 중 예외 발생: " << e.what() << std::endl;
    }
}

void LocalizationManager::setDefaultConfigPath(const std::string& filepath) {
    s_defaultConfigPath = filepath;
}

const std::string& LocalizationManager::getDefaultConfigPath() {
    if (s_defaultConfigPath.empty()) {
        s_defaultConfigPath = Persistence::defaultConfigPath().string();
    }
    return s_defaultConfigPath;
}

PersistenceResult LocalizationManager::loadConfig(const std::string& filepath) {
    auto loadCandidate = [this](const std::filesystem::path& candidate,
                                PersistenceStatus successStatus) -> PersistenceResult {
        std::error_code sizeError;
        if (!std::filesystem::exists(candidate)) {
            return {PersistenceStatus::NotFound, candidate, "설정 파일이 없습니다."};
        }
        const auto fileSize = std::filesystem::file_size(candidate, sizeError);
        if (sizeError) return {PersistenceStatus::IoError, candidate, sizeError.message()};
        if (fileSize > 256U * 1024U) {
            return {PersistenceStatus::Corrupt, candidate, "config 파일이 256 KiB 제한을 초과했습니다."};
        }

        std::ifstream file(candidate, std::ios::binary);
        if (!file.is_open()) {
            return {PersistenceStatus::IoError, candidate, "설정 파일을 읽을 수 없습니다."};
        }

        try {
            nlohmann::json j;
            file >> j;
            if (!j.is_object()) throw std::runtime_error("config 루트는 객체여야 합니다.");
            const int schemaVersion = j.value("schemaVersion", 1);
            if (schemaVersion < 1 || schemaVersion > 2) {
                return {PersistenceStatus::UnsupportedVersion, candidate,
                        "지원하지 않는 config schema입니다."};
            }
            const int languageValue = j.value("language", 0);
            if (languageValue < 0 || languageValue > 4) {
                throw std::runtime_error("language 범위를 벗어났습니다.");
            }
            const int textScale = schemaVersion == 2 ? j.value("textScale", 100) : 100;
            if (textScale < 75 || textScale > 200) {
                throw std::runtime_error("textScale 범위를 벗어났습니다.");
            }
            const bool highContrast = schemaVersion == 2 ? j.value("highContrast", true) : true;

            m_currentLang = static_cast<Language>(languageValue);
            m_textScale = textScale;
            m_highContrast = highContrast;
            loadTranslations();
            std::cout << "[Config] 전역 설정을 성공적으로 불러왔습니다." << std::endl;
            return {successStatus, candidate, {}};
        } catch (const std::exception& exception) {
            return {PersistenceStatus::Corrupt, candidate, exception.what()};
        }
    };

    const std::filesystem::path primary(filepath);
    auto primaryResult = loadCandidate(primary, PersistenceStatus::Loaded);
    const std::filesystem::path backup = filepath + ".bak";
    auto recoverBackup = [&]() -> PersistenceResult {
        auto backupResult = loadCandidate(backup, PersistenceStatus::RecoveredFromBackup);
        if (!backupResult) return backupResult;
        std::ifstream backupFile(backup, std::ios::binary);
        const std::string backupBytes{std::istreambuf_iterator<char>(backupFile),
                                      std::istreambuf_iterator<char>()};
        const auto restoreResult = Persistence::atomicWriteText(primary, backupBytes);
        backupResult.message = restoreResult
            ? "백업 config를 로드하고 primary를 복원했습니다."
            : "백업 config는 로드했지만 primary 복원에 실패했습니다: " + restoreResult.message;
        return backupResult;
    };

    if (primaryResult.status == PersistenceStatus::NotFound && std::filesystem::exists(backup)) {
        return recoverBackup();
    }
    if (primaryResult.status != PersistenceStatus::Corrupt) return primaryResult;

    std::filesystem::path quarantinePath;
    const auto quarantineResult = Persistence::quarantine(primary, quarantinePath);
    if (quarantineResult.status == PersistenceStatus::IoError) return quarantineResult;

    auto backupResult = recoverBackup();
    if (backupResult) {
        backupResult.message = "손상 설정을 격리하고 백업을 복구했습니다.";
        return backupResult;
    }

    std::cerr << "[Config Load Error] 손상 설정을 격리했습니다: " << primaryResult.message << std::endl;
    return {PersistenceStatus::Corrupt, quarantinePath, primaryResult.message};
}

PersistenceResult LocalizationManager::saveConfig(const std::string& filepath) const {
    try {
        nlohmann::json j;
        j["schemaVersion"] = 2;
        j["language"] = static_cast<int>(m_currentLang);
        j["textScale"] = m_textScale;
        j["highContrast"] = m_highContrast;

        auto result = Persistence::atomicWriteText(filepath, j.dump(4));
        if (result.status == PersistenceStatus::CommittedDurabilityUnknown) {
            std::cerr << "[Config Save Warning] " << result.message << std::endl;
        } else if (result) {
            std::cout << "[Config] 전역 설정을 성공적으로 저장했습니다." << std::endl;
        } else {
            std::cerr << "[Config Save Error] " << result.message << std::endl;
        }
        return result;
    } catch (const std::exception& e) {
        std::cerr << "[Config Save Error] 설정 저장 중 예외 발생: " << e.what() << std::endl;
        return {PersistenceStatus::IoError, filepath, e.what()};
    }
}

std::string LocalizationManager::getLangFileSuffix(Language lang) const {
    switch (lang) {
        case Language::KO:    return "ko";
        case Language::EN:    return "en";
        case Language::JA:    return "ja";
        case Language::ZH_TW: return "zh_tw";
        case Language::ZH_CN: return "zh_cn";
    }
    return "ko";
}

} // namespace crawl
