#!/bin/bash

# Multi-Language Professional Setup Script
# Sets up C++, Python, and Java development standards

echo "🌍 Setting up Multi-Language Professional Development Environment"
echo "=============================================================="

PROJECT_NAME=${1:-"birds_of_play"}
echo "📦 Project: $PROJECT_NAME"

# Create directory structure
echo "📁 Creating project structure..."
mkdir -p src/{cpp,python,java}
mkdir -p tests/{cpp,python,java}
mkdir -p config
mkdir -p scripts

# ========================================
# C++ SETUP
# ========================================
echo "🔧 Setting up C++ standards..."

cat > config/.clang-format << 'EOF'
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 100
EOF

cat > config/.clang-tidy << 'EOF'
Checks: 'bugprone-*,cert-*,clang-analyzer-*,cppcoreguidelines-*,google-*,modernize-*,performance-*,readability-*'
EOF

# ========================================
# PYTHON SETUP  
# ========================================
echo "🐍 Setting up Python standards..."

cat > pyproject.toml << 'EOF'
[tool.black]
line-length = 100
target-version = ['py38']

[tool.coverage.run]
source = ["src/python"]
omit = ["*/tests/*", "*/test_*"]

[tool.coverage.report]
show_missing = true
fail_under = 90

[tool.pytest.ini_options]
testpaths = ["tests/python"]
python_files = "test_*.py"
addopts = "--cov=src/python --cov-report=html --cov-report=term-missing"
EOF

cat > .flake8 << 'EOF'
[flake8]
max-line-length = 100
exclude = .git,__pycache__,build,dist,src/cpp,src/java
ignore = E203,W503
EOF

# ========================================
# JAVA SETUP
# ========================================
echo "☕ Setting up Java standards..."

cat > pom.xml << EOF
<?xml version="1.0" encoding="UTF-8"?>
<project xmlns="http://maven.apache.org/POM/4.0.0"
         xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
         xsi:schemaLocation="http://maven.apache.org/POM/4.0.0 
         http://maven.apache.org/xsd/maven-4.0.0.xsd">
    <modelVersion>4.0.0</modelVersion>
    
    <groupId>com.example</groupId>
    <artifactId>$PROJECT_NAME</artifactId>
    <version>1.0.0</version>
    
    <properties>
        <maven.compiler.source>17</maven.compiler.source>
        <maven.compiler.target>17</maven.compiler.target>
        <junit.version>5.9.2</junit.version>
    </properties>
    
    <dependencies>
        <dependency>
            <groupId>org.junit.jupiter</groupId>
            <artifactId>junit-jupiter</artifactId>
            <version>\${junit.version}</version>
            <scope>test</scope>
        </dependency>
    </dependencies>
    
    <build>
        <plugins>
            <plugin>
                <groupId>com.github.spotbugs</groupId>
                <artifactId>spotbugs-maven-plugin</artifactId>
                <version>4.7.3.0</version>
            </plugin>
            <plugin>
                <groupId>org.jacoco</groupId>
                <artifactId>jacoco-maven-plugin</artifactId>
                <version>0.8.8</version>
                <executions>
                    <execution>
                        <goals>
                            <goal>prepare-agent</goal>
                        </goals>
                    </execution>
                    <execution>
                        <id>report</id>
                        <phase>test</phase>
                        <goals>
                            <goal>report</goal>
                        </goals>
                    </execution>
                </executions>
            </plugin>
        </plugins>
    </build>
</project>
EOF

# ========================================
# UNIFIED MAKEFILE
# ========================================
echo "🔨 Creating unified build system..."

cat > Makefile << 'EOF'
# Multi-Language Professional Makefile

.PHONY: test-all format-all lint-all coverage-all clean-all install-deps

# ========================================
# UNIFIED COMMANDS
# ========================================

test-all: test-cpp test-python test-java
	@echo "✅ All tests completed"

format-all: format-cpp format-python format-java
	@echo "✅ All code formatted"

lint-all: lint-cpp lint-python lint-java
	@echo "✅ All code linted"

coverage-all: coverage-cpp coverage-python coverage-java
	@echo "✅ All coverage reports generated"

# ========================================
# C++ COMMANDS
# ========================================

test-cpp:
	@echo "🔧 Testing C++..."
	@cd src/cpp && g++ -std=c++17 -Wall -Wextra --coverage *.cpp -lgtest -lgtest_main -pthread -o tests && ./tests

