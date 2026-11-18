// Лабораторна робота №2, варіант 2.
// Пошук зразка в рядку: наївний алгоритм, КМП, Боєр-Мур.
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "bench.h"
#include "encoding.h"
#include "search.h"

using namespace strsearch;

namespace {

void runAll(const std::string& textCp, const std::string& patCp) {
    struct Algo {
        const char* name;
        SearchResult (*counted)(std::string_view, std::string_view);
        SearchResult (*timed)(std::string_view, std::string_view);
    };
    const Algo algos[] = {
        {"Наївний    ", naiveSearch<true>, naiveSearch<false>},
        {"КМП        ", kmpSearch<true>, kmpSearch<false>},
        {"Боєр-Мур   ", boyerMooreSearch<true>, boyerMooreSearch<false>},
    };
    std::printf("\nДовжина тексту: %zu, довжина зразка: %zu\n", textCp.size(), patCp.size());
    std::printf("Алгоритм    | входжень | порівнянь  | препроцесинг | час, мс\n");
    SearchResult first;
    for (const auto& a : algos) {
        const auto r = a.counted(textCp, patCp);
        const auto t0 = std::chrono::steady_clock::now();
        const auto rt = a.timed(textCp, patCp);
        const auto t1 = std::chrono::steady_clock::now();
        std::printf("%s | %8zu | %10llu | %12llu | %.4f\n", a.name, rt.matches.size(),
                    static_cast<unsigned long long>(r.comparisons),
                    static_cast<unsigned long long>(r.preprocessing),
                    std::chrono::duration<double, std::milli>(t1 - t0).count());
        if (first.matches.empty()) first = r;
    }
    if (first.matches.empty()) {
        std::printf("Зразок не знайдено.\n");
        return;
    }
    std::printf("Позиції входжень (перші 20):");
    for (std::size_t i = 0; i < first.matches.size() && i < 20; ++i)
        std::printf(" %zu", first.matches[i]);
    std::printf("\n");
    const std::size_t p = first.matches[0];
    const std::size_t from = p >= 30 ? p - 30 : 0;
    std::printf("Контекст першого входження: ...%s[%s]%s...\n",
                cp1251ToUtf8(textCp.substr(from, p - from)).c_str(),
                cp1251ToUtf8(textCp.substr(p, patCp.size())).c_str(),
                cp1251ToUtf8(textCp.substr(p + patCp.size(), 30)).c_str());
}

bool readFile(const std::string& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    std::stringstream ss;
    ss << in.rdbuf();
    out = utf8ToCp1251(ss.str());
    return true;
}

std::string ask(const char* prompt) {
    std::printf("%s", prompt);
    std::fflush(stdout);
    std::string s;
    std::getline(std::cin, s);
    return s;
}

void usage(const char* prog) {
    std::printf(
        "Використання:\n"
        "  %s                       інтерактивне меню\n"
        "  %s bench [файл.txt]      експериментальне порівняння алгоритмів\n"
        "  %s search <файл> <зразок> пошук зразка у файлі\n",
        prog, prog, prog);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc >= 2) {
        const std::string cmd = argv[1];
        if (cmd == "bench") {
            BenchConfig cfg;
            if (argc >= 3) cfg.naturalTextPath = argv[2];
            return runBenchmarks(cfg);
        }
        if (cmd == "search" && argc >= 4) {
            std::string text;
            if (!readFile(argv[2], text)) {
                std::printf("Не вдалося відкрити файл.\n");
                return 1;
            }
            runAll(text, utf8ToCp1251(argv[3]));
            return 0;
        }
        usage(argv[0]);
        return 1;
    }

    for (;;) {
        std::printf(
            "\n===== Пошук зразка в рядку (наївний / КМП / Боєр-Мур) =====\n"
            "1. Пошук зразка у введеному тексті\n"
            "2. Пошук зразка у текстовому файлі\n"
            "3. Експериментальне порівняння алгоритмів\n"
            "0. Вихід\n");
        const std::string choice = ask("Ваш вибір: ");
        if (!std::cin || choice == "0") break;
        if (choice == "1") {
            const auto text = ask("Текст: ");
            const auto pat = ask("Зразок: ");
            if (pat.empty()) {
                std::printf("Зразок не може бути порожнім.\n");
                continue;
            }
            runAll(utf8ToCp1251(text), utf8ToCp1251(pat));
        } else if (choice == "2") {
            auto path = ask("Шлях до файлу [data/uk_text.txt]: ");
            if (path.empty()) path = "data/uk_text.txt";
            std::string text;
            if (!readFile(path, text)) {
                std::printf("Не вдалося відкрити файл.\n");
                continue;
            }
            const auto pat = ask("Зразок: ");
            if (pat.empty()) {
                std::printf("Зразок не може бути порожнім.\n");
                continue;
            }
            runAll(text, utf8ToCp1251(pat));
        } else if (choice == "3") {
            BenchConfig cfg;
            const auto path = ask("Природомовний текст [data/uk_text.txt]: ");
            if (!path.empty()) cfg.naturalTextPath = path;
            runBenchmarks(cfg);
        } else {
            std::printf("Невідома команда.\n");
        }
    }
    return 0;
}
