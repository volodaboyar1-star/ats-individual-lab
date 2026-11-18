#include "bench.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <limits>
#include <random>
#include <set>
#include <sstream>
#include <string_view>
#include <vector>

#include "encoding.h"
#include "search.h"

namespace strsearch {
namespace {

constexpr int kAlgos = 3;
const char* kAlgoNames[kAlgos] = {"Naive", "KMP", "Boyer-Moore"};

using SearchFn = SearchResult (*)(std::string_view, std::string_view);
const SearchFn kCounted[kAlgos] = {naiveSearch<true>, kmpSearch<true>, boyerMooreSearch<true>};
const SearchFn kTimed[kAlgos] = {naiveSearch<false>, kmpSearch<false>, boyerMooreSearch<false>};

struct Row {
    std::size_t length = 0;
    int sigma = 0;
    double cmp[kAlgos]{};     // середня кількість порівнянь під час пошуку
    double pre[kAlgos]{};     // середня кількість порівнянь при препроцесингу
    double timeMs[kAlgos]{};  // середній час одного пошуку, мс
    double matches = 0;       // середня кількість знайдених входжень
};

volatile std::size_t gSink = 0;  // не дає компілятору викинути "непотрібні" пошуки

Row measure(std::string_view text, const std::vector<std::string>& patterns, int repeats) {
    Row row;
    for (const auto& p : patterns) {
        std::size_t matchesRef = 0;
        for (int a = 0; a < kAlgos; ++a) {
            const auto r = kCounted[a](text, p);
            row.cmp[a] += static_cast<double>(r.comparisons);
            row.pre[a] += static_cast<double>(r.preprocessing);
            if (a == 0)
                matchesRef = r.matches.size();
            else if (r.matches.size() != matchesRef)
                std::fprintf(stderr, "ПОМИЛКА: %s дав іншу кількість входжень!\n", kAlgoNames[a]);

            double best = std::numeric_limits<double>::max();
            for (int k = 0; k < repeats; ++k) {
                const auto t0 = std::chrono::steady_clock::now();
                const auto rr = kTimed[a](text, p);
                const auto t1 = std::chrono::steady_clock::now();
                gSink = gSink + rr.matches.size();
                best = std::min(best, std::chrono::duration<double, std::milli>(t1 - t0).count());
            }
            row.timeMs[a] += best;
        }
        row.matches += static_cast<double>(matchesRef);
    }
    const auto k = static_cast<double>(patterns.size());
    for (int a = 0; a < kAlgos; ++a) {
        row.cmp[a] /= k;
        row.pre[a] /= k;
        row.timeMs[a] /= k;
    }
    row.matches /= k;
    return row;
}

char alphabetSymbol(int sigma, int k) {
    if (sigma == 2) return static_cast<char>('0' + k);
    if (sigma <= 26) return static_cast<char>('a' + k);
    return static_cast<char>(0x21 + k);
}

std::string randomString(std::size_t n, int sigma, std::mt19937& rng) {
    std::uniform_int_distribution<int> d(0, sigma - 1);
    std::string s(n, ' ');
    for (auto& c : s) c = alphabetSymbol(sigma, d(rng));
    return s;
}

void printHeader(const char* title, std::size_t n, bool withSigma) {
    std::printf("\n=== %s (n = %zu) ===\n", title, n);
    std::printf(
        "Порівняння на 1 символ тексту (comparisons / n) та середній час одного пошуку, мс\n");
    std::printf("%s  m | %-8s %-8s %-8s | %-11s %-11s %-11s | входжень\n", withSigma ? "  σ  " : "",
                "Naive", "KMP", "BM", "Naive,мс", "KMP,мс", "BM,мс");
}

void printRow(const Row& r, std::size_t n, bool withSigma, const char* note = "") {
    if (withSigma) std::printf("%4d ", r.sigma);
    std::printf("%3zu | %-8.4f %-8.4f %-8.4f | %-9.3f %-9.3f %-9.3f | %.1f\n", r.length,
                r.cmp[0] / n, r.cmp[1] / n, r.cmp[2] / n, r.timeMs[0], r.timeMs[1], r.timeMs[2],
                r.matches);
    if (*note) std::printf("      %s\n", note);
}

void writeCsv(const std::string& path, const std::vector<Row>& rows, std::size_t n) {
    std::ofstream f(path);
    f << "sigma,m,n,naive_cmp,kmp_cmp,bm_cmp,naive_pre,kmp_pre,bm_pre,"
         "naive_cmp_per_char,kmp_cmp_per_char,bm_cmp_per_char,naive_ms,kmp_ms,bm_ms,matches\n";
    for (const auto& r : rows) {
        f << r.sigma << ',' << r.length << ',' << n;
        for (double v : r.cmp) f << ',' << v;
        for (double v : r.pre) f << ',' << v;
        for (double v : r.cmp) f << ',' << v / n;
        for (double v : r.timeMs) f << ',' << v;
        f << ',' << r.matches << '\n';
    }
}

}  // namespace

int runBenchmarks(const BenchConfig& cfg) {
    std::filesystem::create_directories(cfg.outDir);
    std::mt19937 rng(cfg.seed);
    const std::vector<std::size_t> lengths = {5, 10, 15, 20, 25, 30, 35, 40, 45, 50};

    // ---------- 1. Бінарний текст ----------
    {
        const std::string text = randomString(cfg.randomTextSize, 2, rng);
        std::vector<Row> rows;
        printHeader("Середовище 1: випадковий бінарний зразок у випадковому бінарному тексті",
                    text.size(), false);
        for (auto m : lengths) {
            std::vector<std::string> pats;
            pats.reserve(static_cast<std::size_t>(cfg.patternsPerLength));
            for (int i = 0; i < cfg.patternsPerLength; ++i) pats.push_back(randomString(m, 2, rng));
            Row r = measure(text, pats, cfg.timingRepeats);
            r.length = m;
            r.sigma = 2;
            printRow(r, text.size(), false);
            rows.push_back(r);
        }
        writeCsv(cfg.outDir + "/binary.csv", rows, text.size());
    }

    // ---------- 2. Природомовний текст ----------
    {
        std::ifstream in(cfg.naturalTextPath, std::ios::binary);
        if (!in) {
            std::fprintf(stderr, "Не вдалося відкрити %s\n", cfg.naturalTextPath.c_str());
            return 1;
        }
        std::stringstream ss;
        ss << in.rdbuf();
        const std::string text = utf8ToCp1251(ss.str());
        std::set<unsigned char> alphabet(text.begin(), text.end());

        // Слова тексту: (початок, довжина)
        std::vector<std::pair<std::size_t, std::size_t>> words;
        for (std::size_t i = 0; i < text.size();) {
            if (!isLetterCp1251(static_cast<unsigned char>(text[i]))) {
                ++i;
                continue;
            }
            std::size_t j = i;
            while (j < text.size() && isLetterCp1251(static_cast<unsigned char>(text[j]))) ++j;
            words.emplace_back(i, j - i);
            i = j;
        }
        std::printf("\nТекст: %s, %zu символів, %zu слів, алфавіт тексту: %zu різних символів\n",
                    cfg.naturalTextPath.c_str(), text.size(), words.size(), alphabet.size());

        std::vector<Row> rows;
        printHeader("Середовище 2: випадкове слово в українському тексті", text.size(), false);
        for (auto m : lengths) {
            std::vector<std::size_t> exact;  // індекси слів довжини рівно m
            for (std::size_t w = 0; w < words.size(); ++w)
                if (words[w].second == m) exact.push_back(w);
            std::vector<std::string> pats;
            for (int i = 0; i < cfg.patternsPerLength; ++i) {
                if (exact.size() >= 10) {
                    const auto& w = words[exact[std::uniform_int_distribution<std::size_t>(
                        0, exact.size() - 1)(rng)]];
                    pats.push_back(text.substr(w.first, w.second));
                } else {
                    // Слів такої довжини (майже) немає - беремо фрагмент тексту довжини m,
                    // що починається з випадкового слова (словосполучення).
                    std::size_t start;
                    do {
                        start = words[std::uniform_int_distribution<std::size_t>(
                                          0, words.size() - 1)(rng)]
                                    .first;
                    } while (start + m > text.size() ||
                             text.find('\n', start) < start + m);  // у межах абзацу
                    pats.push_back(text.substr(start, m));
                }
            }
            Row r = measure(text, pats, cfg.timingRepeats);
            r.length = m;
            r.sigma = static_cast<int>(alphabet.size());
            const std::string note =
                std::string(
                    exact.size() >= 10
                        ? "зразки - слова, напр.: \""
                        : "зразки - фрагменти тексту (слів такої довжини мало), напр.: \"") +
                cp1251ToUtf8(pats[0]) + "\"";
            printRow(r, text.size(), false, note.c_str());
            rows.push_back(r);
        }
        writeCsv(cfg.outDir + "/natural.csv", rows, text.size());
    }

    // ---------- 3. Вплив розміру алфавіту ----------
    {
        const std::vector<int> sigmas = {2, 4, 8, 16, 32, 64, 128};
        const std::vector<std::size_t> lens = {5, 10, 20, 50};
        std::vector<Row> rows;
        printHeader("Експеримент 3: вплив розміру алфавіту (випадковий текст і зразок)",
                    cfg.randomTextSize, true);
        for (int sigma : sigmas) {
            const std::string text = randomString(cfg.randomTextSize, sigma, rng);
            for (auto m : lens) {
                std::vector<std::string> pats;
                for (int i = 0; i < cfg.patternsPerLength / 2; ++i) {
                    // половина зразків - фрагменти тексту (гарантоване входження)
                    if (i % 2 == 0) {
                        const auto pos =
                            std::uniform_int_distribution<std::size_t>(0, text.size() - m)(rng);
                        pats.push_back(text.substr(pos, m));
                    } else {
                        pats.push_back(randomString(m, sigma, rng));
                    }
                }
                Row r = measure(text, pats, cfg.timingRepeats);
                r.length = m;
                r.sigma = sigma;
                printRow(r, text.size(), true);
                rows.push_back(r);
            }
        }
        writeCsv(cfg.outDir + "/alphabet.csv", rows, cfg.randomTextSize);
    }
    std::printf("\nCSV-файли з результатами збережено в теці \"%s\".\n", cfg.outDir.c_str());
    return 0;
}

}  // namespace strsearch
