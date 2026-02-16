#!/usr/bin/env python3

import os
import json
import argparse
from pathlib import Path
from github import Github, GithubException
from dotenv import load_dotenv

# Parse command-line arguments
parser = argparse.ArgumentParser(description='Process OverWatch findings and create GitHub issues')
parser.add_argument('--dry-run', action='store_true',
                    help='Show what would happen without creating issues')
parser.add_argument('--input', type=str,
                    help='Path to findings JSONL file (default: test_findings.jsonl)')
args = parser.parse_args()

# Load .env file from parent directory
env_path = Path(__file__).parent.parent / '.env'
load_dotenv(env_path)

# Load GitHub token from environment
github_token = os.getenv('GITHUB_TOKEN')

if not github_token:
    print("Error: GITHUB_TOKEN not found in environment")
    print("Run: export GITHUB_TOKEN=your_token_here")
    exit(1)

# Connect to GitHub
github = Github(github_token)

# Test authentication
try:
    user = github.get_user()
    print(f"✓ Connected to GitHub as: {user.login}")
    if args.dry_run:
        print(f"✓ DRY-RUN MODE: No issues will be created")
except GithubException as e:
    print(f"✗ Authentication failed: {e.data.get('message', 'Unknown error')}")
    print(f"  Status code: {e.status}")
    print("  Check your GITHUB_TOKEN is valid")
    exit(1)
except Exception as e:
    print(f"✗ Unexpected error during authentication: {e}")
    exit(1)

# Read findings from JSONL file
if args.input:
    findings_file = Path(args.input)
else:
    findings_file = Path(__file__).parent / "test_findings.jsonl"

if not findings_file.exists():
    print(f"✗ Findings file not found: {findings_file}")
    exit(1)

print(f"✓ Reading findings from: {findings_file}")

# Read and parse JSONL
findings = []
try:
    with open(findings_file, 'r') as f:
        for line_num, line in enumerate(f, 1):
            # Skip empty lines
            line = line.strip()
            if not line:
                continue

            # Parse JSON
            try:
                finding = json.loads(line)
                findings.append(finding)
            except json.JSONDecodeError as e:
                print(f"✗ Invalid JSON on line {line_num}: {e}")
                continue

except Exception as e:
    print(f"✗ Error reading file: {e}")
    exit(1)

print(f"✓ Loaded {len(findings)} findings\n")

# Group findings by repository
from collections import defaultdict
grouped_findings = defaultdict(list)
for finding in findings:
    repo_key = f"{finding['owner']}/{finding['repo']}"
    grouped_findings[repo_key].append(finding)

print(f"✓ Grouped into {len(grouped_findings)} repositories\n")

# Get the OverWatch repo (where we'll post issues)
# TODO: Update this to your actual OverWatch repo
OVERWATCH_REPO = "Abhijay25/OverWatch"  # Change this to your repo name

try:
    overwatch_repo = github.get_repo(OVERWATCH_REPO)
    print(f"✓ Will post findings to: {OVERWATCH_REPO}\n")
except GithubException as e:
    print(f"✗ Cannot access OverWatch repo: {OVERWATCH_REPO}")
    print(f"  Error: {e.data.get('message', 'Unknown error')}")
    print(f"  Update OVERWATCH_REPO in bot.py to your repo name")
    exit(1)

# Process each repository
success_count = 0
failed_count = 0
skipped_count = 0
failed_findings = []  # Keep track of findings that failed

