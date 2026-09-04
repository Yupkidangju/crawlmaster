#ifndef CHARACTER_IDENTITY_RULES_HPP
#define CHARACTER_IDENTITY_RULES_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace crawl {

class CharacterIdentityRules {
public:
    static std::optional<std::string> normalizeName(std::string_view input) {
        std::vector<std::uint32_t> codePoints;
        for (std::size_t index = 0; index < input.size();) {
            const auto first = static_cast<std::uint8_t>(input[index]);
            std::uint32_t codePoint = 0;
            std::size_t width = 0;
            if (first <= 0x7FU) { codePoint = first; width = 1; }
            else if ((first & 0xE0U) == 0xC0U) { codePoint = first & 0x1FU; width = 2; }
            else if ((first & 0xF0U) == 0xE0U) { codePoint = first & 0x0FU; width = 3; }
            else if ((first & 0xF8U) == 0xF0U) { codePoint = first & 0x07U; width = 4; }
            else return std::nullopt;
            if (index + width > input.size()) return std::nullopt;
            for (std::size_t offset = 1; offset < width; ++offset) {
                const auto continuation = static_cast<std::uint8_t>(input[index + offset]);
                if ((continuation & 0xC0U) != 0x80U) return std::nullopt;
                codePoint = (codePoint << 6U) | (continuation & 0x3FU);
            }
            if ((width == 2 && codePoint < 0x80U) ||
                (width == 3 && codePoint < 0x800U) ||
                (width == 4 && codePoint < 0x10000U) || codePoint > 0x10FFFFU ||
                (codePoint >= 0xD800U && codePoint <= 0xDFFFU) || isForbidden(codePoint)) {
                return std::nullopt;
            }
            codePoints.push_back(codePoint);
            index += width;
        }
        while (!codePoints.empty() && isWhitespace(codePoints.front())) codePoints.erase(codePoints.begin());
        while (!codePoints.empty() && isWhitespace(codePoints.back())) codePoints.pop_back();
        if (codePoints.empty() || codePoints.size() > 16) return std::nullopt;

        std::string normalized;
        for (const auto codePoint : codePoints) appendUtf8(normalized, codePoint);
        return normalized;
    }

    static bool isValidName(std::string_view input) {
        const auto normalized = normalizeName(input);
        return normalized.has_value() && *normalized == input;
    }

private:
    static bool isWhitespace(std::uint32_t codePoint) {
        return codePoint == 0x20U || codePoint == 0xA0U || codePoint == 0x1680U ||
               (codePoint >= 0x2000U && codePoint <= 0x200AU) || codePoint == 0x2028U ||
               codePoint == 0x2029U || codePoint == 0x202FU || codePoint == 0x205FU ||
               codePoint == 0x3000U;
    }

    static bool isForbidden(std::uint32_t codePoint) {
        return codePoint < 0x20U || (codePoint >= 0x7FU && codePoint <= 0x9FU) ||
               codePoint == 0xADU || (codePoint >= 0x600U && codePoint <= 0x605U) ||
               codePoint == 0x61CU || codePoint == 0x6DDU || codePoint == 0x70FU ||
               (codePoint >= 0x890U && codePoint <= 0x891U) || codePoint == 0x8E2U ||
               codePoint == 0x180EU ||
               (codePoint >= 0x200BU && codePoint <= 0x200FU) ||
               (codePoint >= 0x202AU && codePoint <= 0x202EU) ||
               (codePoint >= 0x2060U && codePoint <= 0x206FU) || codePoint == 0xFEFFU ||
               (codePoint >= 0xFFF9U && codePoint <= 0xFFFBU) || codePoint == 0x110BDU ||
               codePoint == 0x110CDU || (codePoint >= 0x13430U && codePoint <= 0x1343FU) ||
               (codePoint >= 0x1BCA0U && codePoint <= 0x1BCAFU) ||
               (codePoint >= 0x1D173U && codePoint <= 0x1D17AU) || codePoint == 0xE0001U ||
               (codePoint >= 0xE0020U && codePoint <= 0xE007FU);
    }

    static void appendUtf8(std::string& output, std::uint32_t codePoint) {
        if (codePoint <= 0x7FU) output.push_back(static_cast<char>(codePoint));
        else if (codePoint <= 0x7FFU) {
            output.push_back(static_cast<char>(0xC0U | (codePoint >> 6U)));
            output.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
        } else if (codePoint <= 0xFFFFU) {
            output.push_back(static_cast<char>(0xE0U | (codePoint >> 12U)));
            output.push_back(static_cast<char>(0x80U | ((codePoint >> 6U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
        } else {
            output.push_back(static_cast<char>(0xF0U | (codePoint >> 18U)));
            output.push_back(static_cast<char>(0x80U | ((codePoint >> 12U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | ((codePoint >> 6U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | (codePoint & 0x3FU)));
        }
    }
};

} // namespace crawl

#endif
