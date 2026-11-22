#include "core/LocalizationManager.hpp"
#include "core/AudioManager.hpp"
#include "core/ResourceLocator.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace crawl {

LocalizationManager::LocalizationManager() {
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

Language LocalizationManager::getLanguage() const { return m_currentLang; }

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
    const auto it = m_translations.find(key);
    return (it != m_translations.end()) ? it->second : key;
}

sf::String LocalizationManager::getSf(const std::string& key) const {
    const std::string utf8Str = get(key);
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
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return get(key);
}

void LocalizationManager::loadTranslations() {
    const std::string filename = ResourceLocator::assetPath(
        "lang/" + getLangFileSuffix(m_currentLang) + ".json").string();
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[i18n Error] Failed to load locale file: " << filename << std::endl;
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
        std::cerr << "[i18n Parse Error] " << e.what() << std::endl;
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
            return {PersistenceStatus::NotFound, candidate, "Config file not found."};
        }
        const auto fileSize = std::filesystem::file_size(candidate, sizeError);
        if (sizeError) return {PersistenceStatus::IoError, candidate, sizeError.message()};
        if (fileSize > 256U * 1024U) {
            return {PersistenceStatus::Corrupt, candidate, "Config exceeds 256 KiB limit."};
        }

        std::ifstream file(candidate, std::ios::binary);
        if (!file.is_open()) {
            return {PersistenceStatus::IoError, candidate, "Unable to open config file."};
        }

        try {
            nlohmann::json j;
            file >> j;
            if (!j.is_object()) throw std::runtime_error("config root must be an object");
            const int schemaVersion = j.value("schemaVersion", 1);
            if (schemaVersion < 1 || schemaVersion > CONFIG_SCHEMA_VERSION) {
                return {PersistenceStatus::UnsupportedVersion, candidate,
                        "Unsupported config schema version."};
            }

            const int languageValue = j.value("language", 0);
            if (languageValue < 0 || languageValue > 4) {
                throw std::runtime_error("language out of range");
            }
            const int textScale = schemaVersion >= 2 ? j.value("textScale", 100) : 100;
            if (textScale < 75 || textScale > 200) {
                throw std::runtime_error("textScale out of range");
            }
            const bool highContrast = schemaVersion >= 2 ? j.value("highContrast", false) : false;

            int windowWidth = 1280;
            int windowHeight = 720;
            bool fullscreen = false;
            int graphicsQuality = 1;
            int masterVolume = 80;
            int bgmVolume = 65;
            int sfxVolume = 85;
            bool muted = false;

            if (schemaVersion >= 3) {
                windowWidth = j.value("windowWidth", 1280);
                windowHeight = j.value("windowHeight", 720);
                if (windowWidth < 960 || windowWidth > 3840 || windowHeight < 540 || windowHeight > 2160) {
                    throw std::runtime_error("window resolution out of range");
                }
                fullscreen = j.value("fullscreen", false);
                graphicsQuality = j.value("graphicsQuality", 1);
                if (graphicsQuality < 0 || graphicsQuality > 1) {
                    throw std::runtime_error("graphicsQuality out of range");
                }
                masterVolume = j.value("masterVolume", 80);
                bgmVolume = j.value("bgmVolume", 65);
                sfxVolume = j.value("sfxVolume", 85);
                if (masterVolume < 0 || masterVolume > 100 ||
                    bgmVolume < 0 || bgmVolume > 100 ||
                    sfxVolume < 0 || sfxVolume > 100) {
                    throw std::runtime_error("audio volume out of range");
                }
                muted = j.value("muted", false);
            }

            m_currentLang = static_cast<Language>(languageValue);
            m_textScale = textScale;
            m_highContrast = highContrast;
            m_windowWidth = windowWidth;
            m_windowHeight = windowHeight;
            m_fullscreen = fullscreen;
            m_graphicsQuality = graphicsQuality;
            m_masterVolume = masterVolume;
            m_bgmVolume = bgmVolume;
            m_sfxVolume = sfxVolume;
            m_muted = muted;

            loadTranslations();
            AudioManager::getInstance().applyVolumeConfig(
                m_masterVolume, m_bgmVolume, m_sfxVolume, m_muted);
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
        std::string backupBytes;
        {
            std::ifstream backupFile(backup, std::ios::binary);
            backupBytes.assign(std::istreambuf_iterator<char>(backupFile),
                               std::istreambuf_iterator<char>());
        }
        const auto restoreResult = Persistence::atomicWriteText(primary, backupBytes);
        backupResult.message = restoreResult
            ? "Recovered config from backup."
            : "Recovered backup in memory but failed to restore primary: " + restoreResult.message;
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
        backupResult.message = "Quarantined corrupt config and recovered backup.";
        return backupResult;
    }

    return {PersistenceStatus::Corrupt, quarantinePath, primaryResult.message};
}

PersistenceResult LocalizationManager::saveConfig(const std::string& filepath) const {
    try {
        nlohmann::json j;
        j["schemaVersion"] = CONFIG_SCHEMA_VERSION;
        j["language"] = static_cast<int>(m_currentLang);
        j["textScale"] = m_textScale;
        j["highContrast"] = m_highContrast;
        j["windowWidth"] = m_windowWidth;
        j["windowHeight"] = m_windowHeight;
        j["fullscreen"] = m_fullscreen;
        j["graphicsQuality"] = m_graphicsQuality;
        j["masterVolume"] = m_masterVolume;
        j["bgmVolume"] = m_bgmVolume;
        j["sfxVolume"] = m_sfxVolume;
        j["muted"] = m_muted;

        return Persistence::atomicWriteText(filepath, j.dump(4));
    } catch (const std::exception& e) {
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
