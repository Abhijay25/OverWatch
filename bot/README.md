# Bot Usage Guide

The Python bot reads findings from the C++ scanner and creates GitHub issues to notify repository owners about exposed secrets.

## Quick Start

```bash
# 1. Navigate to bot directory
cd bot

# 2. Create virtual environment
python3 -m venv venv

# 3. Activate virtual environment
source venv/bin/activate  # On Windows: venv\Scripts\activate

# 4. Install dependencies
pip install -r requirements.txt

# 5. Make sure .env is configured with GITHUB_TOKEN
# (The bot looks for .env in the parent directory)

# 6. Test with dry-run mode first!
python bot.py --dry-run

# 7. Run for real
python bot.py
```

## How It Works

```
1. Read findings from data/findings.jsonl
           ↓
2. For each finding:
   - Get repository via GitHub API
   - Check if issues are enabled
   - Create security issue with details
           ↓
3. Update findings file:
   - Remove successful entries
   - Remove skipped entries (404, archived, etc.)
   - Keep failed entries for retry
```

## Command-Line Options

```bash
# Process findings with default settings
python bot.py

# Dry-run mode (show what would happen without creating issues)
python bot.py --dry-run

# Specify custom input file
python bot.py --input path/to/findings.jsonl

# Combine options
python bot.py --dry-run --input ../data/findings.jsonl
```

## Finding Processing Logic

The bot handles each finding with different outcomes:

### ✓ Success
- Issue created successfully
- Finding removed from JSONL file
- Logged with issue number and URL

### ✗ Failed
- API error (rate limit, temporary failure, etc.)
- Finding kept in JSONL for retry
- Error details logged

### ⊘ Skipped
- Repository not found (404) - deleted or private
- Repository archived (410) - can't create issues
- Issues disabled on repository
- Access forbidden (403) - can't access repo
- Finding removed from JSONL (no point retrying)

## Issue Template

Issues created by the bot follow this format:

```markdown
## Security Alert

An automated security scan detected what appears to be an exposed credential in your repository.

**Details:**
- **File:** `.env`
- **Line:** 12
- **Type:** `GitHub Token`
- **Detection Date:** 2026-02-15T18:00:00Z

⚠️ **For security reasons, exact details are not posted publicly.**

### Recommended Actions:
1. **Rotate/invalidate the exposed credential** immediately
2. Remove the secret from your code and use environment variables
3. Review git history - the secret may exist in older commits
4. Consider using GitHub Secrets for sensitive data

### Resources:
- [GitHub Encrypted Secrets](https://docs.github.com/en/actions/security-guides/encrypted-secrets)
- [Git: Remove Sensitive Data](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/removing-sensitive-data-from-a-repository)

---
*This is an automated notification from my project, OverWatch. If this is a false positive, please close this issue.*
```

## File Management

### Input File
Default: `bot/test_findings.jsonl`

Each line is a JSON object:
```json
{"owner":"user","repo":"project","file":".env","line":5,"secret_type":"GitHub Token","matched_text":"ghp_abc...","timestamp":"2026-02-15T18:00:00Z"}
```

### Backup Files
Before modifying the input file, the bot creates a timestamped backup:
```
test_findings.jsonl.backup.20260215_180045
```

### Output Behavior
After processing:
- **Successful entries:** Removed from file
- **Skipped entries:** Removed from file
- **Failed entries:** Kept in file for retry

This means you can safely re-run the bot - it will only retry failures.

## Error Handling

### Authentication Errors
```
✗ Authentication failed: Bad credentials
  Status code: 401
  Check your GITHUB_TOKEN is valid
```

**Fix:** Verify your `GITHUB_TOKEN` in `.env` is correct and has `public_repo` scope.

### Rate Limit Errors
```
✗ API rate limit exceeded
```

**Fix:** Wait for rate limit to reset. Check limits with:
```bash
curl -H "Authorization: token YOUR_TOKEN" https://api.github.com/rate_limit
```

Authenticated users get 5000 requests/hour.

### Repository Not Found
```
⊘ Repository not found - skipping
```

**Reason:** Repository was deleted, made private, or never existed.
**Action:** Automatically skipped and removed from file.

### Issues Disabled
```
⊘ Issues disabled - skipping
```

