# Multi-Language Professional Standards - birds_of_play

## 🌍 Complete Professional Setup for C++, Python, Java

### 🎯 The "Big 4" Tools for Each Language:

| Language | **Formatter** | **Linter** | **Testing** | **Coverage** |
|----------|---------------|------------|-------------|--------------|
| **C++** | clang-format | clang-tidy | Google Test | gcov/lcov |
| **Python** | black | pylint/flake8 | pytest | coverage.py |
| **Java** | google-java-format | SpotBugs/PMD | JUnit | JaCoCo |

---

## 🐍 PYTHON STANDARDS

### Install Tools:
```bash
pip install black pylint flake8 pytest coverage mypy
```

### Configuration Files:

#### `pyproject.toml` (Python Config)
```toml
[tool.black]
line-length = 100
target-version = ['py38']

[tool.coverage.run]
source = ["src"]
omit = ["*/tests/*", "*/test_*"]

[tool.coverage.report]
show_missing = true
fail_under = 90

[tool.pytest.ini_options]
testpaths = ["tests"]
python_files = "test_*.py"
addopts = "--cov=src --cov-report=html --cov-report=term-missing"
```

#### `.flake8` (Python Linting)
```ini
[flake8]
max-line-length = 100
exclude = .git,__pycache__,build,dist
ignore = E203,W503
```

### Python Commands:
```bash
black .                    # Format code
pylint src/                # Static analysis
flake8 .                  # Style checking
mypy src/                 # Type checking
pytest --cov=src          # Run tests with coverage
```

---

## ☕ JAVA STANDARDS

### Install Tools:
```bash
# Maven dependencies (add to pom.xml)
# Or install via package manager:
brew install maven spotbugs  # macOS
```

#### `pom.xml` (Maven Config)
```xml
<properties>
    <maven.compiler.source>17</maven.compiler.source>
    <maven.compiler.target>17</maven.compiler.target>
    <junit.version>5.9.2</junit.version>
</properties>

<dependencies>
    <dependency>
        <groupId>org.junit.jupiter</groupId>
        <artifactId>junit-jupiter</artifactId>
        <version>${junit.version}</version>
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
        </plugin>
    </plugins>
</build>
```

#### `checkstyle.xml` (Java Style)
```xml
<?xml version="1.0"?>
<!DOCTYPE module PUBLIC
    "-//Checkstyle//DTD Checkstyle Configuration 1.3//EN"
    "https://checkstyle.org/dtds/configuration_1_3.dtd">
<module name="Checker">
    <module name="TreeWalker">
        <module name="Indentation">
            <property name="basicOffset" value="4"/>
        </module>
        <module name="LineLength">
            <property name="max" value="100"/>
        </module>
    </module>
</module>
```

### Java Commands:
```bash
mvn compile                      # Build
mvn test                        # Run tests
mvn jacoco:report               # Coverage report
mvn spotbugs:check              # Static analysis
mvn checkstyle:check            # Style checking
```

---

## 🔧 UNIFIED PROJECT STRUCTURE

```
birds_of_play/
├── src/
│   ├── cpp/                    # C++ source
│   ├── python/                 # Python source  
│   └── java/                   # Java source
├── tests/
│   ├── cpp/                    # C++ tests
│   ├── python/                 # Python tests
│   └── java/                   # Java tests
├── config/                     # All config files
│   ├── .clang-format
│   ├── .clang-tidy
│   ├── pyproject.toml
│   ├── .flake8
│   ├── pom.xml
│   └── checkstyle.xml
└── scripts/                    # Build/test scripts
    ├── test_all.sh
    ├── format_all.sh
    └── lint_all.sh
```

---

## 🚀 MASTER COMMANDS (All Languages)

### Universal Makefile:
```makefile
# Test all languages
test-all:
	make test-cpp && make test-python && make test-java

# Format all languages  
format-all:
	clang-format -i src/cpp/**/*.cpp src/cpp/**/*.h
	black src/python/
	google-java-format -i src/java/**/*.java

# Lint all languages
lint-all:
	clang-tidy src/cpp/**/*.cpp
	pylint src/python/
	mvn spotbugs:check

# Coverage for all languages
coverage-all:
	make coverage-cpp && make coverage-python && make coverage-java
```

---

## 📊 CI/CD PIPELINE (.github/workflows/ci.yml)

```yaml
name: Multi-Language CI
on: [push, pull_request]

jobs:
  cpp:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - name: Install C++ tools
        run: sudo apt-get install clang-tools-extra libgtest-dev lcov
      - name: Test C++
        run: make test-cpp

  python:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-python@v4
        with:
          python-version: '3.9'
      - name: Install Python tools
        run: pip install black pylint pytest coverage
      - name: Test Python
        run: pytest --cov=src/python

  java:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-java@v3
        with:
          java-version: '17'
          distribution: 'temurin'
      - name: Test Java
        run: mvn test jacoco:report
```

---

## ✅ PRODUCTION CHECKLIST (All Languages)

### Before Every Commit:
- [ ] **C++**: `make format && make tidy && make test`
- [ ] **Python**: `black . && pylint src/ && pytest --cov=src`
- [ ] **Java**: `mvn checkstyle:check && mvn test && mvn jacoco:report`

### Quality Gates:
- [ ] **90%+ test coverage** in all languages
- [ ] **Zero linter warnings** in all languages
- [ ] **All tests passing** in all languages
- [ ] **Consistent formatting** across all languages

---

## 🏢 ENTERPRISE INTEGRATION

### IDE Setup:
- **VSCode**: Install extensions for each language's formatter/linter
- **IntelliJ**: Configure for Java, Python, C++ with respective plugins
- **Vim/Neovim**: Configure with language servers (clangd, pylsp, jdtls)

### Pre-commit Hooks:
```bash
# Install pre-commit
pip install pre-commit

# .pre-commit-config.yaml
repos:
  - repo: local
    hooks:
      - id: clang-format
        name: clang-format
        entry: clang-format -i
        language: system
        files: \.(cpp|h)$
      - id: black
        name: black
        entry: black
        language: python
        files: \.py$
      - id: java-format
        name: google-java-format
        entry: google-java-format -i
        language: system
        files: \.java$
```

**This setup gives you Google/Microsoft-level code quality across all three languages!** 🎯