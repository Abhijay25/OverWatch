#!/usr/bin/env python3
"""
OverWatch - Automated GitHub Secret Scanner
Main orchestrator that runs scanner and bot together
"""

import os
import sys
import subprocess
import signal
import time
import argparse
from pathlib import Path
from typing import Optional


class OverWatch:
    """Main orchestrator for OverWatch"""

    def __init__(self, dry_run: bool = False, verbose: bool = False):
        self.dry_run = dry_run
        self.verbose = verbose
        self.scanner_process: Optional[subprocess.Popen] = None
        self.bot_process: Optional[subprocess.Popen] = None
        self.running = True

        # Paths
        self.project_root = Path(__file__).parent
        self.scanner_binary = self.project_root / 'build' / 'scanner' / 'overwatch'
        self.bot_script = self.project_root / 'bot' / 'bot_stream.py'
        self.bot_venv = self.project_root / 'bot' / 'venv'
        self.findings_file = self.project_root / 'data' / 'findings.jsonl'
        self.marker_file = self.project_root / 'data' / '.scanner_done'

        # Handle Ctrl+C gracefully
        signal.signal(signal.SIGINT, self.signal_handler)
        signal.signal(signal.SIGTERM, self.signal_handler)

    def signal_handler(self, signum, frame):
        """Handle shutdown signals"""
        print("\n\n🛑 Received shutdown signal, cleaning up...")
        self.running = False
        self.cleanup()
        sys.exit(0)

    def cleanup(self):
        """Clean up processes and temporary files"""
        if self.scanner_process and self.scanner_process.poll() is None:
            print("  Stopping scanner...")
            self.scanner_process.terminate()
            self.scanner_process.wait(timeout=5)

        if self.bot_process and self.bot_process.poll() is None:
            print("  Stopping bot...")
            self.bot_process.terminate()
            self.bot_process.wait(timeout=5)

        # Remove marker file
        if self.marker_file.exists():
            self.marker_file.unlink()

    def check_prerequisites(self) -> bool:
        """Check that everything is set up correctly"""
        errors = []

        # Check scanner binary
        if not self.scanner_binary.exists():
            errors.append(f"Scanner binary not found: {self.scanner_binary}")
            errors.append("  Run: cmake -B build -S . && cmake --build build")

        # Check bot script
        if not self.bot_script.exists():
            errors.append(f"Bot script not found: {self.bot_script}")

        # Check bot venv
        if not self.bot_venv.exists():
            errors.append(f"Bot virtual environment not found: {self.bot_venv}")
            errors.append("  Run: cd bot && python3 -m venv venv && source venv/bin/activate && pip install -r requirements.txt")

        # Check GitHub token
        if not os.getenv('GITHUB_TOKEN'):
            errors.append("GITHUB_TOKEN not found in environment")
            errors.append("  Run: export GITHUB_TOKEN=your_token_here")

        # Check data directory
        if not (self.project_root / 'data').exists():
            (self.project_root / 'data').mkdir()

        if errors:
            print("❌ Prerequisites not met:\n")
            for error in errors:
                print(f"  {error}")
            return False

        return True

    def run_scanner(self, query: str, max_repos: Optional[int] = None) -> bool:
        """Start the scanner in the background"""
        print(f"🔍 Starting scanner...")

        # Clear findings file and marker
        if self.findings_file.exists():
            self.findings_file.unlink()
        if self.marker_file.exists():
            self.marker_file.unlink()

        # Build scanner command
        cmd = [str(self.scanner_binary), 'run', query]
        if max_repos:
            cmd.extend(['--max-repos', str(max_repos)])

        # Start scanner
        try:
            self.scanner_process = subprocess.Popen(
                cmd,
                cwd=str(self.project_root),
                stdout=subprocess.PIPE if not self.verbose else None,
                stderr=subprocess.STDOUT if not self.verbose else None,
                text=True
            )
            print(f"   ✓ Scanner started (PID {self.scanner_process.pid})")
            return True
        except Exception as e:
            print(f"   ✗ Failed to start scanner: {e}")
            return False

    def run_bot(self) -> bool:
        """Start the bot in streaming mode"""
        print(f"🤖 Starting bot in streaming mode...")

        # Get Python from venv
        python_bin = self.bot_venv / 'bin' / 'python'

        # Build bot command
        cmd = [str(python_bin), str(self.bot_script), '--stream']
        if self.dry_run:
            cmd.append('--dry-run')

        # Start bot
        try:
            self.bot_process = subprocess.Popen(
                cmd,
                cwd=str(self.project_root),
                text=True
            )
            print(f"   ✓ Bot started (PID {self.bot_process.pid})\n")
            return True
        except Exception as e:
            print(f"   ✗ Failed to start bot: {e}")
            return False

    def wait_for_scanner(self):
        """Wait for scanner to complete"""
        print("⏳ Waiting for scanner to complete...\n")

        if not self.scanner_process:
            return

        # Wait for scanner to finish
        while self.running:
            if self.scanner_process.poll() is not None:
                break
            time.sleep(1)

        # Get exit code
        exit_code = self.scanner_process.poll()
        if exit_code == 0:
            print("\n✅ Scanner completed successfully")
        else:
            print(f"\n⚠️  Scanner exited with code {exit_code}")

        # Create marker file to signal bot
        self.marker_file.touch()

    def wait_for_bot(self):
        """Wait for bot to complete"""
        print("⏳ Waiting for bot to finish processing...\n")

        if not self.bot_process:
            return

        # Wait for bot to finish
        self.bot_process.wait()

        exit_code = self.bot_process.poll()
        if exit_code == 0:
            print("\n✅ Bot completed successfully")
        else:
            print(f"\n⚠️  Bot exited with code {exit_code}")

    def run_query(self, query: str, max_repos: Optional[int] = None):
        """Run a single query with scanner and bot"""
        print("=" * 70)
        print(f"🚀 Starting OverWatch")
        print(f"   Query: {query}")
        if max_repos:
            print(f"   Max repos: {max_repos}")
        if self.dry_run:
            print(f"   Mode: DRY-RUN (no issues will be created)")
        print("=" * 70)
        print()

        # Start scanner
        if not self.run_scanner(query, max_repos):
            print("❌ Failed to start scanner")
            return

        # Give scanner a moment to start
        time.sleep(1)

        # Start bot
        if not self.run_bot():
            print("❌ Failed to start bot")
            self.cleanup()
            return

        # Wait for scanner to complete
        self.wait_for_scanner()

        # Wait for bot to complete
        self.wait_for_bot()

        # Cleanup
        self.cleanup()

        print("\n" + "=" * 70)
        print("✅ OverWatch completed")
        print("=" * 70)

    def run_all_queries(self):
        """Run all queries from the query bank"""
        print("Running all queries from query bank...")
        # TODO: Implement this by reading query_bank.yaml and running each query
        print("Not yet implemented - use single query mode for now")