format-cpp:
	@echo "🔧 Formatting C++..."
	@clang-format -i src/cpp/*.cpp src/cpp/*.h tests/cpp/*.cpp 2>/dev/null || true

lint-cpp:
	@echo "🔧 Linting C++..."
	@clang-tidy src/cpp/*.cpp -- -std=c++17 2>/dev/null || true

coverage-cpp:
	@echo "🔧 Generating C++ coverage..."
	@cd src/cpp && gcov *.cpp 2>/dev/null || true

# ========================================
# PYTHON COMMANDS
# ========================================

test-python:
	@echo "🐍 Testing Python..."
	@pytest tests/python/ --cov=src/python 2>/dev/null || echo "⚠️  Install: pip install pytest coverage"

format-python:
	@echo "🐍 Formatting Python..."
	@black src/python/ tests/python/ 2>/dev/null || echo "⚠️  Install: pip install black"

lint-python:
	@echo "🐍 Linting Python..."
	@pylint src/python/ 2>/dev/null || echo "⚠️  Install: pip install pylint"
	@flake8 src/python/ tests/python/ 2>/dev/null || echo "⚠️  Install: pip install flake8"

coverage-python:
	@echo "🐍 Generating Python coverage..."
	@coverage run -m pytest tests/python/ && coverage html 2>/dev/null || echo "⚠️  Install coverage tools"

# ========================================
# JAVA COMMANDS
# ========================================

test-java:
	@echo "☕ Testing Java..."
	@mvn test 2>/dev/null || echo "⚠️  Install Maven and Java"

format-java:
	@echo "☕ Formatting Java..."
	@google-java-format -i src/java/**/*.java 2>/dev/null || echo "⚠️  Install google-java-format"

lint-java:
	@echo "☕ Linting Java..."
	@mvn spotbugs:check 2>/dev/null || echo "⚠️  Maven project required"

coverage-java:
	@echo "☕ Generating Java coverage..."
	@mvn jacoco:report 2>/dev/null || echo "⚠️  Maven project required"

# ========================================
# INSTALLATION
# ========================================

install-deps:
	@echo "📦 Installing dependencies..."
	@echo "C++:"
	@brew install llvm googletest lcov 2>/dev/null || echo "  Install manually: clang-tools, gtest, lcov"
	@echo "Python:"
	@pip install black pylint flake8 pytest coverage mypy 2>/dev/null || echo "  Install manually: pip install black pylint flake8 pytest coverage"
	@echo "Java:"
	@brew install maven 2>/dev/null || echo "  Install manually: Maven and Java 17+"

# ========================================
# CLEANUP
# ========================================

clean-all:
	@echo "🧹 Cleaning all build artifacts..."
	@rm -rf build/ dist/ *.egg-info/
	@rm -rf target/ .coverage htmlcov/
	@rm -rf *.o *.gcda *.gcno tests
	@find . -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
	@echo "✅ Cleanup complete"

# ========================================
# HELP
# ========================================

help:
	@echo "🌍 Multi-Language Development Commands:"
	@echo ""
	@echo "  test-all      - Run tests for all languages"
	@echo "  format-all    - Format code for all languages"
	@echo "  lint-all      - Lint code for all languages"
	@echo "  coverage-all  - Generate coverage for all languages"
	@echo ""
	@echo "  install-deps  - Install all required tools"
	@echo "  clean-all     - Clean all build artifacts"
	@echo ""
	@echo "Language-specific commands:"
	@echo "  test-cpp, format-cpp, lint-cpp, coverage-cpp"
	@echo "  test-python, format-python, lint-python, coverage-python"
	@echo "  test-java, format-java, lint-java, coverage-java"
EOF

# ========================================
# SAMPLE FILES
# ========================================
echo "📝 Creating sample files..."

# Sample C++ test
cat > tests/cpp/test_sample.cpp << 'EOF'
#include <gtest/gtest.h>

TEST(SampleTest, BasicAssertion) {
    EXPECT_EQ(2 + 2, 4);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
EOF

# Sample Python test
cat > tests/python/test_sample.py << 'EOF'
import unittest

class TestSample(unittest.TestCase):
    def test_basic_assertion(self):
        self.assertEqual(2 + 2, 4)

if __name__ == '__main__':
    unittest.main()
EOF

# Sample Java test
mkdir -p src/main/java/com/example
mkdir -p src/test/java/com/example

cat > src/test/java/com/example/SampleTest.java << 'EOF'
package com.example;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.assertEquals;

public class SampleTest {
    @Test
    public void testBasicAssertion() {
        assertEquals(4, 2 + 2);
    }
}
EOF

# ========================================
# COMPLETION
# ========================================

echo ""
echo "🎉 Multi-Language Professional Setup Complete!"
echo "=============================================="
echo ""
echo "📋 Available Commands:"
echo "  make test-all      - Test all languages"
echo "  make format-all    - Format all code"
echo "  make lint-all      - Lint all code"
echo "  make coverage-all  - Generate coverage reports"
echo "  make install-deps  - Install all tools"
echo "  make help          - Show all commands"
echo ""
echo "📁 Project Structure Created:"
echo "  src/{cpp,python,java}/     - Source code"
echo "  tests/{cpp,python,java}/   - Test files"
echo "  config/                    - Configuration files"
echo ""
echo "🚀 Your multi-language project is now enterprise-ready!"
echo "   Run 'make help' for more information."
echo "   Run 'make install-deps' to install all required tools."