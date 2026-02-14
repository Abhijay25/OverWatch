#!/usr/bin/env python3
"""
OverWatch Bot - Creates GitHub issues to notify repository owners about exposed secrets
"""

import os
import sys
import csv
import argparse
from pathlib import Path
from datetime import datetime
from github import Github, GithubException
from dotenv import load_dotenv

def load_template():
    """Load issue template"""
    template_path = Path(__file__).parent / "issue_template.md"
    with open(template_path, 'r') as f:
        return f.read()

def load_findings(csv_file):
    """Load findings from CSV file"""
    findings = []
    if not os.path.exists(csv_file):
        print(f"❌ File not found: {csv_file}")
        return findings

    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            findings.append(row)

    print(f"📋 Loaded {len(findings)} findings from {csv_file}")
    return findings

def create_issue_title(finding):
    """Generate issue title"""
    return f"🔒 Security: {finding['secret_type']} detected in {finding['file_path']}"

def create_issue_body(finding, template):
    """Generate issue body from template"""
    return template.format(
        file_path=finding['file_path'],
        line_number=finding['line_number'],
        secret_type=finding['secret_type'],
        timestamp=finding['timestamp']
    )

def issue_already_exists(repo, title):
    """Check if an issue with similar title already exists"""
    try:
        issues = repo.get_issues(state='all')
        for issue in issues:
            if title in issue.title or issue.title in title:
                return True, issue.number
    except GithubException as e:
        print(f"  ⚠️  Could not check existing issues: {e}")
    return False, None

def create_github_issue(g, finding, template, dry_run=False):
    """Create a GitHub issue for the finding"""
    repo_full_name = f"{finding['repo_owner']}/{finding['repo_name']}"

    try:
        # Get repository
        repo = g.get_repo(repo_full_name)

        # Generate title and body
        title = create_issue_title(finding)
        body = create_issue_body(finding, template)

        # Check if issue already exists
        exists, issue_num = issue_already_exists(repo, title)
        if exists:
            print(f"  ℹ️  Issue already exists: #{issue_num}")
            return False, f"Duplicate of #{issue_num}"

        if dry_run:
            print(f"  [DRY RUN] Would create issue:")
            print(f"    Title: {title}")
            print(f"    Labels: security, help wanted")
            return True, "dry-run"

        # Create issue
        issue = repo.create_issue(
            title=title,
            body=body,
            labels=["security", "help wanted"]
        )

        print(f"  ✅ Created issue: {issue.html_url}")
        return True, issue.html_url

    except GithubException as e:
        if e.status == 404:
            print(f"  ⚠️  Repository not found or not accessible")
        elif e.status == 403:
            print(f"  ⚠️  Permission denied (may be private repo)")
        elif e.status == 410:
            print(f"  ⚠️  Repository has been deleted")
        else:
            print(f"  ❌ Error: {e}")
        return False, str(e)

def delete_csv_row(csv_file, row_to_delete):
    """Remove a row from CSV file"""
    temp_file = csv_file + ".tmp"
    rows_kept = []

    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        fieldnames = reader.fieldnames
        for row in reader:
            # Keep row if it doesn't match the one to delete
            if not all(row[k] == row_to_delete[k] for k in row.keys()):
                rows_kept.append(row)

    # Write back to file
    with open(temp_file, 'w', newline='') as f:
        if rows_kept:
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(rows_kept)

    # Replace original file
    os.replace(temp_file, csv_file)

def main():
    parser = argparse.ArgumentParser(
        description='OverWatch Bot - Notify repository owners about exposed secrets'
    )
    parser.add_argument('--input', '-i', required=True, help='Path to findings CSV file')
    parser.add_argument('--dry-run', action='store_true', help='Preview without creating issues')
    args = parser.parse_args()

    print("╔═══════════════════════════════════════╗")
    print("║   OverWatch Bot v0.1.0 🤖           ║")
    print("║   GitHub Issue Notification Bot      ║")
    print("╚═══════════════════════════════════════╝\n")

    # Load environment variables
    load_dotenv()
    token = os.getenv('GITHUB_BOT_TOKEN') or os.getenv('GITHUB_TOKEN')

    if not token:
        print("❌ Error: GITHUB_BOT_TOKEN not set!")
        print("Set GITHUB_BOT_TOKEN in .env file or environment")
        sys.exit(1)

    print("✓ GitHub token loaded")

    # Load template
    template = load_template()
    print("✓ Issue template loaded\n")

    # Load findings
    findings = load_findings(args.input)
    if not findings:
        print("No findings to process")
        return

    if args.dry_run:
        print("⚠️  DRY RUN MODE - No issues will be created\n")

    # Initialize GitHub client
    g = Github(token)

    try:
        # Verify authentication
        user = g.get_user()
        print(f"✓ Authenticated as: {user.login}\n")
    except GithubException as e:
        print(f"❌ Authentication failed: {e}")
        sys.exit(1)

    # Process each finding
    processed = []
    failed = []
    duplicates = []

    for i, finding in enumerate(findings, 1):
        repo_name = f"{finding['repo_owner']}/{finding['repo_name']}"
        print(f"[{i}/{len(findings)}] Processing: {repo_name}")
        print(f"  File: {finding['file_path']} (line {finding['line_number']})")
        print(f"  Type: {finding['secret_type']}")

        success, result = create_github_issue(g, finding, template, args.dry_run)

        if success:
            processed.append((finding, result))
            # Delete from CSV (only if not dry-run)
            if not args.dry_run:
                delete_csv_row(args.input, finding)
                print(f"  🗑️  Removed from CSV")
        else:
            if "Duplicate" in result:
                duplicates.append((finding, result))
                # Also delete duplicates from CSV
                if not args.dry_run:
                    delete_csv_row(args.input, finding)
                    print(f"  🗑️  Removed from CSV (duplicate)")
            else:
                failed.append((finding, result))

        print()

    # Summary
    print("╔═══════════════════════════════════════╗")
    print("║          Bot Summary                  ║")
    print("╚═══════════════════════════════════════╝")
    print(f"Total findings: {len(findings)}")
    print(f"✅ Issues created: {len(processed)}")
    print(f"ℹ️  Duplicates skipped: {len(duplicates)}")
    print(f"❌ Failed: {len(failed)}")

    if not args.dry_run:
        # Check remaining CSV entries
        remaining = load_findings(args.input)
        print(f"\n📄 Remaining in CSV: {len(remaining)}")
        if remaining:
            print("   (These failed to process and can be retried)")

    if args.dry_run:
        print("\n⚠️  This was a dry run. Use without --dry-run to actually create issues.")

    print("\n✅ Bot completed successfully")

if __name__ == "__main__":
    main()
