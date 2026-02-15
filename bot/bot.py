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

# Process each finding
success_count = 0
failed_count = 0
skipped_count = 0
failed_findings = []  # Keep track of findings that failed

for i, finding in enumerate(findings, 1):
    print(f"[{i}/{len(findings)}] Processing {finding['owner']}/{finding['repo']}...")

    # Get repository
    try:
        repo = github.get_repo(f"{finding['owner']}/{finding['repo']}")
    except GithubException as e:
        if e.status == 404:
            print(f"  ⊘ Repository not found - skipping")
            skipped_count += 1
            continue  # Don't save - repo deleted
        elif e.status == 403:
            print(f"  ⊘ Access forbidden - skipping")
            skipped_count += 1
            continue  # Don't save - can't access
        else:
            print(f"  ✗ Error: {e.data.get('message', 'Unknown error')}")
            failed_count += 1
            failed_findings.append(finding)  # Save for retry
            continue

    # Check if issues are enabled
    if not repo.has_issues:
        print(f"  ⊘ Issues disabled - skipping")
        skipped_count += 1
        continue  # Don't save - issues disabled

    # Create issue title and body from finding
    title = f"🔒 Security Alert: Potential {finding['secret_type']} Exposure"
    body = f"""## Security Alert

An automated security scan detected what appears to be an exposed credential in your repository.

**Details:**
- **File:** `{finding['file']}`
- **Line:** {finding['line']}
- **Type:** `{finding['secret_type']}`
- **Detection Date:** {finding['timestamp']}

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
"""

    # Create the issue
    try:
        if args.dry_run:
            print(f"  ✓ [DRY-RUN] Would create issue: {title}")
            print(f"     Repository: {finding['owner']}/{finding['repo']}")
            print(f"     File: {finding['file']} (line {finding['line']})")
            success_count += 1
        else:
            issue = repo.create_issue(
                title=title,
                body=body,
                labels=['security']
            )
            print(f"  ✓ Created issue #{issue.number}: {issue.html_url}")
            success_count += 1
        # Don't add to failed_findings - success!
    except GithubException as e:
        if e.status == 403:
            print(f"  ✗ Permission denied")
            failed_count += 1
            failed_findings.append(finding)  # Save for retry
        elif e.status == 410:
            print(f"  ⊘ Repository archived - skipping")
            skipped_count += 1
            # Don't save - repo archived
        else:
            print(f"  ✗ Failed: {e.data.get('message', 'Unknown error')}")
            failed_count += 1
            failed_findings.append(finding)  # Save for retry
    except Exception as e:
        print(f"  ✗ Unexpected error: {e}")
        failed_count += 1
        failed_findings.append(finding)  # Save for retry

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
