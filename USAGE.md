# OverWatch Usage Guide

## Quick Start

### 1. Setup Environment

#### On NixOS (or with Nix installed):
```bash
# Enter development shell
nix-shell

# Build C++ scanner
cd scanner
cmake -B build -S .
cmake --build build
cd ..

# Setup Python bot
cd bot
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
cd ..
```

#### On other Linux/macOS:
```bash
# Install dependencies
# - C++17 compiler (g++ or clang)
# - CMake 3.15+
# - Libraries: libcpr, nlohmann-json, yaml-cpp, spdlog
# - Python 3.8+

# Build C++ scanner
cd scanner
cmake -B build -S .
cmake --build build
cd ..

# Setup Python bot
cd bot
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
cd ..
```

### 2. Configure GitHub Token

```bash
# Create .env file (NEVER commit this!)
cp .env.example .env

# Edit .env and add your GitHub token
# Get a token from: https://github.com/settings/tokens
# Required scopes: public_repo (read-only for scanner, write for bot)
nano .env
```

Your `.env` should look like:
```
GITHUB_TOKEN=ghp_your_scanner_token_here
GITHUB_BOT_TOKEN=ghp_your_bot_token_here
```

### 3. Configure Search Criteria (Optional)

Edit `config/keywords.yaml` to customize what repositories to search for:
```yaml
keywords:
  - bot
  - discord
  - telegram
  - scraper
  # Add your own keywords...
```

Edit `config/patterns.yaml` to add or modify secret detection patterns:
```yaml
patterns:
  - name: "My Custom Token"
    regex: "mytoken_[a-zA-Z0-9]{32}"
    files: ["*"]
```

### 4. Run the Scanner

```bash
cd scanner

# Dry run (test without writing findings)
nix-shell ../shell.nix --run "./build/scanner --max-repos 5 --dry-run"

# Real scan (write findings to CSV)
nix-shell ../shell.nix --run "./build/scanner --max-repos 10"

# Check findings
cat ../data/findings.csv
```

Scanner options:
- `--max-repos N` - Maximum repositories to scan (default: 10)
- `--max-stars N` - Only scan repos with < N stars (default: 10)
- `--dry-run` - Don't write findings to CSV
- `--help` - Show help

### 5. Review Findings

```bash
# View findings in CSV
cat data/findings.csv

# Or use a CSV viewer
column -t -s, data/findings.csv | less -S
```

**IMPORTANT:** Review findings for false positives before notifying!

### 6. Run the Bot

```bash
cd bot
source venv/bin/activate  # If not already activated

# Dry run (preview what would happen)
python bot.py --input ../data/findings.csv --dry-run

# Real run (create GitHub issues)
python bot.py --input ../data/findings.csv
```

The bot will:
1. Read each finding from CSV
2. Create a GitHub issue in the repository
3. Delete the processed entry from CSV
4. Keep failed entries in CSV for retry

### 7. Monitor Results

- Issues will be created with labels: `security`, `help wanted`
- Check `data/findings.csv` - successfully processed entries are deleted
- Failed entries remain in CSV for manual review or retry

## Advanced Usage

### Scheduling Regular Scans

Use cron to schedule regular scans:

```bash
# Edit crontab
crontab -e

# Add line to run scanner daily at 2 AM
0 2 * * * cd /path/to/OverWatch/scanner && ./build/scanner --max-repos 50 >> /var/log/overwatch.log 2>&1
```

### Custom Search Queries

The scanner constructs queries like:
```
language:Python stars:<10 created:>2026-02-07
```

To modify this, edit `scanner/src/main.cpp` around line 86.

### Adding New Secret Patterns

Edit `config/patterns.yaml`:

```yaml
  - name: "Stripe API Key"
    regex: "sk_(test|live)_[a-zA-Z0-9]{24,}"
    files: ["*"]

  - name: "SendGrid API Key"
    regex: "SG\\.[a-zA-Z0-9]{22}\\.[a-zA-Z0-9]{43}"
    files: ["*"]
```

Rebuild is NOT required - patterns are loaded at runtime!

### Filtering by File Type

In `patterns.yaml`, specify which files to check:

