# Professional C++ Standards - Copy & Paste Template

## 🚀 Quick Setup (30 seconds)

### 1. Install Tools (One-time)
```bash
# macOS
brew install llvm googletest lcov

# Ubuntu/Debian  
sudo apt install clang-tools-extra libgtest-dev lcov

# Windows (with vcpkg)
vcpkg install gtest
```

### 2. Copy These 4 Files to Any Project:

#### `.clang-format` (Code Style)
```yaml
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 100
```

#### `.clang-tidy` (Bug Detection)
```yaml
Checks: 'bugprone-*,cert-*,clang-analyzer-*,cppcoreguidelines-*,google-*,modernize-*,performance-*,readability-*'
```

#### `Makefile` (Build Commands)
```makefile
test:
	g++ -std=c++17 -Wall -Wextra --coverage *.cpp -lgtest -lgtest_main -pthread -o tests && ./tests
format:
	clang-format -i *.cpp *.h
tidy:
	clang-tidy *.cpp -- -std=c++17
coverage:
	gcov *.cpp && lcov --capture --directory . --output-file coverage.info
clean:
	rm -rf tests *.o *.gcda *.gcno coverage*
.PHONY: test format tidy coverage clean
```

#### `test_template.cpp` (Test Template)
```cpp
#include <gtest/gtest.h>
#include "your_class.h"

TEST(YourClassTest, BasicTest) {
    // Your test code here
    EXPECT_TRUE(true);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

## 🎯 Daily Commands:
```bash
make format    # Format code
make tidy      # Find bugs  
make test      # Run tests
make coverage  # Check coverage
```

## ✅ Production Checklist:
- [ ] Code formatted with clang-format
- [ ] Zero clang-tidy warnings
- [ ] All tests passing
- [ ] 90%+ code coverage
- [ ] No memory leaks (valgrind)

## 🏢 Industry Standards Met:
- **Google Style Guide** (clang-format)
- **Static Analysis** (clang-tidy) 
- **Unit Testing** (Google Test)
- **Code Coverage** (gcov/lcov)
- **Modern C++** (C++17+)

**This is the exact setup used at Google, Microsoft, Apple, etc.** 🎯