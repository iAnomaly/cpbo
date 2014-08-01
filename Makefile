DEFS = _NIX

INC_PATH ?= /usr/local/boost_1_55_0
LIB_PATH ?= $(INC_PATH)/stage/lib
OBJ_PATH ?= out
BIN_PATH ?= bin

CPPFLAGS := -I"${INC_PATH}"
CXXFLAGS :=
LDFLAGS := -L"${LIB_PATH}"

OBJS := ${addprefix $(OBJ_PATH)/,main.o pbo.o sha1.o}

${BIN_PATH}/cpbo: ${OBJS} | ${BIN_PATH}
	${CXX} ${LDFLAGS} -o ${BIN_PATH}/cpbo ${OBJS} \
	-lboost_filesystem \
	-lboost_system

${OBJ_PATH}/%.o: %.cpp
	${CXX} ${CXXFLAGS} ${CPPFLAGS} -c -D ${DEFS} -I"${INC_PATH}/" $< -o $@

${OBJS}: | ${OBJ_PATH}

${OBJ_PATH}:
	mkdir ${OBJ_PATH}

${BIN_PATH}:
	mkdir ${BIN_PATH}

clean:
	rm -rf ${OBJ_PATH}
