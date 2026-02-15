# OverWatch 🔒

> **Automated security tool that scans public GitHub repositories for exposed secrets and notifies repository owners**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

---

## Why OverWatch?

Developers often accidentally commit sensitive credentials (API keys, tokens, passwords) to public repositories. **OverWatch** helps identify these security issues and notifies repository owners so they can take action.

### Goals

- 🎓 **Learning:** Hands-on experience with C++, HTTP APIs, pattern matching, and security practices
- 🛡️ **Impact:** Help developers protect their credentials and improve security
- 🏗️ **Longevity:** Built with stable technologies to run reliably for years

---

## How It Works

```
┌─────────────┐      ┌──────────┐      ┌─────────────┐      ┌─────────────────┐
│   GitHub    │─────▶│   C++    │─────▶│   findings  │─────▶│  Python Bot     │
│   Search    │      │  Scanner │      │     .jsonl  │      │  Creates Issues │
└─────────────┘      └──────────┘      └─────────────┘      └─────────────────┘
     ▲                    │                                          │
     │                    │                                          ▼
  Search API        Pattern Matching                         GitHub Issues API
```

### **C++ Scanner**
- Searches GitHub for recently created, low-star repositories
- Scans files for exposed secrets (API keys, tokens, passwords)
- Outputs findings to JSONL format (no local repo downloads)
- Fast, efficient, and configurable via YAML patterns

### **Python Bot**
- Reads findings from JSONL
- Creates GitHub issues to alert repository owners
- Removes successfully processed entries
- Handles errors gracefully (retry failed, skip archived repos)

---

## Quick Start

### Prerequisites

- **C++ Scanner:**
  - C++17 compiler (GCC 7+ or Clang 5+)
  - CMake 3.15+
  - Dependencies: libcpr, nlohmann-json, yaml-cpp, spdlog

- **Python Bot:**
  - Python 3.8+
  - pip (Python package manager)

- **GitHub Token:**
  - Personal access token with `public_repo` scope
  - Get one here: https://github.com/settings/tokens

### Installation

```bash
# 1. Clone the repository
git clone https://github.com/yourusername/OverWatch.git
cd OverWatch

# 2. Set up GitHub token
cp .env.example .env
# Edit .env and add your GitHub token: GITHUB_TOKEN=ghp_your_token_here

# 3. Build the C++ scanner
cmake -B build -S .
cmake --build build

# 4. Install scanner command (optional)
cmake --install build --prefix ~/.local
export PATH="$HOME/.local/bin:$PATH"

# 5. Set up Python bot
cd bot
python3 -m venv venv
source venv/bin/activate  # On Windows: venv\Scripts\activate
pip install -r requirements.txt
cd ..

# 6. Load environment variables
export $(cat .env | grep -v '^#' | xargs)
```

### Platform-Specific Dependencies

<details>
<summary><b>Ubuntu/Debian</b></summary>

```bash
sudo apt update
sudo apt install cmake build-essential libcpr-dev nlohmann-json3-dev \
                 libyaml-cpp-dev libspdlog-dev python3 python3-pip python3-venv
```
</details>

<details>
<summary><b>Fedora/RHEL</b></summary>

```bash
sudo dnf install cmake gcc-c++ cpr-devel json-devel \
                 yaml-cpp-devel spdlog-devel python3 python3-pip
```
</details>

<details>
<summary><b>macOS (Homebrew)</b></summary>

```bash
brew install cmake cpr nlohmann-json yaml-cpp spdlog python3
```
</details>

<details>
<summary><b>Arch Linux</b></summary>

```bash
sudo pacman -S cmake gcc cpr nlohmann-json yaml-cpp spdlog python python-pip
```
</details>

### Basic Usage

```bash
# Run scanner and bot together (dry-run first!)
./overwatch run "language:Python stars:<10 created:>2026-02-15" --max-repos 5 --dry-run

# Run for real
./overwatch run "language:Python bot stars:<20" --max-repos 10

# List saved queries
./overwatch list

# Run all saved queries
./overwatch all --dry-run

# Run scanner only (skip bot)
./overwatch run "language:JavaScript stars:<5" --no-bot
```

**The `overwatch` command automatically:**
- Loads `GITHUB_TOKEN` from `.env` file
- Runs scanner and bot together
- Can be stopped anytime with Ctrl+C

---

## Documentation

Detailed guides for extending and customizing OverWatch:

- **[Scanner Code Guide](scanner/README.md)** - C++ architecture, components, and how to extend the scanner
- **[Bot Usage Guide](bot/README.md)** - Python bot setup, options, error handling, and customization
- **[Patterns & Query Bank](docs/PATTERNS.md)** - Configure detection patterns and manage search queries

---

## Project Structure

```
OverWatch/
├── scanner/              # C++ scanner source code
│   ├── include/          # Header files (.h)
│   ├── src/              # Implementation files (.cpp)
│   └── README.md         # Scanner code guide
├── bot/                  # Python bot
│   ├── bot.py            # Main bot script
│   ├── requirements.txt  # Python dependencies
│   └── README.md         # Bot usage guide
├── config/               # Configuration files
│   ├── patterns.yaml     # Secret detection patterns
│   └── keywords.yaml     # Search keywords (deprecated)
├── data/                 # Runtime data (gitignored)
│   ├── query_bank.yaml   # Saved search queries
│   └── findings.jsonl    # Scan results
├── docs/                 # Additional documentation
│   └── PATTERNS.md       # Guide to patterns and query bank
├── .env.example          # Environment template
├── LICENSE               # MIT License
└── README.md             # This file
```

---

## Quick Reference

```bash
# Main commands (scanner + bot automatically)
overwatch run <query> [--max-repos N]   # Run query with bot
overwatch all                           # Run all saved queries
overwatch random                        # Run random query
overwatch filter --tag <tag>            # Run queries by tag

# Management (scanner only, no bot)
overwatch list                          # List all queries
overwatch add [options]                 # Add query to bank
overwatch delete <id>                   # Delete query

# Options
--dry-run                               # Bot test mode (no issues created)
--no-bot                                # Skip bot, run scanner only
--max-repos N                           # Limit repositories scanned

# Examples
overwatch run "language:Python stars:<10" --dry-run
overwatch all --dry-run
overwatch run "query" --no-bot          # Scanner only
```

---

## Security & Ethics

⚠️ **Important Guidelines:**

- ✅ **Only scan public repositories** - respect privacy
- ✅ **Verify findings** - review for false positives before notifying
- ✅ **Be helpful** - provide actionable guidance in issues
- ✅ **Respect rate limits** - 5000 requests/hour (authenticated)
- ✅ **Never commit `.env`** - keep tokens secure
- ❌ **Don't spam** - one notification per finding
- ❌ **Don't store actual secrets** - only metadata in findings

### Responsible Disclosure

- Always use `--dry-run` mode when testing the bot
- Review findings manually before creating issues
- Respect maintainers who close issues as false positives
- Consider using a separate bot account for issue creation

---

## Contributing

This is a learning project, but contributions are welcome!

- 🐛 **Found a bug?** Open an issue
- 💡 **Have an idea?** Open an issue
- 🔧 **Want to contribute?** Fork and submit a PR

---

## License

MIT License - see [LICENSE](LICENSE) for details

---

## Disclaimer

This tool is for **educational and security research purposes**. Use responsibly and respect GitHub's Terms of Service and API rate limits.

---

**Built with ❤️ as a learning project to understand C++, APIs, and security practices.**