def main():
    parser = argparse.ArgumentParser(
        description='OverWatch - Automated GitHub Secret Scanner',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Run a single query
  ./overwatch.py run "language:Python stars:<10"

  # Run with custom repo limit
  ./overwatch.py run "language:Python bot stars:<20" --max-repos 10

  # Dry-run mode (no issues created)
  ./overwatch.py run "language:Python stars:<5" --dry-run

  # Verbose mode (show scanner output)
  ./overwatch.py run "language:JavaScript stars:<10" --verbose
        """
    )

    subparsers = parser.add_subparsers(dest='command', help='Command to run')

    # Run command
    run_parser = subparsers.add_parser('run', help='Run a single query')
    run_parser.add_argument('query', type=str, help='GitHub search query')
    run_parser.add_argument('--max-repos', type=int, help='Maximum repositories to scan')
    run_parser.add_argument('--dry-run', action='store_true', help='Dry-run mode (no issues created)')
    run_parser.add_argument('--verbose', action='store_true', help='Show scanner output')

    # All command
    all_parser = subparsers.add_parser('all', help='Run all queries from query bank')
    all_parser.add_argument('--dry-run', action='store_true', help='Dry-run mode')

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        sys.exit(1)

    # Create orchestrator
    orchestrator = OverWatch(
        dry_run=args.dry_run if hasattr(args, 'dry_run') else False,
        verbose=args.verbose if hasattr(args, 'verbose') else False
    )

    # Check prerequisites
    if not orchestrator.check_prerequisites():
        sys.exit(1)

    # Run command
    if args.command == 'run':
        orchestrator.run_query(args.query, args.max_repos)
    elif args.command == 'all':
        orchestrator.run_all_queries()


if __name__ == '__main__':
    main()
