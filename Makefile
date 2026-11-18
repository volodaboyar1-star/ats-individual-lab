CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS += -Iinclude

LIB_SRC := src/search.cpp src/encoding.cpp src/bench.cpp

CLANG_FORMAT ?= clang-format
CLANG_TIDY   ?= clang-tidy
FORMAT_FILES := $(wildcard src/*.cpp include/*.h tests/*.cpp)
TIDY_FILES   := $(wildcard src/*.cpp tests/*.cpp)

all: strsearch run_tests

strsearch: $(LIB_SRC) src/main.cpp $(wildcard include/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LIB_SRC) src/main.cpp -o $@

run_tests: $(LIB_SRC) tests/tests.cpp $(wildcard include/*.h)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LIB_SRC) tests/tests.cpp -o $@

test: run_tests
	./run_tests

bench: strsearch
	./strsearch bench

# Перевірка формату (без змін файлів); ненульовий код виходу, якщо є розбіжності
format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_FILES)

# Автоматичне форматування за .clang-format
format:
	$(CLANG_FORMAT) -i $(FORMAT_FILES)

# Статичний аналіз за .clang-tidy
lint:
	$(CLANG_TIDY) --quiet --warnings-as-errors='*' $(TIDY_FILES) -- $(CPPFLAGS) -std=c++20

check: format-check lint test

clean:
	rm -f strsearch run_tests

.PHONY: all test bench clean format format-check lint check