for i, (repo_key, repo_findings) in enumerate(grouped_findings.items(), 1):
    owner, repo_name = repo_key.split('/')
    num_secrets = len(repo_findings)

    print(f"[{i}/{len(grouped_findings)}] Processing {repo_key} ({num_secrets} finding(s))...")

    # Check if target repository still exists (optional validation)
    try:
        target_repo = github.get_repo(repo_key)
        repo_exists = True
        repo_url = target_repo.html_url
    except GithubException:
        repo_exists = False
        repo_url = f"https://github.com/{repo_key}"
    # Skip if repo doesn't exist anymore
    if not repo_exists:
        print(f"  ⊘ Repository no longer exists - skipping")
        skipped_count += num_secrets
        continue

    # Create secure issue title
    if num_secrets == 1:
        title = f"🔒 Security: Potential {repo_findings[0]['secret_type']} in {owner}/{repo_name}"
    else:
        title = f"🔒 Security: {num_secrets} Potential Secrets in {owner}/{repo_name}"

    # Build secure details section (NO ACTUAL SECRETS, NO DIRECT LINKS!)
    details_section = ""
    for idx, finding in enumerate(repo_findings, 1):
        if num_secrets > 1:
            details_section += f"\n**Finding {idx}:**\n"
        details_section += f"""- **Type:** {finding['secret_type']}
- **File:** `{finding['file']}`
- **Line:** {finding['line']}
- **Detected:** {finding['timestamp']}

"""

    # Create issue body with @mention
    body = f"""## 🔒 Security Alert

Hey @{owner}! 👋

An automated security scan detected potential exposed credential(s) in your repository **[{owner}/{repo_name}]({repo_url})**.

### 📋 Findings

{details_section}

---

### ⚠️ Security Notice

**For your safety, the actual secret values are NOT posted here.** Please check the file locations above in your repository.

### 🛠️ Recommended Actions

1. **Immediately rotate/invalidate** the exposed credential(s)
2. Remove secrets from your code and use environment variables instead
3. Review git history - secrets may exist in previous commits
4. Use [GitHub Secrets](https://docs.github.com/en/actions/security-guides/encrypted-secrets) for sensitive data
5. Add sensitive files to `.gitignore`

### 🔧 How to Remove Secrets from Git History

```bash
# Remove sensitive file from history
git filter-branch --force --index-filter \
  "git rm --cached --ignore-unmatch {repo_findings[0]['file']}" \
  --prune-empty --tag-name-filter cat -- --all

# Force push (WARNING: rewrites history)
git push origin --force --all
```

### 📚 Resources

- [Remove Sensitive Data from Git](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/removing-sensitive-data-from-a-repository)
- [Using Environment Variables](https://12factor.net/config)
- [Git-Secret Tool](https://git-secret.io/)

---

*🤖 This is an automated notification from [OverWatch](https://github.com/Abhijay25/OverWatch) - a security scanning project. If this is a false positive or you need help, feel free to comment here!*
"""

    # Create the issue in OverWatch repo
    try:
        if args.dry_run:
            print(f"  ✓ [DRY-RUN] Would create issue in {OVERWATCH_REPO}")
            print(f"     Title: {title}")
            print(f"     Target: {repo_key}")
            print(f"     @mention: @{owner}")
            for finding in repo_findings:
                print(f"     - {finding['file']} (line {finding['line']}) - {finding['secret_type']}")
            success_count += num_secrets
        else:
            issue = overwatch_repo.create_issue(
                title=title,
                body=body,
                labels=['security-alert', 'external-repo']
            )
            print(f"  ✓ Created issue #{issue.number}: {issue.html_url}")
            print(f"     @mentioned: @{owner}")
            print(f"     ({num_secrets} secret(s) reported)")
            success_count += num_secrets
        # Don't add to failed_findings - success!
    except GithubException as e:
        if e.status == 403:
            print(f"  ✗ Permission denied")
            failed_count += num_secrets
            failed_findings.extend(repo_findings)  # Save all findings for retry
        elif e.status == 410:
            print(f"  ⊘ Repository archived - skipping")
            skipped_count += num_secrets
            # Don't save - repo archived
        else:
            print(f"  ✗ Failed: {e.data.get('message', 'Unknown error')}")
            failed_count += num_secrets
            failed_findings.extend(repo_findings)  # Save all findings for retry
    except Exception as e:
        print(f"  ✗ Unexpected error: {e}")
        failed_count += num_secrets
        failed_findings.extend(repo_findings)  # Save all findings for retry

# Update the JSONL file (remove successful and skipped entries)
if success_count > 0 or skipped_count > 0:
    if args.dry_run:
        print(f"\n📝 [DRY-RUN] Would update findings file...")
        removed_count = success_count + skipped_count
        print(f"  ✓ Would remove {removed_count} processed entries")
        print(f"  ✓ Would keep {failed_count} failed entries for retry")
    else:
        print(f"\n📝 Updating findings file...")

        # Create backup first
        from datetime import datetime
        backup_file = findings_file.with_suffix(f'.jsonl.backup.{datetime.now().strftime("%Y%m%d_%H%M%S")}')
        backup_file.write_text(findings_file.read_text())
        print(f"  ✓ Created backup: {backup_file.name}")

        # Write only failed findings back to file
        with open(findings_file, 'w') as f:
            for finding in failed_findings:
                f.write(json.dumps(finding) + '\n')

        removed_count = success_count + skipped_count
        print(f"  ✓ Removed {removed_count} processed entries")
        print(f"  ✓ Kept {failed_count} failed entries for retry")

# Print summary
print("\n" + "=" * 60)
print("SUMMARY:")
print(f"  Total findings: {len(findings)}")
print(f"  ✓ Success: {success_count}")
print(f"  ✗ Failed: {failed_count}")
print(f"  ⊘ Skipped: {skipped_count}")
print("=" * 60)
