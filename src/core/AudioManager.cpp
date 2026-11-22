#include "core/AudioManager.hpp"
#include "core/ResourceLocator.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>

#if defined(_MSC_VER)
#pragma warning(push, 0)
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#define MA_NO_ENCODING
#define MA_NO_FLAC
#define MA_NO_MP3
#include <miniaudio.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

namespace crawl {
namespace {

bool envFlagEnabled(const char* name) {
#ifdef _MSC_VER
    char* raw = nullptr;
    std::size_t sz = 0;
    if (_dupenv_s(&raw, &sz, name) != 0 || raw == nullptr) {
        std::free(raw);
        return false;
    }
    const std::string val(raw);
    std::free(raw);
    return val == "1" || val == "true" || val == "TRUE";
#else
    const char* raw = std::getenv(name);
    if (!raw) return false;
    const std::string val(raw);
    return val == "1" || val == "true" || val == "TRUE";
#endif
}

float clamp01(float v) {
    return std::clamp(v, 0.0f, 1.0f);
}

} // namespace

struct AudioManager::Impl {
    static constexpr std::size_t SFX_POOL_SIZE = 16;

    std::mutex mutex;
    ma_engine engine{};
    ma_sound bgmSound{};
    ma_sound ambSound{};
    std::array<ma_sound, SFX_POOL_SIZE> sfxPool{};
    std::array<bool, SFX_POOL_SIZE> sfxLoaded{};
    std::size_t nextSfxSlot = 0;

    bool initialized = false;
    bool engineReady = false;
    bool headless = false;
    bool bgmLoaded = false;
    bool ambLoaded = false;

    float masterVol = 0.80f;
    float bgmVol = 0.65f;
    float sfxVol = 0.85f;
    bool muted = false;

    std::string currentBgmId;
    std::string currentAmbId;

    void updateGainsLocked() {
        if (!engineReady) return;
        const float master = muted ? 0.0f : masterVol;
        ma_engine_set_volume(&engine, master);
        if (bgmLoaded) ma_sound_set_volume(&bgmSound, bgmVol * 0.65f);
        if (ambLoaded) ma_sound_set_volume(&ambSound, bgmVol * 0.42f);
        for (std::size_t i = 0; i < SFX_POOL_SIZE; ++i) {
            if (sfxLoaded[i]) ma_sound_set_volume(&sfxPool[i], sfxVol);
        }
    }

    void unloadAllLocked() {
        if (bgmLoaded) {
            ma_sound_uninit(&bgmSound);
            bgmLoaded = false;
        }
        if (ambLoaded) {
            ma_sound_uninit(&ambSound);
            ambLoaded = false;
        }
        for (std::size_t i = 0; i < SFX_POOL_SIZE; ++i) {
            if (sfxLoaded[i]) {
                ma_sound_uninit(&sfxPool[i]);
                sfxLoaded[i] = false;
            }
        }
        if (engineReady) {
            ma_engine_uninit(&engine);
            engineReady = false;
        }
        currentBgmId.clear();
        currentAmbId.clear();
        initialized = false;
    }
};

AudioManager::AudioManager() : m_impl(std::make_unique<Impl>()) {}
AudioManager::~AudioManager() { shutdown(); }

AudioManager& AudioManager::getInstance() {
    static AudioManager instance;
    return instance;
}

void AudioManager::initialize(bool forceHeadless) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (m_impl->initialized) return;

    m_impl->initialized = true;
    const bool wantHeadless = forceHeadless || envFlagEnabled("CRAWLMASTER_HEADLESS") ||
                              envFlagEnabled("CI") || envFlagEnabled("GITHUB_ACTIONS");
    if (wantHeadless) {
        m_impl->headless = true;
        m_impl->engineReady = false;
        return;
    }

    ma_engine_config config = ma_engine_config_init();
    config.channels = 2;
    config.sampleRate = 22050;
    if (ma_engine_init(&config, &m_impl->engine) == MA_SUCCESS) {
        m_impl->engineReady = true;
        m_impl->headless = false;
        m_impl->updateGainsLocked();
    } else {
        // 오디오 장치가 없는 데스크톱/컨테이너 환경에서도 절대 크래시하지 않고 무음 폴백한다.
        m_impl->engineReady = false;
        m_impl->headless = true;
        std::cerr << "[Audio] Hardware device unavailable; running in headless audio fallback." << std::endl;
    }
}

void AudioManager::shutdown() {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->unloadAllLocked();
}

