# 编译器设置
CC = gcc
CFLAGS = -Iinclude -Wall -Wextra -g -O2
LDFLAGS = -lcurl -pthread -lrt

# 目录定义
SRC_DIR = gateway/src
INC_DIR = gateway/include
OBJ_DIR = gateway/obj
BIN_DIR = gateway/bin

# 自动获取所有的源文件和目标文件
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SOURCES))
TARGET = $(BIN_DIR)/sentinel_edge_iot

# 默认规则
all: directories $(TARGET)

# 创建必要的目录
directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

# 链接目标文件
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

# 编译源文件为目标文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理规则
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# 伪目标
.PHONY: all clean directories