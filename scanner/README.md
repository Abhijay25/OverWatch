# Scanner Code Guide

This document explains the C++ scanner architecture and codebase structure.

## Architecture Overview

The scanner is built with a modular architecture, separating concerns into distinct components:

```
┌─────────────┐
│     CLI     │ ← Command-line interface (main.cpp)
└──────┬──────┘
       │
       ├─────────────┐
       │             │
       ▼             ▼
┌─────────────┐ ┌─────────────┐
│ QueryBank   │ │  Scanner    │
└─────────────┘ └──────┬──────┘
                       │
                ┌──────┴────────┐
                │               │
                ▼               ▼
        ┌──────────────┐ ┌──────────────┐
        │GitHubClient  │ │SecretDetector│
        └──────────────┘ └──────────────┘
                │               │
                ▼               ▼
           GitHub API      patterns.yaml
```

## Core Components

### 1. GitHubClient (`github_client.h/cpp`)

**Purpose:** Handles all GitHub API interactions

**Key methods:**
- `searchRepositories()` - Search for repos using GitHub Search API
- `getFileContent()` - Fetch file contents (base64-decoded)
- `validateToken()` - Verify GitHub token is valid
- `getRateLimit()` - Check API rate limit status

**Implementation details:**
- Uses `libcpr` for HTTP requests
- Automatically adds authentication headers
- Decodes base64-encoded file contents from API
- Returns structured `Repository` objects

### 2. SecretDetector (`secret_detector.h/cpp`)

**Purpose:** Pattern-based secret detection

**Key methods:**
- `loadPatterns()` - Parse patterns from `config/patterns.yaml`
- `scanContent()` - Match patterns against file content
- `fileMatchesPattern()` - Check if pattern applies to filename

**How it works:**
1. Loads YAML patterns into compiled `std::regex` objects
2. Scans content line-by-line for matches
3. Returns `Match` objects with line number and matched text
4. Supports file-specific patterns (e.g., only scan `.env` files)

**Pattern structure:**
```yaml
patterns:
  - name: "GitHub Token"
    regex: "ghp_[a-zA-Z0-9]{36}"
    files: ["*"]  # Apply to all files
```

### 3. Scanner (`scanner.h/cpp`)

**Purpose:** Orchestrates the scanning process

**Key methods:**
- `run()` - Main scan loop
- `scanRepository()` - Scan a single repo
- `writeFinding()` - Output matches to JSONL

**Workflow:**
1. Search GitHub for repositories matching query
2. For each repo, check for suspicious files (`.env`, `config.json`, etc.)
3. Download file contents via API
4. Run secret detector on contents
5. Write findings to `data/findings.jsonl`

**Suspicious files checked:**
- `.env`, `.env.local`, `.env.production`
- `config.json`, `config.yaml`
- `credentials.json`, `secrets.json`
- `firebase.json`, `google-services.json`
- `.npmrc`, `.pypirc`

### 4. QueryBank (`query_bank.h/cpp`)

**Purpose:** Manage saved search queries

**Key methods:**
- `load()` - Load queries from `data/query_bank.yaml`
- `save()` - Persist changes back to YAML
- `add()`, `remove()`, `getAll()` - CRUD operations
- `filterByTag()`, `getRandom()` - Query filtering

**Query structure:**
```yaml
queries:
  - id: 1
    name: "Recent Low-Star Python"
    query: "language:Python stars:<10 created:>2026-02-10"
    tags: ["python", "recent"]
    max_repos: 5
```

### 5. CLI (`cli.h/cpp`)

**Purpose:** Command-line interface and argument parsing

**Supported commands:**
- `run <query>` - Run single search
- `list` - Show all saved queries
- `all` - Run all queries
- `random` - Run random query
- `filter --tag <tag>` - Run queries with tag
- `add --name ... --query ...` - Add query to bank
- `delete <id>` - Remove query

**Implementation:**
- Parses arguments into `Command` enum
- Stores options in `std::map<string, string>`
- Delegates execution to command handlers

### 6. Base64 (`base64.h/cpp`)

**Purpose:** Decode GitHub API file contents

