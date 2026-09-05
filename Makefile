.PHONY: all setup-lib dist run test ctest unittest clean backend valgrind-ctest valgrind-unittest valgrind-test pylint-ctest pylint-test help

C_DIR := c
PY_DIR := python
ASSETS_DIR := $(C_DIR)/assets
C_LIB_DIR := $(C_DIR)/lib
PY_DIST_DIR := $(PY_DIR)/dist
BUILD_DIR := $(C_DIR)/build
VENV_PYTHON := .venv/bin/python
PYTHON := $(shell if [ -x "$(VENV_PYTHON)" ]; then echo "$(VENV_PYTHON)"; else echo python3; fi)

# Default target
all: dist

# Environment variables for Python
export PYTHONPATH := $(PWD)/$(PY_DIR):$(PYTHONPATH)
export LD_LIBRARY_PATH := $(PWD)/$(PY_DIST_DIR):$(LD_LIBRARY_PATH)

# C compiler settings
CC := $(shell command -v clang >/dev/null 2>&1 && echo clang || command -v gcc >/dev/null 2>&1 && echo gcc || echo cc)
CFLAGS := -Wall -Wextra -Werror -fPIC -I$(C_DIR)/include
LDFLAGS := -shared

# C test framework check
CHECK_AVAILABLE := $(shell pkg-config --exists check 2>/dev/null && echo yes || echo no)
VALGRIND_AVAILABLE := $(shell command -v valgrind >/dev/null 2>&1 && echo yes || echo no)
VALGRIND_FLAGS := --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1
PYLINT_AVAILABLE := $(shell $(PYTHON) -c "import pylint" >/dev/null 2>&1 && echo yes || echo no)
PY_LINT_TARGETS := $(PY_DIR)/habits $(PY_DIR)/tests $(PY_DIR)/run_integration.py
ifeq ($(CHECK_AVAILABLE),yes)
CHECK_CFLAGS := $(shell pkg-config --cflags check)
CHECK_LDFLAGS := $(shell pkg-config --libs check)
endif

