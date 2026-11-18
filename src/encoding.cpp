#include "encoding.h"

#include <array>
#include <cstdint>
#include <utility>

namespace strsearch {
namespace {

// Символи cp1251 у діапазоні 0x80..0xBF, які потрібні для українського тексту.
constexpr std::array<std::pair<unsigned char, char32_t>, 25> kSpecial{{
    {0xA8, U'Ё'}, {0xB8, U'ё'}, {0xAA, U'Є'}, {0xBA, U'є'}, {0xB2, U'І'},
    {0xB3, U'і'}, {0xAF, U'Ї'}, {0xBF, U'ї'}, {0xA5, U'Ґ'}, {0xB4, U'ґ'},
    {0xAB, U'«'}, {0xBB, U'»'}, {0x97, U'—'}, {0x96, U'–'}, {0x92, U'’'},
    {0x91, U'‘'}, {0x93, U'“'}, {0x94, U'”'}, {0x84, U'„'}, {0xB9, U'№'},
    {0x85, U'…'}, {0xB0, U'°'}, {0xA7, U'§'}, {0xB1, U'±'}, {0xA0, U' '},
}};

char32_t toUnicode(unsigned char b) {
    if (b < 0x80) return b;
    if (b >= 0xC0) return U'А' + (b - 0xC0);
    for (auto [k, v] : kSpecial)
        if (k == b) return v;
    return U'?';
}

unsigned char fromUnicode(char32_t c) {
    if (c < 0x80) return static_cast<unsigned char>(c);
    if (c >= U'А' && c <= U'я') return static_cast<unsigned char>(0xC0 + (c - U'А'));
    if (c == U'ʼ') return '\'';  // модифікаційний апостроф
    for (auto [k, v] : kSpecial)
        if (v == c) return k;
    return '?';
}

void appendUtf8(std::string& out, char32_t c) {
    if (c < 0x80) {
        out += static_cast<char>(c);
    } else if (c < 0x800) {
        out += static_cast<char>(0xC0 | (c >> 6));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        out += static_cast<char>(0xE0 | (c >> 12));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (c >> 18));
        out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (c & 0x3F));
    }
}

}  // namespace

std::string utf8ToCp1251(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    std::size_t i = 0;
    while (i < s.size()) {
        const auto b = static_cast<unsigned char>(s[i]);
        char32_t cp;
        std::size_t len;
        if (b < 0x80) {
            cp = b;
            len = 1;
        } else if ((b & 0xE0) == 0xC0) {
            cp = b & 0x1F;
            len = 2;
        } else if ((b & 0xF0) == 0xE0) {
            cp = b & 0x0F;
            len = 3;
        } else if ((b & 0xF8) == 0xF0) {
            cp = b & 0x07;
            len = 4;
        } else {
            out += '?';
            ++i;
            continue;
        }
        if (i + len > s.size()) {
            out += '?';
            break;
        }
        bool ok = true;
        for (std::size_t k = 1; k < len; ++k) {
            const auto c = static_cast<unsigned char>(s[i + k]);
            if ((c & 0xC0) != 0x80) {
                ok = false;
                break;
            }
            cp = (cp << 6) | (c & 0x3F);
        }
        if (!ok) {
            out += '?';
            ++i;
            continue;
        }
        i += len;
        if (cp == 0x0301) continue;  // наголос - пропускаємо
        out += static_cast<char>(fromUnicode(cp));
    }
    return out;
}

std::string cp1251ToUtf8(std::string_view s) {
    std::string out;
    out.reserve(s.size() * 2);
    for (char ch : s) appendUtf8(out, toUnicode(static_cast<unsigned char>(ch)));
    return out;
}

bool isLetterCp1251(unsigned char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c >= 0xC0 || c == 0xA8 ||
           c == 0xB8 || c == 0xAA || c == 0xBA || c == 0xB2 || c == 0xB3 || c == 0xAF ||
           c == 0xBF || c == 0xA5 || c == 0xB4 || c == '\'' || c == 0x92;
}

}  // namespace strsearch
