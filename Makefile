# --------	MAKE VARIABLES	--------
RED = \033[1;31m
GREEN = \033[1;32m
CYAN = \033[1;36m
YELLOW = \033[1;33m
CLEAR = \033[0m

# Compilation flags
CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++20 -MMD -MP
INC_FLAGS = -I${INCLUDE_DIR}

# Build type flags
BUILD_TYPE ?= default

ifeq ($(BUILD_TYPE), default)
# No compilation flags added
else ifeq ($(BUILD_TYPE), release)
	CXXFLAGS += -O2 -march=native
else ifeq ($(BUILD_TYPE), debug)
	CXXFLAGS += -g -O0 -DDEBUG
else
	$(error Unknown build type: $(BUILD_TYPE). Available types are "default", "release" and "debug".)
endif

# Enable parallel compilation
MAKEFLAGS += -j$(shell nproc)

# Directories
BUILD_DIR = ./build
INCLUDE_DIR = ./include
SRC_DIR = ./src
OBJ_DIR = ./obj

NAME = ircserv
BUILD = ${BUILD_DIR}/${NAME}

# Add more subdirectories in /src when required
VPATH = ${SRC_DIR}

SRCS =	main.cpp \
		Server.cpp \
		Client.cpp \
		Logger.cpp \
		Response.cpp \
		Channels.cpp \
		Command.cpp \
		CommandHandler.cpp \
		CommandBroadcast.cpp \
		CommandHelpers.cpp \
		CommandModes.cpp \

OBJS = ${SRCS:%.cpp=${OBJ_DIR}/%.o}

DEPS = ${OBJS:.o=.d}

# --------	MAKE TARGETS	--------
all: ${BUILD}

${BUILD}: ${OBJS}
	@echo "${GREEN}Generating build...${CLEAR}"
	@mkdir -p ${BUILD_DIR}
	@${CXX} ${CXXFLAGS} -o $@ $^
	@echo "${GREEN}Executable ${YELLOW}${NAME}${GREEN} was created in ${YELLOW}"${BUILD_DIR}"${CLEAR}"

${OBJ_DIR}/%.o : %.cpp
	@echo "${CYAN}Generating object files...${CLEAR}"
	@mkdir -p ${OBJ_DIR}
	@$(CXX) $(CXXFLAGS) -c $< -o $@ ${INC_FLAGS}

# Build types
default:
	@$(MAKE) BUILD_TYPE=default

release: clean
	@$(MAKE) BUILD_TYPE=release --no-print-directory

debug: clean
	@$(MAKE) BUILD_TYPE=debug --no-print-directory

# Include dependency files
-include ${DEPS}

clean:
	@echo "${RED}Cleaning object files...${CLEAR}"
	@rm -rf ${OBJ_DIR}

fclean: clean
	@echo "${RED}Fully cleaning project...${CLEAR}"
	@rm -rf ${BUILD_DIR}

re: fclean all

.PHONY: all re clean fclean release debug
