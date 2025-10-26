# Makefile for ZYGL2 Project
# 资源管理系统编译文件

# 编译器配置
CXX = g++
CXXFLAGS = -std=c++17 -I. -Wall -Wextra -O2
LDFLAGS = -pthread

# Debug模式：使用 make DEBUG=1
ifdef DEBUG
    CXXFLAGS = -std=c++17 -I. -g -O0 -Wall -Wextra
endif

# 启用HTTPS：使用 make HTTPS=1
ifdef HTTPS
    CXXFLAGS += -DCPPHTTPLIB_OPENSSL_SUPPORT
    LDFLAGS += -lssl -lcrypto
endif

# 启用压缩：使用 make ZLIB=1
ifdef ZLIB
    CXXFLAGS += -DCPPHTTPLIB_ZLIB_SUPPORT
    LDFLAGS += -lz
endif

# 目标文件
TEST_DEPS_TARGET = test_dependencies
TEST_DOMAIN_TARGET = test_domain
MAIN_TARGET = zygl2

# 源文件
TEST_DEPS_SRC = test_dependencies.cpp
TEST_DOMAIN_SRC = test_domain.cpp
MAIN_SRC = src/main.cpp

# 所有头文件（用于依赖检查）
HEADERS = $(shell find src -name "*.h") \
          $(shell find third_party -name "*.h" -o -name "*.hpp")

# 默认目标
.PHONY: all
all: test_deps test_domain

# 编译测试程序
.PHONY: test_deps
test_deps: $(TEST_DEPS_TARGET)

$(TEST_DEPS_TARGET): $(TEST_DEPS_SRC) $(HEADERS)
	@echo "编译依赖库测试程序..."
	$(CXX) $(CXXFLAGS) $(TEST_DEPS_SRC) -o $(TEST_DEPS_TARGET) $(LDFLAGS)
	@echo "✅ $(TEST_DEPS_TARGET) 编译完成"

# 编译领域层测试
.PHONY: test_domain
test_domain: $(TEST_DOMAIN_TARGET)

$(TEST_DOMAIN_TARGET): $(TEST_DOMAIN_SRC) $(HEADERS)
	@echo "编译领域层测试程序..."
	$(CXX) $(CXXFLAGS) $(TEST_DOMAIN_SRC) -o $(TEST_DOMAIN_TARGET) $(LDFLAGS)
	@echo "✅ $(TEST_DOMAIN_TARGET) 编译完成"

# 编译主程序（需要先创建src/main.cpp）
.PHONY: main
main: $(MAIN_TARGET)

$(MAIN_TARGET): $(MAIN_SRC) $(HEADERS)
	@echo "编译主程序..."
	@if [ ! -f $(MAIN_SRC) ]; then \
		echo "❌ 错误: $(MAIN_SRC) 不存在"; \
		echo "请先创建主程序文件"; \
		exit 1; \
	fi
	$(CXX) $(CXXFLAGS) $(MAIN_SRC) -o $(MAIN_TARGET) $(LDFLAGS)
	@echo "✅ $(MAIN_TARGET) 编译完成"

# 运行测试
.PHONY: run_tests
run_tests: test_deps test_domain
	@echo ""
	@echo "=========================================="
	@echo "运行依赖库测试..."
	@echo "=========================================="
	./$(TEST_DEPS_TARGET)
	@echo ""
	@echo "=========================================="
	@echo "运行领域层测试..."
	@echo "=========================================="
	./$(TEST_DOMAIN_TARGET)
	@echo ""
	@echo "✅ 所有测试运行完成"

# 运行主程序
.PHONY: run
run: main
	@echo "运行主程序..."
	./$(MAIN_TARGET)

# 清理编译产物
.PHONY: clean
clean:
	@echo "清理编译产物..."
	rm -f $(TEST_DEPS_TARGET) $(TEST_DOMAIN_TARGET) $(MAIN_TARGET)
	rm -f *.o *.out *.exe
	rm -rf *.dSYM
	@echo "✅ 清理完成"