# C source files and object files
C_SOURCES := $(wildcard $(C_DIR)/src/*.c)
C_OBJECTS := $(patsubst $(C_DIR)/src/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
C_LIB := $(C_LIB_DIR)/libhabittracker.so

# C test files
C_TEST_DIR := $(C_DIR)/tests
C_TEST_BUILD_DIR := $(C_DIR)/build/test
C_TEST_SOURCES := $(wildcard $(C_TEST_DIR)/*.c)
C_TEST_OBJECTS := $(patsubst $(C_TEST_DIR)/%.c,$(C_TEST_BUILD_DIR)/%.o,$(C_TEST_SOURCES))
C_TEST_EXECUTABLE := $(C_TEST_BUILD_DIR)/test_runner

backend: $(C_LIB)
$(C_LIB): $(C_OBJECTS)
	@mkdir -p $(C_DIR)/lib
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(C_DIR)/src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

# Build C test object files (only if check library is available)
ifeq ($(CHECK_AVAILABLE),yes)
$(C_TEST_BUILD_DIR)/%.o: $(C_TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CHECK_CFLAGS) -c -o $@ $<

$(C_TEST_EXECUTABLE): $(C_TEST_OBJECTS) $(C_LIB)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $(C_TEST_OBJECTS) -L$(C_LIB_DIR) -lhabittracker $(CHECK_LDFLAGS) -lm -lpthread
endif

ctest: backend
	@if [ "$(CHECK_AVAILABLE)" = "no" ]; then \
		echo ""; \
		echo "⚠️  SKIPPING C TESTS: Check library not installed"; \
		echo ""; \
		echo "To install check library:"; \
		echo "  macOS:          brew install check"; \
		echo "  Ubuntu/Debian:  sudo apt-get install check libcheck-dev"; \
		echo "  Fedora/RHEL:    sudo dnf install check check-devel"; \
		echo ""; \
		echo "Or use Docker container: 'Reopen in Container' in VS Code"; \
		echo ""; \
	else \
		echo "Running C unit tests..."; \
		mkdir -p build/tests; \
		$(MAKE) $(C_TEST_EXECUTABLE) && LD_LIBRARY_PATH="$(PWD)/$(C_LIB_DIR):$$LD_LIBRARY_PATH" $(C_TEST_EXECUTABLE); \
	fi

valgrind-ctest: backend
	@if [ "$(CHECK_AVAILABLE)" = "no" ]; then \
		echo ""; \
		echo "SKIPPING VALGRIND C TESTS: Check library not installed"; \
		echo "Install with: sudo apt-get install check libcheck-dev"; \
		echo ""; \
	elif [ "$(VALGRIND_AVAILABLE)" = "no" ]; then \
		echo ""; \
		echo "SKIPPING VALGRIND C TESTS: valgrind is not installed"; \
		echo "Install with: sudo apt-get install valgrind"; \
		echo ""; \
	else \
		echo "Running C unit tests under valgrind..."; \
		mkdir -p build/tests; \
		$(MAKE) $(C_TEST_EXECUTABLE) && LD_LIBRARY_PATH="$(PWD)/$(C_LIB_DIR):$$LD_LIBRARY_PATH" valgrind $(VALGRIND_FLAGS) $(C_TEST_EXECUTABLE); \
	fi

valgrind-unittest: dist
	@if [ "$(VALGRIND_AVAILABLE)" = "no" ]; then \
		echo ""; \
		echo "SKIPPING VALGRIND PYTHON UNIT TESTS: valgrind is not installed"; \
		echo "Install with: sudo apt-get install valgrind"; \
		echo ""; \
	else \
		echo "Running Python unit tests under valgrind..."; \
		LD_LIBRARY_PATH="$(PWD)/$(PY_DIST_DIR):$$LD_LIBRARY_PATH" valgrind $(VALGRIND_FLAGS) $(PYTHON) -m unittest discover "$(PY_DIR)/tests" -v; \
	fi

valgrind-test: valgrind-ctest valgrind-unittest
	@echo "Valgrind C and Python leak checks completed"

pylint-ctest: dist
	@if [ "$(PYLINT_AVAILABLE)" = "no" ]; then \
		echo ""; \
		echo "SKIPPING PYTHON LINT TESTS: pylint is not installed for $(PYTHON)"; \
		echo "Install with:"; \
		echo "  $(PYTHON) -m pip install pylint"; \
		echo ""; \
	else \
		echo "Running pylint checks for Python scripts..."; \
		$(PYTHON) -m pylint $(PY_LINT_TARGETS); \
	fi

pylint-test: pylint-ctest
	@echo "Python pylint checks completed"

setup-lib:
	mkdir -p $(C_LIB_DIR)
	@arch="$$(uname -m)"; \
	case "$$arch" in \
		x86_64|amd64) patterns='*x86_64*.so *amd64*.so' ;; \
		aarch64|arm64) patterns='*aarch64*.so *arm64*.so' ;; \
		*) patterns='*.so' ;; \
	esac; \
	selected=''; \
	for p in $$patterns; do \
		candidate="$$(find $(C_LIB_DIR) -maxdepth 1 -type f -name "$$p" 2>/dev/null | head -n 1)"; \
		if [ -n "$$candidate" ]; then selected="$$candidate"; break; fi; \
	done; \
	if [ -z "$$selected" ]; then \
		echo "No architecture-specific libdatagen.so found in $(C_LIB_DIR) for $$arch"; \
		exit 1; \
	fi; \
	cp "$$selected" "$(C_LIB_DIR)/libdatagen.so"

dist: setup-lib backend
	mkdir -p "$(PY_DIST_DIR)"
	cp -R "$(C_LIB_DIR)/." "$(PY_DIST_DIR)/"

run: dist
	$(PYTHON) "$(PY_DIR)/run_integration.py"

test: ctest pytest
	@echo "All tests completed"

pytest: dist
	$(PYTHON) -m unittest discover "$(PY_DIR)/tests" -v

clean:
	rm -f $(C_OBJECTS) $(C_LIB)
	rm -rf $(C_TEST_BUILD_DIR) $(C_TEST_EXECUTABLE)

help:
	@echo ""
	@echo "╔════════════════════════════════════════════════════════════════╗"
	@echo "║         Habit Tracker Project - Build & Test Targets          ║"
	@echo "╚════════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo " BUILD TARGETS:"
	@echo "  make all              Build distribution (default target)"
	@echo "  make backend          Compile C library (libhabittracker.so)"
	@echo "  make dist             Prepare distribution (C lib + Python dist)"
	@echo ""
	@echo " UNIT TEST TARGETS:"
	@echo "  make ctest            Run C unit tests (requires: check library)"
	@echo "  make unittest         Run Python unit tests"
	@echo "  make test             Run all unit tests (ctest + unittest)"
	@echo ""
	@echo " LINT & ANALYSIS TARGETS:"
	@echo "  make pylint-ctest     Check Python code with pylint (requires: pylint)"
	@echo "  make pylint-test      Alias for pylint-ctest"
	@echo ""
	@echo " MEMORY LEAK DETECTION TARGETS:"
	@echo "  make valgrind-ctest   Run C tests with valgrind (requires: valgrind)"
	@echo "  make valgrind-unittest Run Python tests with valgrind (requires: valgrind)"
	@echo "  make valgrind-test    Run C and Python tests with valgrind"
	@echo ""
	@echo " INTEGRATION & EXECUTION TARGETS:"
	@echo "  make run              Run Python integration tests"
	@echo ""
	@echo " MAINTENANCE TARGETS:"
	@echo "  make clean            Remove build artifacts and test executables"
	@echo "  make help             Display this help message"
	@echo ""
	@echo " ENVIRONMENT VARIABLES:"
	@echo "  PYTHON                Python interpreter (auto-detected from .venv or system)"
	@echo "  CC                    C compiler (auto-detected: clang, gcc, cc)"
	@echo "  LD_LIBRARY_PATH       Set to include C library path automatically"
	@echo ""
	@echo "💡 QUICK START:"
	@echo "  make                  Build and prepare distribution"
	@echo "  make test             Run all tests"
	@echo "  make run              Execute integration tests"
	@echo "  make clean            Clean everything"
	@echo ""