void AudioManager::playBgm(const std::string& trackId) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (!m_impl->initialized) return;
    if (m_impl->currentBgmId == trackId && (m_impl->bgmLoaded || m_impl->headless)) return;

    m_impl->currentBgmId = trackId;
    if (!m_impl->engineReady) return;

    if (m_impl->bgmLoaded) {
        ma_sound_stop(&m_impl->bgmSound);
        ma_sound_uninit(&m_impl->bgmSound);
        m_impl->bgmLoaded = false;
    }

    const auto path = ResourceLocator::assetPath("audio/" + trackId + ".wav");
    if (!std::filesystem::is_regular_file(path)) return;
    const std::string pathStr = path.string();
    if (ma_sound_init_from_file(&m_impl->engine, pathStr.c_str(),
                                MA_SOUND_FLAG_STREAM, nullptr, nullptr, &m_impl->bgmSound) == MA_SUCCESS) {
        m_impl->bgmLoaded = true;
        ma_sound_set_looping(&m_impl->bgmSound, MA_TRUE);
        ma_sound_set_volume(&m_impl->bgmSound, m_impl->bgmVol * 0.65f);
        ma_sound_start(&m_impl->bgmSound);
    }
}

void AudioManager::stopBgm() {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->currentBgmId.clear();
    if (m_impl->bgmLoaded) {
        ma_sound_stop(&m_impl->bgmSound);
        ma_sound_uninit(&m_impl->bgmSound);
        m_impl->bgmLoaded = false;
    }
}

void AudioManager::playAmbience(int floorNumber) {
    const int clampedFloor = std::clamp(floorNumber, 1, 5);
    const std::string ambId = "amb_floor" + std::to_string(clampedFloor);
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (!m_impl->initialized) return;
    if (m_impl->currentAmbId == ambId && (m_impl->ambLoaded || m_impl->headless)) return;

    m_impl->currentAmbId = ambId;
    if (!m_impl->engineReady) return;

    if (m_impl->ambLoaded) {
        ma_sound_stop(&m_impl->ambSound);
        ma_sound_uninit(&m_impl->ambSound);
        m_impl->ambLoaded = false;
    }

    const auto path = ResourceLocator::assetPath("audio/" + ambId + ".wav");
    if (!std::filesystem::is_regular_file(path)) return;
    const std::string pathStr = path.string();
    if (ma_sound_init_from_file(&m_impl->engine, pathStr.c_str(),
                                MA_SOUND_FLAG_STREAM, nullptr, nullptr, &m_impl->ambSound) == MA_SUCCESS) {
        m_impl->ambLoaded = true;
        ma_sound_set_looping(&m_impl->ambSound, MA_TRUE);
        ma_sound_set_volume(&m_impl->ambSound, m_impl->bgmVol * 0.42f);
        ma_sound_start(&m_impl->ambSound);
    }
}

void AudioManager::stopAmbience() {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->currentAmbId.clear();
    if (m_impl->ambLoaded) {
        ma_sound_stop(&m_impl->ambSound);
        ma_sound_uninit(&m_impl->ambSound);
        m_impl->ambLoaded = false;
    }
}

void AudioManager::playSfx(const std::string& sfxId) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (!m_impl->initialized || !m_impl->engineReady || m_impl->muted) return;

    const auto path = ResourceLocator::assetPath("audio/" + sfxId + ".wav");
    if (!std::filesystem::is_regular_file(path)) return;

    const std::size_t slot = m_impl->nextSfxSlot;
    m_impl->nextSfxSlot = (m_impl->nextSfxSlot + 1) % Impl::SFX_POOL_SIZE;

    if (m_impl->sfxLoaded[slot]) {
        ma_sound_stop(&m_impl->sfxPool[slot]);
        ma_sound_uninit(&m_impl->sfxPool[slot]);
        m_impl->sfxLoaded[slot] = false;
    }

    const std::string pathStr = path.string();
    if (ma_sound_init_from_file(&m_impl->engine, pathStr.c_str(),
                                MA_SOUND_FLAG_DECODE, nullptr, nullptr, &m_impl->sfxPool[slot]) == MA_SUCCESS) {
        m_impl->sfxLoaded[slot] = true;
        ma_sound_set_volume(&m_impl->sfxPool[slot], m_impl->sfxVol);
        ma_sound_start(&m_impl->sfxPool[slot]);
    }
}

void AudioManager::applyVolumeConfig(int masterPct, int bgmPct, int sfxPct, bool muted) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->masterVol = clamp01(static_cast<float>(masterPct) / 100.0f);
    m_impl->bgmVol = clamp01(static_cast<float>(bgmPct) / 100.0f);
    m_impl->sfxVol = clamp01(static_cast<float>(sfxPct) / 100.0f);
    m_impl->muted = muted;
    m_impl->updateGainsLocked();
}

bool AudioManager::isInitialized() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->initialized;
}

bool AudioManager::isHeadlessFallback() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    return m_impl->headless;
}

const std::string& AudioManager::currentBgm() const {
    return m_impl->currentBgmId;
}

const std::string& AudioManager::currentAmbience() const {
    return m_impl->currentAmbId;
}

} // namespace crawl
