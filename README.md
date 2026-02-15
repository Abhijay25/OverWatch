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
C++ Scanner → JSONL → Python Bot → GitHub Issues
```

**C++ Scanner:**
- Searches GitHub for recently created, low-star repositories
- Scans files for exposed secrets (API keys, tokens, passwords)
- Outputs findings to JSONL (no local repo downloads)

**Python Bot:**
- Reads findings from JSONL
- Creates GitHub issues to alert repository owners
- Deletes processed entries from JSONL

---

## Installation

### Prerequisites
- C++17 compiler (GCC/Clang)
- CMake 3.15+
- Dependencies: libcpr, nlohmann-json, yaml-cpp, spdlog
- Python 3.8+ (for bot)
- GitHub personal access token

### Quick Install

```bash
# 1. Clone repository
git clone https://github.com/yourusername/OverWatch.git
cd OverWatch

# 2. Set up your GitHub token
cp .env.example .env
# Edit .env and add your GitHub token

# 3. Build C++ scanner
cmake -B build -S .
cmake --build build

# 4. Install (makes 'overwatch' command available)
cmake --install build --prefix ~/.local

# 5. Add to PATH (one-time setup)
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc

# 6. Load environment variables
export $(cat .env | grep -v '^#' | xargs)
```

### Platform-Specific Dependencies

**NixOS / Nix (Recommended):**
```bash
nix-shell  # Automatically installs all dependencies
```

**Ubuntu/Debian:**
```bash
sudo apt install cmake build-essential libcpr-dev nlohmann-json3-dev libyaml-cpp-dev libspdlog-dev
```

**Fedora:**
```bash
sudo dnf install cmake gcc-c++ cpr-devel json-devel yaml-cpp-devel spdlog-devel
```

**macOS (Homebrew):**
```bash
brew install cmake cpr nlohmann-json yaml-cpp spdlog
```

---

## Usage

### Getting Started

```bash
# View all commands
overwatch help

# List saved queries
overwatch list

# Run a single query
overwatch run "language:Python stars:<5 created:>2026-02-10"

# Run with custom repo limit
overwatch run "language:Python stars:<5" --max-repos 10
```

### Query Bank System

OverWatch includes a query bank for managing search queries with tags:

```bash
# List all saved queries
overwatch list

# Run all queries from the bank
overwatch all

# Run a random query
overwatch random

# Filter queries by tag
overwatch filter --tag python
overwatch filter --tag bot

# Add a new query
overwatch add --name "Low Star JS" \
              --query "language:JavaScript stars:<5" \
              --tag javascript \
              --max-repos 5

# Delete a query
overwatch delete 3
```

### Query Bank File

Edit `data/query_bank.yaml` to customize queries:

```yaml
queries:
  - id: 1
    name: "Recent Low-Star Python"
    query: "language:Python stars:<10 created:>2026-02-10"
    tags: ["python", "low-stars", "recent"]
    max_repos: 5
```

### Workflow

**1. Configure patterns (optional):**
Edit `config/patterns.yaml` to add/modify secret detection patterns.

**2. Run scans:**
```bash
# Single query
overwatch run "language:Python stars:<10"

# All saved queries
overwatch all

# Specific tag
overwatch filter --tag bot
```

**3. Review findings:**
```bash
cat data/findings.jsonl
```

**4. Run bot (Python - coming soon):**
```bash
cd bot
python bot.py --input ../data/findings.jsonl
```

---

## Configuration

### Secret Patterns

Edit `config/patterns.yaml` to customize secret detection:

```yaml
patterns:
  - name: "GitHub Token"
    regex: "ghp_[a-zA-Z0-9]{36}"
    files: ["*"]

  - name: "AWS Access Key"
    regex: "AKIA[0-9A-Z]{16}"
    files: ["*"]
```

**No recompilation needed!** Changes take effect immediately.

### Query Bank

Edit `data/query_bank.yaml` to manage search queries. Supports:
- Multiple queries
- Tag-based organization
- Custom repository limits

---

## Commands Reference

```bash
overwatch help                          # Show help
overwatch list                          # List all queries
overwatch run <query> [--max-repos N]  # Run single query
overwatch all                           # Run all queries
overwatch random                        # Run random query
overwatch filter --tag <tag>            # Run queries with tag
overwatch add [options]                 # Add query to bank
overwatch delete <id>                   # Delete query
```

---

## Security

⚠️ **IMPORTANT:**
- Never commit `.env` or `data/` directory
- Your GitHub token should have `public_repo` scope (read-only)
- Use a separate bot account for creating issues
- This tool only reads public data - no cloning, no local storage
- Respect GitHub's rate limits (5000/hour authenticated, 60/hour unauthenticated)

---

## How It Works

1. Scanner queries GitHub Search API for repositories matching criteria
2. For each repo, checks for common secret-containing files (.env, config.json, etc.)
3. Downloads file contents via API (base64-encoded)
4. Runs regex patterns to detect secrets
5. Writes findings to JSONL (metadata only, not actual secrets)
6. Bot reads JSONL, creates GitHub issues, deletes processed entries

---

## Example Findings Output

`data/findings.jsonl`:
```json
{"owner":"user","repo":"project","file":".env","line":5,"secret_type":"GitHub Token","matched_text":"ghp_abc123...","timestamp":"2026-02-15T18:00:00Z"}
{"owner":"user2","repo":"app","file":"config.js","line":12,"secret_type":"AWS Access Key","matched_text":"AKIA1234...","timestamp":"2026-02-15T18:01:00Z"}
```

---

## Development

### Project Structure

```
OverWatch/
├── scanner/           # C++ scanner
│   ├── include/       # Header files
│   ├── src/           # Source files
│   └── CMakeLists.txt
├── bot/               # Python bot (coming soon)
├── config/            # Configuration files
│   ├── patterns.yaml  # Secret detection patterns
│   └── keywords.yaml  # Search keywords
├── data/              # Output directory
│   ├── query_bank.yaml   # Saved queries
│   └── findings.jsonl    # Scan results
└── .env               # GitHub token (gitignored)
```

### Building from Source

```bash
# Debug build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Release build (optimized)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## Contributing

This is a learning project, but suggestions welcome!

---

## License

MIT

---

## Disclaimer

This tool is for educational and security research purposes. Use responsibly and respect rate limits.
