#!/usr/bin/env python3
"""
OverWatch Runner - Simple orchestrator
Runs scanner and bot together with continuous monitoring
"""

import os
import sys
import subprocess
import signal
import time
from pathlib import Path


class OverWatchRunner:
    def __init__(self, dry_run=False):
        self.dry_run = dry_run
        self.running = True
        self.project_root = Path(__file__).parent

        # Paths
        self.scanner = self.project_root / 'build' / 'scanner' / 'overwatch'
        self.bot_dir = self.project_root / 'bot'
        self.findings = self.project_root / 'data' / 'findings.jsonl'
        self.venv_python = self.bot_dir / 'venv' / 'bin' / 'python'

        # Handle Ctrl+C
        signal.signal(signal.SIGINT, self.handle_exit)
        signal.signal(signal.SIGTERM, self.handle_exit)

    def handle_exit(self, sig, frame):
        print("\n\n🛑 Shutting down...")
        self.running = False
        sys.exit(0)

    def run(self, query, max_repos=None):
        """Run scanner and bot together"""
        print("=" * 70)
        print(f"🚀 OverWatch Starting")
        print(f"   Query: {query}")
        if max_repos:
            print(f"   Max repos: {max_repos}")
        if self.dry_run:
            print("   Mode: DRY-RUN")
        print("=" * 70)
        print()

        # Clear findings file
        if self.findings.exists():
            self.findings.unlink()

        # Build scanner command
        scanner_cmd = [str(self.scanner), 'run', query]
        if max_repos:
            scanner_cmd.extend(['--max-repos', str(max_repos)])

        # Start scanner in background
        print("🔍 Starting scanner...")
        scanner_proc = subprocess.Popen(
            scanner_cmd,
            cwd=str(self.project_root),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True
        )
        print(f"   ✓ Scanner running (PID {scanner_proc.pid})\n")

        # Give scanner a moment to start creating the file
        time.sleep(2)

        # Bot loop - keep processing findings as they appear
        print("🤖 Bot monitoring findings...\n")
        last_line_count = 0

        while self.running:
            # Check if scanner is still running
            scanner_running = scanner_proc.poll() is None

            # Check if there are new findings
            if self.findings.exists():
                with open(self.findings, 'r') as f:
                    lines = [l for l in f if l.strip()]
                    current_count = len(lines)

                # If new findings appeared, run the bot
                if current_count > last_line_count:
                    print(f"📝 Found {current_count - last_line_count} new finding(s), processing...")

                    # Run bot on current findings
                    bot_cmd = [str(self.venv_python), 'bot.py']
                    if self.dry_run:
                        bot_cmd.append('--dry-run')
                    bot_cmd.extend(['--input', str(self.findings)])

                    subprocess.run(bot_cmd, cwd=str(self.bot_dir))

                    # Update count
                    # Re-count in case bot removed some entries
                    if self.findings.exists():
                        with open(self.findings, 'r') as f:
                            last_line_count = len([l for l in f if l.strip()])
                    else:
                        last_line_count = 0

                    print()

            # If scanner finished and no more findings, we're done
            if not scanner_running:
                # One final check for any remaining findings
                if self.findings.exists():
                    with open(self.findings, 'r') as f:
                        remaining = len([l for l in f if l.strip()])
                    if remaining > 0:
                        print(f"📝 Processing {remaining} final finding(s)...")
                        bot_cmd = [str(self.venv_python), 'bot.py']
                        if self.dry_run:
                            bot_cmd.append('--dry-run')
                        bot_cmd.extend(['--input', str(self.findings)])
                        subprocess.run(bot_cmd, cwd=str(self.bot_dir))

                print("\n✅ Scanner completed")
                break

            # Wait a bit before checking again
            time.sleep(2)

        # Get scanner output
        if scanner_proc.poll() is not None:
            scanner_proc.wait()

        print("\n" + "=" * 70)
        print("✅ OverWatch Completed")
        print("=" * 70)


def main():
    import argparse

    parser = argparse.ArgumentParser(description='OverWatch - Run scanner and bot together')
    parser.add_argument('query', help='GitHub search query')
    parser.add_argument('--max-repos', type=int, help='Max repositories to scan')
    parser.add_argument('--dry-run', action='store_true', help='Dry-run mode')

    args = parser.parse_args()

    # Check prerequisites
    project_root = Path(__file__).parent
    scanner = project_root / 'build' / 'scanner' / 'overwatch'
    venv = project_root / 'bot' / 'venv'

    if not scanner.exists():
        print("❌ Scanner not built. Run: cmake -B build -S . && cmake --build build")
        sys.exit(1)

    if not venv.exists():
        print("❌ Bot venv not set up. Run: cd bot && python3 -m venv venv && source venv/bin/activate && pip install -r requirements.txt")
        sys.exit(1)

    if not os.getenv('GITHUB_TOKEN'):
        print("❌ GITHUB_TOKEN not set. Run: export GITHUB_TOKEN=your_token")
        sys.exit(1)

    # Run
    runner = OverWatchRunner(dry_run=args.dry_run)
    runner.run(args.query, args.max_repos)


if __name__ == '__main__':
    main()