GitHub's Contents API returns files as base64-encoded strings. This utility decodes them back to plaintext for scanning.

## Data Flow

```
1. User runs: overwatch run "language:Python stars:<5"
                    ↓
2. CLI parses command → creates GitHubClient, SecretDetector, Scanner
                    ↓
3. Scanner.run() → GitHubClient.searchRepositories()
                    ↓
4. For each repo → check suspicious files → getFileContent()
                    ↓
5. SecretDetector.scanContent() → check patterns
                    ↓
6. Write findings to data/findings.jsonl
```

## File Organization

```
scanner/
├── include/           # Header files (.h)
│   ├── base64.h       # Base64 decoder
│   ├── cli.h          # CLI parser
│   ├── github_client.h # GitHub API client
│   ├── query_bank.h   # Query management
│   ├── scanner.h      # Main scanner
│   └── secret_detector.h # Pattern matcher
├── src/               # Implementation (.cpp)
│   ├── base64.cpp
│   ├── cli.cpp
│   ├── github_client.cpp
│   ├── main.cpp       # Entry point
│   ├── query_bank.cpp
│   ├── scanner.cpp
│   └── secret_detector.cpp
└── CMakeLists.txt     # Build configuration
```

## Key Design Decisions

### Why C++?
- **Performance:** Fast HTTP requests and regex matching
- **Longevity:** Stable language, will compile for years
- **Learning:** Experience with systems programming

### Why no git cloning?
- Uses GitHub Contents API instead of `git clone`
- Faster: only downloads specific files
- Lower bandwidth and storage requirements
- Simpler cleanup (no repos on disk)

### Why JSONL output?
- **Streaming:** Append findings as they're discovered
- **Simple parsing:** One JSON object per line
- **Language-agnostic:** Python bot can easily read it

### Why separate pattern file?
- **No recompilation:** Change patterns without rebuilding
- **User-friendly:** YAML is easier than C++ regex strings
- **Shareable:** Users can exchange pattern files

## Dependencies

- **libcpr** - HTTP client library (wrapper around libcurl)
- **nlohmann-json** - JSON parsing and serialization
- **yaml-cpp** - YAML file parsing
- **spdlog** - Logging framework

## Building

```bash
# Configure
cmake -B build -S .

# Build
cmake --build build

# Install (optional)
cmake --install build --prefix ~/.local
```

## Extension Points

Want to extend the scanner? Here are the main extension points:

1. **Add new patterns:** Edit `config/patterns.yaml` (no code changes needed)
2. **Change suspicious files:** Modify `Scanner::suspicious_files_` in `scanner.h`
3. **Add new commands:** Add to `Command` enum in `cli.h` and implement handler in `cli.cpp`
4. **Custom output format:** Modify `Scanner::writeFinding()` in `scanner.cpp`
5. **Different API endpoints:** Extend `GitHubClient` methods

## Common Code Patterns

### Error Handling
```cpp
// Check API responses
if (!response.success()) {
    spdlog::error("API request failed: {}", response.error());
    return;
}
```

### Pattern Matching
```cpp
// Iterate through patterns and check content
for (const auto& pattern : patterns_) {
    if (!fileMatchesPattern(filename, pattern.files)) continue;

    std::smatch match;
    if (std::regex_search(line, match, pattern.regex)) {
        // Found a match!
    }
}
```

### JSONL Output
```cpp
nlohmann::json j = {
    {"owner", owner},
    {"repo", repo},
    {"file", file},
    {"secret_type", match.pattern_name}
};
outfile << j.dump() << "\n";
```

## Debugging Tips

1. **Check API rate limits:** `overwatch` logs remaining API calls
2. **Test patterns:** Use a small `--max-repos 1` to test changes
3. **Enable debug logging:** Modify `spdlog::set_level()` in `main.cpp`
4. **Validate queries:** Test GitHub search queries on github.com/search first

## Performance

- **API rate limit:** 5000 requests/hour (authenticated)
- **Typical speed:** ~10-30 repos/minute (depends on file sizes)
- **Memory:** Low footprint, no repo cloning
- **CPU:** Regex matching is fast for simple patterns
