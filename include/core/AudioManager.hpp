#ifndef AUDIO_MANAGER_HPP
#define AUDIO_MANAGER_HPP

#include <memory>
#include <string>

namespace crawl {

// miniaudio 0.11.26 기반 크로스플랫폼 오디오 엔진.
// 헤드리스/CI 환경이나 오디오 장치 부재 시 자동으로 Null 모드로 안전 폴백한다.
class AudioManager {
public:
    static AudioManager& getInstance();

    void initialize(bool forceHeadless = false);
    void shutdown();

    void playBgm(const std::string& trackId);
    void stopBgm();
    void playAmbience(int floorNumber);
    void stopAmbience();
    void playSfx(const std::string& sfxId);

    void applyVolumeConfig(int masterPct, int bgmPct, int sfxPct, bool muted);
    [[nodiscard]] bool isInitialized() const;
    [[nodiscard]] bool isHeadlessFallback() const;
    [[nodiscard]] const std::string& currentBgm() const;
    [[nodiscard]] const std::string& currentAmbience() const;

private:
    AudioManager();
    ~AudioManager();
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace crawl

#endif // AUDIO_MANAGER_HPP
