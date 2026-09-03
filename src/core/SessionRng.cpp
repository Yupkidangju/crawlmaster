#include "core/SessionRng.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace crawl {
namespace {

std::uint32_t entropySeed() {
    std::random_device device;
    const std::uint32_t seed = device();
    // schema에서 0은 legacy/미지정 sentinel이므로 새 session에는 사용하지 않는다.
    return seed == 0U ? 0x9E3779B9U : seed;
}

} // namespace

SessionRng::SessionRng(std::uint32_t seed) : m_seed(seed), m_engine(seed) {}

SessionRng::SessionRng(std::uint32_t seed, std::uint64_t drawCount)
    : m_seed(seed), m_drawCount(drawCount), m_engine(seed) {
    m_engine.discard(drawCount);
}

SessionRng& SessionRng::global() {
    static SessionRng instance(entropySeed());
    return instance;
}

void SessionRng::reseedGlobal(std::uint32_t seed) {
    global() = SessionRng(seed);
}

std::uint32_t SessionRng::startNewGlobalSession() {
    const std::uint32_t seed = entropySeed();
    reseedGlobal(seed);
    return seed;
}

std::uint32_t SessionRng::seed() const { return m_seed; }

std::uint64_t SessionRng::drawCount() const { return m_drawCount; }

std::uint32_t SessionRng::nextRaw() {
    ++m_drawCount;
    return m_engine();
}

int SessionRng::rollDie(int sides) {
    if (sides <= 0) throw std::invalid_argument("주사위 면수는 양수여야 합니다.");
    return rollRange(1, sides);
}

int SessionRng::rollRange(int minimum, int maximum) {
    if (minimum > maximum) std::swap(minimum, maximum);
    const std::uint64_t span = static_cast<std::uint64_t>(
        static_cast<std::int64_t>(maximum) - static_cast<std::int64_t>(minimum)) + 1U;
    constexpr std::uint64_t sourceRange =
        static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) + 1U;
    const std::uint64_t acceptanceLimit = sourceRange - (sourceRange % span);
    std::uint32_t sample = 0;
    do {
        sample = nextRaw();
    } while (static_cast<std::uint64_t>(sample) >= acceptanceLimit);
    const std::int64_t value = static_cast<std::int64_t>(minimum) +
        static_cast<std::int64_t>(static_cast<std::uint64_t>(sample) % span);
    return static_cast<int>(value);
}

} // namespace crawl
