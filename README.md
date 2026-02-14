# OverWatch 🔒

A security tool that scans public repositories for exposed secrets and notifies repository owners.

## Why?

Developers often accidentally commit API keys, tokens, and credentials to public repositories. This tool helps identify these security issues and notifies owners so they can fix them.

## Goals

- **Learning:** Hands-on experience with C++, HTTP APIs, pattern matching, and security practices
- **Impact:** Help developers protect their credentials
- **Longevity:** Built with stable technologies to run for 5+ years

## Architecture

```
C++ Scanner → CSV → Python Bot → GitHub Issues
```

**C++ Scanner:**
- Searches GitHub for recently created, low-star repositories
- Scans files for exposed secrets (API keys, tokens, passwords)
- Outputs findings to CSV (no local repo downloads)

**Python Bot:**
- Reads findings from CSV
- Creates GitHub issues to alert repository owners
- Deletes processed entries from CSV

## Quick Start

### Prerequisites
- C++17 compiler (GCC/Clang)
- CMake 3.15+
- vcpkg (for C++ dependencies)
- Python 3.8+
- GitHub account (for API access)

### Setup

#### Option 1: NixOS / Nix Package Manager (Recommended)
```bash
# 1. Clone repository
git clone https://github.com/yourusername/OverWatch.git
cd OverWatch

# 2. Enter development shell (installs all dependencies)
nix-shell

# 3. Create .env file (NEVER COMMIT THIS)
cp .env.example .env
# Edit .env and add your GitHub token

# 4. Build C++ scanner
cd scanner
cmake -B build -S .
cmake --build build
cd ..

# 5. Setup Python bot
cd bot
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
cd ..
```

#### Option 2: Traditional Linux/macOS
```bash
# 1. Clone repository
git clone https://github.com/yourusername/OverWatch.git
cd OverWatch

# 2. Install dependencies
# Ubuntu/Debian:
sudo apt install cmake build-essential libcpr-dev nlohmann-json3-dev libyaml-cpp-dev libspdlog-dev python3 python3-venv

# Fedora:
sudo dnf install cmake gcc-c++ cpr-devel json-devel yaml-cpp-devel spdlog-devel python3

# macOS (with Homebrew):
brew install cmake cpr nlohmann-json yaml-cpp spdlog python3

# 3. Create .env file (NEVER COMMIT THIS)
cp .env.example .env
# Edit .env and add your GitHub token

# 4. Build C++ scanner
cd scanner
cmake -B build -S .
cmake --build build
cd ..

# 5. Setup Python bot
cd bot
python3 -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r requirements.txt
cd ..
```

### Usage

**See [USAGE.md](USAGE.md) for detailed instructions.**

Quick workflow:

**1. Configure (optional):**
Edit `config/keywords.yaml` and `config/patterns.yaml` to customize search.

**2. Run scanner:**
```bash
cd scanner
nix-shell ../shell.nix --run "./build/scanner --max-repos 10"
# Or without nix-shell if dependencies installed system-wide:
./build/scanner --max-repos 10
```

**3. Review findings:**
```bash
cat ../data/findings.csv
```

**4. Run bot (dry-run first):**
```bash
cd ../bot
source venv/bin/activate
python bot.py --input ../data/findings.csv --dry-run
python bot.py --input ../data/findings.csv
```

## Configuration

**Keywords:** `config/keywords.yaml` - Search terms for finding repositories
**Patterns:** `config/patterns.yaml` - Regex patterns for detecting secrets

Easy to edit - no recompilation needed!

## Security

⚠️ **IMPORTANT:**
- Never commit `.env` or `data/` directory
- Your GitHub token should have read-only public repo access
- Use a separate bot account for creating issues
- This tool only reads public data - no cloning, no local storage

## How It Works

1. Scanner queries GitHub Search API for repositories matching criteria
2. For each repo, searches for common secret-containing files (.env, config.json, etc.)
3. Downloads file contents via API (base64-encoded)
4. Runs regex patterns to detect secrets
5. Writes findings to CSV (metadata only, not actual secrets)
6. Bot reads CSV, creates GitHub issues, deletes processed entries

## Contributing

This is a learning project, but suggestions welcome!

## License

MIT

## Disclaimer

This tool is for educational and security research purposes. Use responsibly and respect rate limits.