**Reason:** Repository has issues disabled in settings.
**Action:** Automatically skipped and removed from file.

## Best Practices

### 1. Always Test First
```bash
python bot.py --dry-run
```

Review the findings before creating real issues. Check for false positives.

### 2. Use a Bot Account
Consider creating a separate GitHub account for issue creation:
- Keeps your personal account separate
- Easier to identify automated notifications
- Can be disabled without affecting your main account

### 3. Review Findings Manually
Before running the bot, check `findings.jsonl`:
```bash
cat ../data/findings.jsonl | jq .
```

Look for:
- False positives (e.g., example code, documentation)
- Duplicate findings
- Already-reported issues

### 4. Start Small
Process a few findings at a time:
```bash
# Take first 5 findings
head -n 5 ../data/findings.jsonl > test_findings.jsonl
python bot.py --input test_findings.jsonl
```

### 5. Monitor Rate Limits
The bot doesn't currently check rate limits before making requests. If you have many findings, you might hit the 5000/hour limit.

Consider adding delays between requests:
```python
import time
time.sleep(1)  # Wait 1 second between issues
```

### 6. Handle Feedback Gracefully
Repository owners may:
- Close issues as false positives
- Ask questions about the detection
- Report bugs in your scanner

Be responsive and respectful.

## Dependencies

From `requirements.txt`:
```
PyGithub>=2.1.1  # GitHub API client
python-dotenv>=1.0.0  # Load .env files
```

Install with:
```bash
pip install -r requirements.txt
```

## Workflow Example

Complete workflow from scanner to bot:

```bash
# 1. Run scanner to generate findings
cd /path/to/OverWatch
overwatch run "language:Python stars:<5 created:>2026-02-10"

# 2. Review findings
cat data/findings.jsonl | jq .

# 3. Copy to bot directory (or use --input)
cp data/findings.jsonl bot/test_findings.jsonl

# 4. Test bot
cd bot
source venv/bin/activate
python bot.py --dry-run

# 5. Run for real
python bot.py

# 6. Check results
cat test_findings.jsonl  # Should only contain failed entries
```

## Customization

### Modify Issue Template
Edit the `title` and `body` variables in `bot.py`:

```python
# Line 104-129
title = f"🔒 Security Alert: Potential {finding['secret_type']} Exposure"
body = f"""## Security Alert
...
"""
```

### Add Issue Labels
The bot currently adds a `security` label:

```python
# Line 136
labels=['security']
```

You can add more:
```python
labels=['security', 'bot', 'automated']
```

### Change Backup Format
Modify the timestamp format in backup filenames:

```python
# Line 165
backup_file = findings_file.with_suffix(
    f'.jsonl.backup.{datetime.now().strftime("%Y%m%d_%H%M%S")}'
)
```

## Troubleshooting

### Bot says token not found
```
Error: GITHUB_TOKEN not found in environment
Run: export GITHUB_TOKEN=your_token_here
```

**Fix:** The bot looks for `.env` in the **parent directory**, not `bot/.env`:
```bash
# Make sure this exists
cat ../.env
```

### No findings file
```
✗ Findings file not found: test_findings.jsonl
```

**Fix:** Either create the file or use `--input` to specify a different path:
```bash
python bot.py --input ../data/findings.jsonl
```

### Permission denied errors
```
✗ Permission denied
```

**Fix:** Check your token has the correct scopes:
1. Go to https://github.com/settings/tokens
2. Ensure token has `public_repo` scope
3. For private repos, needs `repo` scope

## Security Considerations

- **Don't include matched text in issues:** The bot intentionally omits the actual secret value
- **Use separate bot account:** Protects your personal account
- **Review before posting:** Always use `--dry-run` first
- **Respect privacy:** Only scan public repositories
- **Handle rate limits:** Don't spam the API

## Future Improvements

Potential enhancements (not yet implemented):

- [ ] Rate limit checking before making requests
- [ ] Configurable delays between requests
- [ ] Custom issue templates from file
- [ ] Check for existing issues before creating duplicates
- [ ] Support for issue comments instead of new issues
- [ ] Integration with notification services (Slack, email)
- [ ] Dry-run statistics (show what would happen)