```yaml
  - name: "Database Password"
    regex: "DB_PASSWORD\\s*=\\s*['\"][^'\"]+['\"]"
    files: ["*.env", ".env.*", "config.yml"]
```

### Rate Limit Management

- **Unauthenticated:** 60 requests/hour
- **Authenticated:** 5000 requests/hour

The scanner automatically handles rate limits and waits when exhausted.

To check your current rate limit:
```bash
curl -H "Authorization: Bearer $GITHUB_TOKEN" https://api.github.com/rate_limit
```

## Troubleshooting

### Scanner Build Fails

**Problem:** CMake can't find libraries

**Solution (NixOS):**
```bash
nix-shell
cd scanner
cmake -B build -S .
```

**Solution (Other Linux):**
```bash
# Install dependencies via package manager
# Ubuntu/Debian:
sudo apt install libcpr-dev nlohmann-json3-dev libyaml-cpp-dev libspdlog-dev cmake build-essential

# Fedora:
sudo dnf install cpr-devel json-devel yaml-cpp-devel spdlog-devel cmake gcc-c++
```

### Python Bot Fails

**Problem:** `ModuleNotFoundError: No module named 'github'`

**Solution:**
```bash
cd bot
source venv/bin/activate
pip install -r requirements.txt
```

### No Secrets Found

**Possible reasons:**
1. Repositories don't have the target files (.env, config.json, etc.)
2. Files exist but don't contain secrets
3. Secrets don't match patterns in `config/patterns.yaml`

**Debug:**
- Run scanner with `--dry-run` and check console output
- Try more repositories: `--max-repos 50`
- Check different languages or keywords

### Rate Limit Exhausted

**Problem:** Scanner says "Rate limit exhausted"

**Solution:**
- Wait for rate limit reset (shown in log)
- Use authenticated token (5000/hour vs 60/hour)
- Reduce `--max-repos` count

### Bot Can't Create Issues

**Possible reasons:**
1. Repository is private (bot only works on public repos)
2. Repository has been deleted
3. Token lacks permissions
4. Issue already exists

**Solution:**
- Check `GITHUB_BOT_TOKEN` has `public_repo` write access
- Review failed entries in CSV
- Use `--dry-run` to preview without creating issues

## Best Practices

### Before Running in Production

1. **Test on your own repositories first**
   - Create a test repo with fake secrets
   - Run scanner on it
   - Verify bot creates issue correctly
   - Check issue template looks good

2. **Start small**
   - Use `--max-repos 10` initially
   - Review all findings manually
   - Tune patterns to reduce false positives

3. **Use separate accounts**
   - Scanner token: read-only access
   - Bot token: separate bot account with write access
   - Keeps your main account clean

4. **Monitor rate limits**
   - Check remaining requests before large scans
   - Schedule scans during off-hours
   - Spread scans over time

### Ethical Considerations

- **Only scan public repositories** - respect privacy
- **Verify findings** - avoid false positive spam
- **Be helpful** - provide actionable guidance in issues
- **Respect maintainers** - don't reopen closed issues
- **Rate limit awareness** - don't abuse GitHub API

### Security

- ✅ **DO:** Use environment variables for tokens
- ✅ **DO:** Keep `.env` in `.gitignore`
- ✅ **DO:** Use separate bot account
- ✅ **DO:** Rotate tokens periodically
- ❌ **DON'T:** Commit tokens to git
- ❌ **DON'T:** Share your `.env` file
- ❌ **DON'T:** Log actual secret values
- ❌ **DON'T:** Store secrets in CSV (only metadata)

## Example Workflow

```bash
# 1. Enter development environment
nix-shell

# 2. Run scanner for discord bots
cd scanner
./build/scanner --max-repos 20

# 3. Review findings
cat ../data/findings.csv | wc -l
echo "Found secrets in that many files ^"

# 4. Preview what bot would do
cd ../bot
source venv/bin/activate
python bot.py --input ../data/findings.csv --dry-run

# 5. If everything looks good, create issues
python bot.py --input ../data/findings.csv

# 6. Check results
echo "Remaining in CSV (failed):"
cat ../data/findings.csv | wc -l
```

## Contributing

Found a bug? Have a suggestion?

1. Check existing issues
2. Open a new issue with details
3. PRs welcome!

## License

MIT - See LICENSE file
