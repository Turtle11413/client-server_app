CC=gcc -std=c++17
FLAGS=-Wall -Werror -Wextra -lstdc++
APP_NAME=client
BUILD_DIR=build
OS:=$(shell uname -s)

ifeq ($(OS),Linux)
  OPEN_CMD=xdg-open
	LEAKS_CMD=valgrind --leak-check=full \
						--show-leak-kinds=all \
						--track-origins=yes
	ADD_LIB=-lgtest -lgtest_main -lpthread -lm
	GCOV_FLAGS=
endif

ifeq ($(OS),Darwin)
	OPEN_CMD=open -a "Google Chrome"
	LEAKS_CMD=leaks -atExit --
	ADD_LIB=-lgtest -lgtest_main -lm
	GCOV_FLAGS=--ignore-errors mismatch
endif

all: install run_client

rebuild: uninstall install run_client

install:
	@qmake -o QMakefile
	@make -f QMakefile all

uninstall: clean
	@rm -rf $(BUILD_DIR)
	@rm -rf QMakefile

run_client: install
ifeq ($(OS),Darwin)
	./$(BUILD_DIR)/$(APP_NAME).app/Contents/MacOS/$(APP_NAME)
endif
ifeq ($(OS),Linux)
	./$(BUILD_DIR)/$(APP_NAME)
endif

style:
	@clang-format -style=Google -i *.cc *.h */*.h */*.cc

check_style:
	@clang-format -style=Google -n *.cc *.h */*.h */*.cc

clean:
	@rm -rf *.o *.gcno *.gcda *.info *.gcov *.a *.dSYM report *.out