# 显示帮助信息
.PHONY: help
help:
	@echo "ZYGL2 项目 Makefile 使用说明"
	@echo ""
	@echo "基本目标："
	@echo "  make                 - 编译所有测试程序"
	@echo "  make all             - 同上"
	@echo "  make test_deps       - 只编译依赖库测试"
	@echo "  make test_domain     - 只编译领域层测试"
	@echo "  make main            - 编译主程序（需要src/main.cpp）"
	@echo "  make run_tests       - 编译并运行所有测试"
	@echo "  make run             - 编译并运行主程序"
	@echo "  make clean           - 清理所有编译产物"
	@echo "  make help            - 显示此帮助信息"
	@echo ""
	@echo "编译选项："
	@echo "  make DEBUG=1         - 使用Debug模式编译（带调试信息）"
	@echo "  make HTTPS=1         - 启用HTTPS支持（需要OpenSSL）"
	@echo "  make ZLIB=1          - 启用压缩支持（需要zlib）"
	@echo "  make CXX=clang++     - 使用不同的编译器"
	@echo ""
	@echo "组合使用："
	@echo "  make DEBUG=1 HTTPS=1    - Debug模式 + HTTPS支持"
	@echo "  make CXX=clang++ test_deps  - 使用clang编译测试"
	@echo ""
	@echo "多核编译："
	@echo "  make -j4             - 使用4核并行编译"
	@echo ""
	@echo "示例："
	@echo "  make                     # 编译测试程序"
	@echo "  make run_tests           # 运行所有测试"
	@echo "  make DEBUG=1             # Debug模式编译"
	@echo "  make clean && make       # 清理后重新编译"

# 显示编译配置
.PHONY: info
info:
	@echo "编译配置信息："
	@echo "  编译器: $(CXX)"
	@echo "  编译选项: $(CXXFLAGS)"
	@echo "  链接选项: $(LDFLAGS)"
	@echo "  源代码文件数: $(shell find src -name '*.h' | wc -l)"
	@echo "  第三方库: cpp-httplib, nlohmann/json"

# 检查代码风格（需要clang-format）
.PHONY: format
format:
	@echo "格式化代码..."
	@if command -v clang-format >/dev/null 2>&1; then \
		find src -name "*.h" -o -name "*.cpp" | xargs clang-format -i; \
		clang-format -i test_dependencies.cpp test_domain.cpp; \
		echo "✅ 代码格式化完成"; \
	else \
		echo "❌ 未找到clang-format，请先安装"; \
	fi

# 静态分析（需要cppcheck）
.PHONY: check
check:
	@echo "运行静态分析..."
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --std=c++17 --suppress=missingIncludeSystem src/; \
		echo "✅ 静态分析完成"; \
	else \
		echo "❌ 未找到cppcheck，请先安装"; \
	fi

# 安装（复制可执行文件到系统目录）
.PHONY: install
install: main
	@echo "安装主程序..."
	@if [ -f $(MAIN_TARGET) ]; then \
		sudo cp $(MAIN_TARGET) /usr/local/bin/; \
		echo "✅ 已安装到 /usr/local/bin/$(MAIN_TARGET)"; \
	else \
		echo "❌ 主程序未编译，请先运行 make main"; \
	fi

# 卸载
.PHONY: uninstall
uninstall:
	@echo "卸载主程序..."
	@sudo rm -f /usr/local/bin/$(MAIN_TARGET)
	@echo "✅ 卸载完成"

# 显示项目统计
.PHONY: stats
stats:
	@echo "项目统计信息："
	@echo "  源代码文件: $(shell find src -name '*.h' | wc -l) 个"
	@echo "  源代码行数: $(shell find src -name '*.h' -exec wc -l {} \; | awk '{sum+=$$1} END {print sum}') 行"
	@echo "  文档文件: $(shell ls docs/*.md 2>/dev/null | wc -l) 个"
	@echo "  测试文件: 2 个"
	@echo "  第三方库: 2 个 (cpp-httplib, nlohmann/json)"

# 伪目标声明
.PHONY: all test_deps test_domain main run_tests run clean help info format check install uninstall stats

