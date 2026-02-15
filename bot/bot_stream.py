#!/usr/bin/env python3
"""
OverWatch Bot - Processes findings and creates GitHub issues
Supports both batch mode and streaming mode for real-time processing
"""

import os
import json
import time
import signal
import argparse
from pathlib import Path
from typing import Optional, Dict, Any
from github import Github, GithubException
from dotenv import load_dotenv


class FindingsProcessor:
    """Processes findings and creates GitHub issues"""

    def __init__(self, github_token: str, dry_run: bool = False):
        self.github = Github(github_token)
        self.dry_run = dry_run
        self.user = None
        self.processed_lines = set()  # Track processed line hashes to avoid duplicates

        # Statistics
        self.success_count = 0
        self.failed_count = 0
        self.skipped_count = 0

    def authenticate(self) -> bool:
        """Test GitHub authentication"""
        try:
            self.user = self.github.get_user()
            print(f"✓ Connected to GitHub as: {self.user.login}")
            if self.dry_run:
                print("✓ DRY-RUN MODE: No issues will be created")
            return True
        except GithubException as e:
            print(f"✗ Authentication failed: {e.data.get('message', 'Unknown error')}")
            print(f"  Status code: {e.status}")
            return False
        except Exception as e:
            print(f"✗ Unexpected error during authentication: {e}")
            return False

    def process_finding(self, finding: Dict[str, Any]) -> bool:
        """
        Process a single finding and create a GitHub issue
        Returns True if successful, False if failed (for retry)
        """
        # Create a unique hash for this finding to avoid duplicates
        finding_hash = f"{finding['owner']}:{finding['repo']}:{finding['file']}:{finding['line']}"

        if finding_hash in self.processed_lines:
            return True  # Already processed

        print(f"Processing {finding['owner']}/{finding['repo']}...")

        # Get repository
        try:
            repo = self.github.get_repo(f"{finding['owner']}/{finding['repo']}")
        except GithubException as e:
            if e.status == 404:
                print(f"  ⊘ Repository not found - skipping")
                self.skipped_count += 1
                self.processed_lines.add(finding_hash)
                return True  # Don't retry
            elif e.status == 403:
                print(f"  ⊘ Access forbidden - skipping")
                self.skipped_count += 1
                self.processed_lines.add(finding_hash)
                return True  # Don't retry
            else:
                print(f"  ✗ Error: {e.data.get('message', 'Unknown error')}")
                self.failed_count += 1
                return False  # Retry later

        # Check if issues are enabled
        if not repo.has_issues:
            print(f"  ⊘ Issues disabled - skipping")
            self.skipped_count += 1
            self.processed_lines.add(finding_hash)
            return True  # Don't retry

        # Create issue
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
*This is an automated notification from OverWatch. If this is a false positive, please close this issue.*
"""

        try:
            if self.dry_run:
                print(f"  ✓ [DRY-RUN] Would create issue: {title}")
                print(f"     Repository: {finding['owner']}/{finding['repo']}")
                print(f"     File: {finding['file']} (line {finding['line']})")
                self.success_count += 1
                self.processed_lines.add(finding_hash)
                return True
            else:
                issue = repo.create_issue(
                    title=title,
                    body=body,
                    labels=['security']
                )
                print(f"  ✓ Created issue #{issue.number}: {issue.html_url}")
                self.success_count += 1
                self.processed_lines.add(finding_hash)
                return True
        except GithubException as e:
            if e.status == 403:
                print(f"  ✗ Permission denied")
                self.failed_count += 1
                return False  # Retry
            elif e.status == 410:
                print(f"  ⊘ Repository archived - skipping")
                self.skipped_count += 1
                self.processed_lines.add(finding_hash)
                return True  # Don't retry
            else:
                print(f"  ✗ Failed: {e.data.get('message', 'Unknown error')}")
                self.failed_count += 1
                return False  # Retry
        except Exception as e:
            print(f"  ✗ Unexpected error: {e}")
            self.failed_count += 1
            return False  # Retry

    def print_summary(self):
        """Print processing summary"""
        total = self.success_count + self.failed_count + self.skipped_count
        print("\n" + "=" * 60)
        print("SUMMARY:")
        print(f"  Total processed: {total}")
        print(f"  ✓ Success: {self.success_count}")
        print(f"  ✗ Failed: {self.failed_count}")
        print(f"  ⊘ Skipped: {self.skipped_count}")
        print("=" * 60)


class StreamingProcessor:
    """Processes findings in streaming mode (real-time)"""

    def __init__(self, findings_file: Path, processor: FindingsProcessor):
        self.findings_file = findings_file
        self.processor = processor
        self.running = True
        self.position = 0

        # Handle Ctrl+C gracefully
        signal.signal(signal.SIGINT, self.signal_handler)
        signal.signal(signal.SIGTERM, self.signal_handler)

    def signal_handler(self, signum, frame):
        """Handle shutdown signals"""
        print("\n\n🛑 Received shutdown signal, finishing current operation...")
        self.running = False

    def check_scanner_done(self) -> bool:
        """Check if scanner is done by looking for a completion marker"""
        marker_file = self.findings_file.parent / ".scanner_done"
        return marker_file.exists()

    def tail_and_process(self):
        """Tail the findings file and process new lines as they appear"""
        print(f"📡 Streaming mode: Watching {self.findings_file}")
        print("   Waiting for scanner to write findings...\n")

        # Create file if it doesn't exist
        if not self.findings_file.exists():
            self.findings_file.touch()

        failed_findings = []

        while self.running:
            # Read new lines
            try:
                with open(self.findings_file, 'r') as f:
                    f.seek(self.position)

                    for line in f:
                        line = line.strip()
                        if not line:
                            continue

                        try:
                            finding = json.loads(line)
                            success = self.processor.process_finding(finding)

                            if not success:
                                failed_findings.append(finding)
                        except json.JSONDecodeError as e:
                            print(f"✗ Invalid JSON: {e}")

                    self.position = f.tell()
            except Exception as e:
                print(f"✗ Error reading file: {e}")

            # Check if scanner is done
            if self.check_scanner_done():
                print("\n✓ Scanner completed")
                break

            # Wait a bit before checking for new lines
            time.sleep(0.5)

        # Write failed findings back to file for retry
        if failed_findings:
            print(f"\n📝 Saving {len(failed_findings)} failed findings for retry...")
            with open(self.findings_file, 'w') as f:
                for finding in failed_findings:
                    f.write(json.dumps(finding) + '\n')
        else:
            # Clear the file if all were successful
            self.findings_file.write_text("")


def batch_mode(findings_file: Path, processor: FindingsProcessor):
    """Process all findings in batch mode (original behavior)"""
    print(f"✓ Reading findings from: {findings_file}")

    if not findings_file.exists():
        print(f"✗ Findings file not found: {findings_file}")
        return

    # Read all findings
    findings = []
    try:
        with open(findings_file, 'r') as f:
            for line_num, line in enumerate(f, 1):
                line = line.strip()
                if not line:
                    continue

                try:
                    finding = json.loads(line)
                    findings.append(finding)
                except json.JSONDecodeError as e:
                    print(f"✗ Invalid JSON on line {line_num}: {e}")
    except Exception as e:
        print(f"✗ Error reading file: {e}")
        return

    print(f"✓ Loaded {len(findings)} findings\n")

    # Process each finding
    failed_findings = []
    for i, finding in enumerate(findings, 1):
        print(f"[{i}/{len(findings)}] ", end="")
        success = processor.process_finding(finding)
        if not success:
            failed_findings.append(finding)

    # Update the JSONL file (remove successful entries)
    if processor.success_count > 0 or processor.skipped_count > 0:
        if processor.dry_run:
            print(f"\n📝 [DRY-RUN] Would update findings file...")
            removed_count = processor.success_count + processor.skipped_count
            print(f"  ✓ Would remove {removed_count} processed entries")
            print(f"  ✓ Would keep {processor.failed_count} failed entries for retry")
        else:
            print(f"\n📝 Updating findings file...")

            # Create backup
            from datetime import datetime
            backup_file = findings_file.with_suffix(
                f'.jsonl.backup.{datetime.now().strftime("%Y%m%d_%H%M%S")}'
            )
            backup_file.write_text(findings_file.read_text())
            print(f"  ✓ Created backup: {backup_file.name}")

            # Write only failed findings back
            with open(findings_file, 'w') as f:
                for finding in failed_findings:
                    f.write(json.dumps(finding) + '\n')

            removed_count = processor.success_count + processor.skipped_count
            print(f"  ✓ Removed {removed_count} processed entries")
            print(f"  ✓ Kept {processor.failed_count} failed entries for retry")


def main():
    # Parse arguments
    parser = argparse.ArgumentParser(
        description='OverWatch Bot - Process findings and create GitHub issues'
    )
    parser.add_argument('--dry-run', action='store_true',
                        help='Show what would happen without creating issues')
    parser.add_argument('--input', type=str,
                        help='Path to findings JSONL file')
    parser.add_argument('--stream', action='store_true',
                        help='Stream mode: process findings as scanner writes them')
    args = parser.parse_args()

    # Load environment
    env_path = Path(__file__).parent.parent / '.env'
    load_dotenv(env_path)

    github_token = os.getenv('GITHUB_TOKEN')
    if not github_token:
        print("Error: GITHUB_TOKEN not found in environment")
        print("Run: export GITHUB_TOKEN=your_token_here")
        exit(1)

    # Determine findings file
    if args.input:
        findings_file = Path(args.input)
    elif args.stream:
        # In streaming mode, use the main findings file
        findings_file = Path(__file__).parent.parent / 'data' / 'findings.jsonl'
    else:
        findings_file = Path(__file__).parent / 'test_findings.jsonl'

    # Create processor
    processor = FindingsProcessor(github_token, dry_run=args.dry_run)

    # Authenticate
    if not processor.authenticate():
        exit(1)

    # Run in appropriate mode
    if args.stream:
        streamer = StreamingProcessor(findings_file, processor)
        streamer.tail_and_process()
    else:
        batch_mode(findings_file, processor)

    # Print summary
    processor.print_summary()


if __name__ == "__main__":
    main()
