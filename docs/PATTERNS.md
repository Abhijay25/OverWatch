# Patterns & Query Bank Guide

This guide explains how to configure secret detection patterns and manage search queries in OverWatch.

## Table of Contents

- [Secret Detection Patterns](#secret-detection-patterns)
- [Query Bank](#query-bank)
- [GitHub Search Syntax](#github-search-syntax)

---

## Secret Detection Patterns

### Overview

Patterns are defined in `config/patterns.yaml` and control what secrets the scanner looks for. The file uses YAML syntax for easy editing.

**Key advantage:** Changes take effect immediately - no recompilation needed!

### Pattern Structure

```yaml
patterns:
  - name: "GitHub Token"           # Human-readable name
    regex: "ghp_[a-zA-Z0-9]{36}"   # Regular expression pattern
    files: ["*"]                    # Which files to scan
```

### Pattern Fields

| Field | Description | Example |
|-------|-------------|---------|
| `name` | Display name for the secret type | `"GitHub Token"` |
| `regex` | Regular expression to match | `"ghp_[a-zA-Z0-9]{36}"` |
| `files` | File patterns to scan (glob syntax) | `["*.env", "config.*"]` |

### File Patterns

You can specify which files to scan using glob patterns:

```yaml
# Scan all files
files: ["*"]

# Only specific files
files: [".env", "config.json"]

# Pattern matching
files: ["*.env", "*.config", "*.yaml"]

# Multiple specific files
files: ["google-services.json", "GoogleService-Info.plist"]
```

### Example Patterns

#### GitHub Personal Access Token
```yaml
- name: "GitHub Token"
  regex: "ghp_[a-zA-Z0-9]{36}"
  files: ["*"]
```

Matches: `ghp_1234567890abcdefghij1234567890ABCDEF`

#### AWS Access Key
```yaml
- name: "AWS Access Key"
  regex: "AKIA[0-9A-Z]{16}"
  files: ["*"]
```

Matches: `AKIAIOSFODNN7EXAMPLE`

#### Generic API Key (in config files)
```yaml
- name: "Generic API Key"
  regex: "(api[_-]?key|apikey)\\s*[:=]\\s*['\"][a-zA-Z0-9]{20,}['\"]"
  files: ["*.env", "*.json", "*.yaml", "*.yml", "*.config"]
```

Matches:
- `api_key = "abcd1234567890efghij"`
- `apikey: "xyz123abc456def789ghi"`
- `API-KEY="long_key_value_here_12345"`

#### Private SSH/TLS Keys
```yaml
- name: "Private Key"
  regex: "-----BEGIN (RSA |EC )?PRIVATE KEY-----"
  files: ["*"]
```

Matches:
- `-----BEGIN PRIVATE KEY-----`
- `-----BEGIN RSA PRIVATE KEY-----`
- `-----BEGIN EC PRIVATE KEY-----`

#### Slack Tokens
```yaml
- name: "Slack Token"
  regex: "xox[baprs]-[0-9a-zA-Z]{10,48}"
  files: ["*"]
```

Matches tokens like:
- `xoxb-[numbers]-[numbers]-[alphanumeric]` (Bot token)
- `xoxp-[numbers]-[numbers]-[alphanumeric]` (User token)

#### Discord Bot Tokens
```yaml
- name: "Discord Token"
  regex: "[MN][A-Za-z\\d]{23}\\.[\\w-]{6}\\.[\\w-]{27}"
  files: ["*"]
```

Matches tokens in format: `[MN][23chars].[6chars].[27chars]`

#### Firebase API Keys
```yaml
- name: "Firebase Config"
  regex: "\"api_key\":\\s*\"[A-Za-z0-9_-]{39}\""
  files: ["google-services.json", "GoogleService-Info.plist"]
```

Matches: `"api_key": "AIzaSyBCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"`

### Regex Tips

#### Escaping Special Characters
In YAML strings, backslashes need to be escaped:

```yaml
# Wrong - won't work
regex: "\s*:\s*"

# Correct - double backslash
regex: "\\s*:\\s*"
```

#### Common Regex Patterns

| Pattern | Meaning | Example |
|---------|---------|---------|
| `[a-z]` | Lowercase letter | `a`, `b`, `z` |
| `[A-Z]` | Uppercase letter | `A`, `B`, `Z` |
| `[0-9]` | Digit | `0`, `5`, `9` |
| `[a-zA-Z0-9]` | Alphanumeric | `a`, `Z`, `5` |
| `{36}` | Exactly 36 characters | `ghp_` + 36 chars |
| `{20,}` | 20 or more characters | Min length 20 |
| `\\s` | Whitespace | Space, tab, newline |
| `[:=]` | Colon or equals | `:` or `=` |
| `*` | Zero or more | `a*` matches `""`, `"a"`, `"aaa"` |
| `+` | One or more | `a+` matches `"a"`, `"aaa"` |
| `?` | Optional | `a?` matches `""` or `"a"` |
| `(a|b)` | Either a or b | `(api\|key)` matches `api` or `key` |

#### Testing Regex

Test your regex patterns before adding them:

1. Use online tools like [regex101.com](https://regex101.com)
2. Select "ECMAScript (JavaScript)" flavor (similar to C++ std::regex)
3. Test with real examples

### Adding New Patterns

1. Edit `config/patterns.yaml`
2. Add your pattern following the structure
3. Save the file
4. Run the scanner - changes take effect immediately!

Example - adding a Stripe API key pattern:

```yaml
patterns:
  # ... existing patterns ...

  - name: "Stripe API Key"
    regex: "sk_(test|live)_[0-9a-zA-Z]{24,}"
    files: ["*.env", "config.*"]
```

### Testing Patterns

Create a small test to verify your pattern works:

```bash
# Run scanner with low max_repos to test
overwatch run "language:Python stars:<5" --max-repos 1

# Check if pattern matched anything
cat data/findings.jsonl
```

### Common Pitfalls

#### Too Broad Patterns
```yaml
# Bad - matches too much
regex: "[a-z]+"

# Good - specific to actual secret format
regex: "ghp_[a-zA-Z0-9]{36}"
```

#### Forgetting to Escape
```yaml
# Bad - won't work
regex: "\d+"

# Good - properly escaped
regex: "\\d+"
```

#### Wrong File Patterns
```yaml
# Bad - tries to scan all files (slow)
files: ["*"]

# Good - only scans relevant files
files: ["*.env", "config.json"]
```

---

## Query Bank

### Overview

The query bank is a collection of saved GitHub search queries stored in `data/query_bank.yaml`. It allows you to organize and reuse searches.

### Query Structure

```yaml
queries:
  - id: 1                                      # Unique identifier
    name: "Recent Low-Star Python"             # Descriptive name
    query: "language:Python stars:<10"         # GitHub search query
    tags: ["python", "low-stars", "recent"]    # Organization tags
    max_repos: 5                               # Repository limit
```

### Managing Queries

#### List All Queries
```bash
overwatch list
```

Output:
```
Saved queries:
  [1] Recent Low-Star Python
      Query: language:Python stars:<10 created:>2026-02-10
      Tags: python, low-stars, recent
      Max repos: 5

  [2] Discord Bot Projects
      Query: discord bot language:Python stars:<15
      Tags: python, bot, discord
      Max repos: 5
```

#### Add a Query
```bash
overwatch add \
  --name "Recent Node.js Projects" \
  --query "language:JavaScript stars:<10 created:>2026-02-15" \
  --tag javascript \
  --tag recent \
  --max-repos 10
```

This appends to `data/query_bank.yaml`:
```yaml
- id: 7
  name: "Recent Node.js Projects"
  query: "language:JavaScript stars:<10 created:>2026-02-15"
  tags: ["javascript", "recent"]
  max_repos: 10
```

#### Delete a Query
```bash
# Delete query with ID 3
overwatch delete 3
```

#### Edit Queries Manually
You can also edit `data/query_bank.yaml` directly:

```bash
nano data/query_bank.yaml
```

### Running Queries

#### Single Query
```bash
# Run query by ID (from list command)
overwatch run "language:Python stars:<5"
```

#### All Queries
```bash
# Run all saved queries sequentially
overwatch all
```

#### Random Query
```bash
# Run a random query from the bank
overwatch random
```

#### Filter by Tag
```bash
# Run all queries tagged "python"
overwatch filter --tag python

# Run all queries tagged "bot"
overwatch filter --tag bot
```

### Tagging Strategy

Use tags to organize queries by:

**Language:**
```yaml
tags: ["python"]
tags: ["javascript", "typescript"]
tags: ["go", "rust"]
```

**Project Type:**
```yaml
tags: ["bot", "discord"]
tags: ["api", "backend"]
tags: ["scraper", "automation"]
```

**Characteristics:**
```yaml
tags: ["low-stars"]    # stars:<10
tags: ["recent"]       # created:>date
tags: ["popular"]      # stars:>100
```

**Combination:**
```yaml
tags: ["python", "bot", "recent", "low-stars"]
```

### Example Queries

#### Recent Python Projects
```yaml
- id: 1
  name: "Recent Low-Star Python"
  query: "language:Python stars:<10 created:>2026-02-10"
  tags: ["python", "low-stars", "recent"]
  max_repos: 5
```

#### Discord Bots
```yaml
- id: 2
  name: "Discord Bot Projects"
  query: "discord bot language:Python stars:<15"
  tags: ["python", "bot", "discord"]
  max_repos: 5
```

#### API Projects
```yaml
- id: 3
  name: "API Projects"
  query: "api language:Python stars:<10"
  tags: ["python", "api", "low-stars"]
  max_repos: 5
```

#### Multi-Language Scrapers
```yaml
- id: 4
  name: "Scraper Projects"
  query: "scraper stars:<10"
  tags: ["scraper", "multi-language", "low-stars"]
  max_repos: 5
```

---

## GitHub Search Syntax

### Overview

OverWatch uses [GitHub Code Search syntax](https://docs.github.com/en/search-github/searching-on-github/searching-code).

### Common Filters

#### Language
```
language:Python
language:JavaScript
language:Go
```

#### Stars
```
stars:<10        # Less than 10 stars
stars:>100       # More than 100 stars
stars:10..50     # Between 10 and 50 stars
```

#### Created Date
```
created:>2026-02-10       # After date
created:<2026-01-01       # Before date
created:2026-02-01..2026-02-15  # Date range
```

#### Repository Size
```
size:<1000       # Less than 1000 KB
size:>10000      # More than 10000 KB
```

#### Forks
```
forks:<5         # Few forks
forks:>100       # Many forks
```

#### Keywords
```
discord bot      # Contains "discord" and "bot"
"exact phrase"   # Exact phrase match
api OR rest      # Either keyword
```

### Combining Filters

You can combine multiple filters:

```bash
# Recent, low-star Python bots
overwatch run "language:Python bot stars:<10 created:>2026-02-10"

# Large JavaScript projects
overwatch run "language:JavaScript stars:>100 size:>5000"

# Small, new Go projects
overwatch run "language:Go stars:<5 created:>2026-02-15"
```

### Advanced Examples

#### Target Beginner Projects
```
language:Python stars:<5 created:>2026-02-10 forks:<2
```
Small, new projects likely by beginners (higher chance of exposed secrets).

#### Discord/Telegram Bots
```
(discord OR telegram) bot language:Python stars:<20
```
Bot projects that often contain API tokens.

#### Recent API Projects
```
api language:JavaScript stars:<15 created:>2026-02-01
```
API projects that might have hardcoded keys.

#### Firebase Projects
```
firebase language:JavaScript stars:<10
```
Projects using Firebase (often expose API keys).

### Tips for Effective Queries

1. **Target low-star repos:** Higher chance of security mistakes
   ```
   stars:<10
   ```

2. **Focus on recent repos:** Fresh commits, active development
   ```
   created:>2026-02-10
   ```

3. **Specific project types:** Bots, APIs, scrapers often contain secrets
   ```
   bot language:Python
   api language:JavaScript
   scraper
   ```

4. **Limit scope:** Use `--max-repos` to avoid rate limits
   ```bash
   overwatch run "..." --max-repos 10
   ```

5. **Use tags:** Organize queries for easy filtering
   ```yaml
   tags: ["python", "bot", "high-priority"]
   ```

### Rate Limits

- **Authenticated:** 5000 requests/hour
- **Unauthenticated:** 60 requests/hour

Always use a GitHub token for realistic usage.

### Testing Queries

Before adding to query bank, test on GitHub:

1. Go to https://github.com/search
2. Select "Repositories"
3. Try your query
4. Verify results are relevant
5. Add to OverWatch

Example: https://github.com/search?q=language:Python+stars:<10&type=repositories

---

## Configuration Files Location

```
OverWatch/
├── config/
│   └── patterns.yaml      # Secret detection patterns
└── data/
    └── query_bank.yaml    # Saved search queries
```

Both files are version-controlled, so changes persist across updates.

---

## Best Practices

### Patterns
- ✅ Start with specific patterns (fewer false positives)
- ✅ Test patterns on sample data before using
- ✅ Use file restrictions when possible (faster scanning)
- ✅ Document complex regex patterns
- ❌ Avoid overly broad patterns
- ❌ Don't scan all files unless necessary

### Queries
- ✅ Use descriptive names
- ✅ Tag consistently
- ✅ Set reasonable `max_repos` limits
- ✅ Target specific project types
- ✅ Focus on recent repositories
- ❌ Don't spam the same query repeatedly
- ❌ Don't set `max_repos` too high (rate limits)

### General
- ✅ Review findings before running the bot
- ✅ Test with `--dry-run` first
- ✅ Respect rate limits
- ✅ Handle false positives gracefully
- ❌ Don't store actual secret values
- ❌ Don't commit findings files to git
