#ifndef SESSION_RNG_HPP
#define SESSION_RNG_HPP

#include <cstdint>
#include <random>

namespace crawl {

class SessionRng {
public:
    explicit SessionRng(std::uint32_t seed);
    SessionRng(std::uint32_t seed, std::uint64_t drawCount);
    static SessionRng& global();
    static void reseedGlobal(std::uint32_t seed);
    static std::uint32_t startNewGlobalSession();
    std::uint32_t seed() const;
    std::uint64_t drawCount() const;
    int rollDie(int sides);
    int rollRange(int minimum, int maximum);

private:
    std::uint32_t nextRaw();
    std::uint32_t m_seed;
    std::uint64_t m_drawCount = 0;
    std::mt19937 m_engine;
};

} // namespace crawl

#endif // SESSION_RNG_HPP
