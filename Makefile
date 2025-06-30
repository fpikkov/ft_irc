# --------	MAKE VARIABLES	--------
RED = \033[1;31m
GREEN = \033[1;32m
CYAN = \033[1;36m
YELLOW = \033[1;33m
CLEAR = \033[0m

CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++20
INC_FLAGS = -I${INCLUDE_DIR}

BUILD_DIR = ./build
INCLUDE_DIR = ./include
SRC_DIR = ./src
OBJ_DIR = ./obj

NAME = ft_irc

BUILD = ${BUILD_DIR}/${NAME}

# Add more subdirectories in /src when required
VPATH = ${SRC_DIR}/main

SRCS = main.cpp

OBJS = ${SRCS:%.cpp=${OBJ_DIR}/%.o}

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

clean:
	@echo "${RED}Cleaning object files...${CLEAR}"
	@rm -rf ${OBJ_DIR}

fclean: clean
	@echo "${RED}Fully cleaning project...${CLEAR}"
	@rm -rf ${BUILD_DIR}

re: fclean all

.PHONY: all re clean fclean
