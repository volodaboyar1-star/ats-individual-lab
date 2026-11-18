#include "search.h"

#include <algorithm>

namespace strsearch {

// ----------------------------------------------------------------------------
// Наївний алгоритм: зразок прикладається до кожної позиції тексту
// і порівнюється зліва направо до першої розбіжності. O(n*m) у найгіршому.
// ----------------------------------------------------------------------------
template <bool Count>
SearchResult naiveSearch(std::string_view text, std::string_view pat) {
    SearchResult r;
    const std::size_t n = text.size(), m = pat.size();
    if (m == 0 || m > n) return r;
    for (std::size_t s = 0; s + m <= n; ++s) {
        std::size_t j = 0;
        while (j < m) {
            if constexpr (Count) ++r.comparisons;
            if (text[s + j] != pat[j]) break;
            ++j;
        }
        if (j == m) r.matches.push_back(s);
    }
    return r;
}

// ----------------------------------------------------------------------------
// КМП
// ----------------------------------------------------------------------------
std::vector<std::size_t> prefixFunction(std::string_view p, std::uint64_t* cmp) {
    const std::size_t m = p.size();
    std::vector<std::size_t> pi(m, 0);
    std::size_t k = 0;
    for (std::size_t i = 1; i < m; ++i) {
        for (;;) {
            if (cmp) ++*cmp;
            if (p[i] == p[k]) {
                ++k;
                break;
            }
            if (k == 0) break;
            k = pi[k - 1];
        }
        pi[i] = k;
    }
    return pi;
}

template <bool Count>
SearchResult kmpSearch(std::string_view text, std::string_view pat) {
    SearchResult r;
    const std::size_t n = text.size(), m = pat.size();
    if (m == 0 || m > n) return r;
    const auto pi = prefixFunction(pat, Count ? &r.preprocessing : nullptr);
    std::size_t q = 0;  // кількість символів зразка, що вже збіглися
    for (std::size_t i = 0; i < n; ++i) {
        for (;;) {
            if constexpr (Count) ++r.comparisons;
            if (pat[q] == text[i]) {
                ++q;
                break;
            }
            if (q == 0) break;
            q = pi[q - 1];  // відкат по префікс-функції, текст не повертаємо
        }
        if (q == m) {
            r.matches.push_back(i + 1 - m);
            q = pi[q - 1];
        }
    }
    return r;
}

// ----------------------------------------------------------------------------
// Боєр-Мур
// ----------------------------------------------------------------------------
std::vector<std::ptrdiff_t> badCharTable(std::string_view p) {
    std::vector<std::ptrdiff_t> last(256, -1);
    for (std::size_t i = 0; i < p.size(); ++i)
        last[static_cast<unsigned char>(p[i])] = static_cast<std::ptrdiff_t>(i);
    return last;
}

// Сильна евристика хорошого суфікса.
// bpos[i] - початок найширшої межі суфікса p[i..m-1].
// Випадок 1: хороший суфікс зустрічається в зразку ще раз (з іншим символом перед ним).
// Випадок 2: лише частина хорошого суфікса збігається з префіксом зразка.
std::vector<std::size_t> goodSuffixTable(std::string_view p, std::uint64_t* cmp) {
    const std::size_t m = p.size();
    std::vector<std::size_t> shift(m + 1, 0), bpos(m + 1, 0);
    std::size_t i = m, j = m + 1;
    bpos[i] = j;
    while (i > 0) {
        while (j <= m) {
            if (cmp) ++*cmp;
            if (p[i - 1] == p[j - 1]) break;
            if (shift[j] == 0) shift[j] = j - i;
            j = bpos[j];
        }
        --i;
        --j;
        bpos[i] = j;
    }
    j = bpos[0];
    for (i = 0; i <= m; ++i) {
        if (shift[i] == 0) shift[i] = j;
        if (i == j) j = bpos[j];
    }
    return shift;
}

template <bool Count>
SearchResult boyerMooreSearch(std::string_view text, std::string_view pat) {
    SearchResult r;
    const std::size_t n = text.size(), m = pat.size();
    if (m == 0 || m > n) return r;
    const auto last = badCharTable(pat);
    const auto shift = goodSuffixTable(pat, Count ? &r.preprocessing : nullptr);
    std::size_t s = 0;
    while (s + m <= n) {
        auto j = static_cast<std::ptrdiff_t>(m) - 1;  // порівняння справа наліво
        while (j >= 0) {
            if constexpr (Count) ++r.comparisons;
            if (pat[j] != text[s + j]) break;
            --j;
        }
        if (j < 0) {
            r.matches.push_back(s);
            s += shift[0];
        } else {
            const std::ptrdiff_t bc = j - last[static_cast<unsigned char>(text[s + j])];
            const auto gs = static_cast<std::ptrdiff_t>(shift[j + 1]);
            s += static_cast<std::size_t>(std::max(bc, gs));
        }
    }
    return r;
}

template SearchResult naiveSearch<true>(std::string_view, std::string_view);
template SearchResult naiveSearch<false>(std::string_view, std::string_view);
template SearchResult kmpSearch<true>(std::string_view, std::string_view);
template SearchResult kmpSearch<false>(std::string_view, std::string_view);
template SearchResult boyerMooreSearch<true>(std::string_view, std::string_view);
template SearchResult boyerMooreSearch<false>(std::string_view, std::string_view);

}  // namespace strsearch
