# Nix Flake Setup

This project uses **Nix flakes** + **direnv** for automatic, reproducible development environments.

## ✨ What This Gives You

- ✅ **Automatic activation**: Environment loads when you `cd` into the project
- ✅ **Reproducible**: `flake.lock` pins exact versions of all dependencies
- ✅ **Self-contained**: Everything defined in `flake.nix`
- ✅ **No manual `nix-shell`**: direnv handles it automatically

## 🚀 First Time Setup

### 1. Enable Flakes (if not already)

```bash
# Check if flakes work
nix flake --version

# If error, enable them:
mkdir -p ~/.config/nix
echo "experimental-features = nix-command flakes" >> ~/.config/nix/nix.conf
```

### 2. Install direnv (if not already)

```bash
# NixOS - add to configuration.nix:
environment.systemPackages = with pkgs; [ direnv ];

# Or use nix-env:
nix-env -iA nixpkgs.direnv
```

### 3. Hook direnv into your shell

Add to `~/.bashrc` or `~/.zshrc`:

```bash
eval "$(direnv hook bash)"  # For bash
# OR
eval "$(direnv hook zsh)"   # For zsh
```

Then restart your shell or run `source ~/.bashrc`

## 📂 Project Structure

```
OverWatch/
├── flake.nix          # Nix flake definition (dependencies)
├── flake.lock         # Locked versions (like package-lock.json)
├── .envrc             # direnv config (just "use flake")
├── shell.nix          # ⚠️ DEPRECATED - can be removed
└── ...
```

## 🎯 How to Use

### Automatic (with direnv)

```bash
# Just cd into the project
cd ~/repos/OverWatch

# direnv automatically activates the environment!
# You'll see: "direnv: loading ~/repos/OverWatch/.envrc"
# Then: The OverWatch welcome message

# Now all tools are available:
cmake --version  # ✓ Works
gcc --version    # ✓ Works
python --version # ✓ Works

# Build your project:
cd scanner
cmake -B build -S .
cmake --build build
```

### Manual (without direnv)

```bash
# Enter development shell manually
nix develop

# Or run a single command in the environment
nix develop --command cmake --build build
```

## 🔧 Modifying Dependencies

### Add a C++ Library

1. Edit `flake.nix`:
```nix
buildInputs = with pkgs; [
  # ... existing packages ...
  curl  # ← Add new library
];
```

2. Update lock file:
```bash
nix flake update
```

3. Reload environment:
```bash
direnv reload
# OR
exit  # Then cd back into project
```

### Add a Python Package

For system packages, add to `flake.nix`:
```nix
python311Packages.requests
python311Packages.beautifulsoup4
```

For pip packages, use the venv as normal:
```bash
cd bot
source venv/bin/activate
pip install package-name
```

## 🔄 Updating Dependencies

```bash
# Update all inputs to latest versions
nix flake update

# Update specific input
nix flake update nixpkgs

# See what changed
git diff flake.lock
```

## 🗑️ Cleanup

### Remove shell.nix (now redundant)

```bash
# The flake.nix replaces shell.nix
git rm shell.nix
git commit -m "Remove shell.nix (replaced by flake.nix)"
```

### Clear direnv cache (if issues)

```bash
direnv reload
```

## 🆚 flake.nix vs shell.nix

| Feature | shell.nix | flake.nix |
|---------|-----------|-----------|
| **Activation** | Manual `nix-shell` | Automatic via direnv |
| **Lock file** | ❌ No | ✅ Yes (`flake.lock`) |
| **Reproducible** | Mostly | Fully |
| **Standard** | Old way | New way |
| **Sharing** | Works | Better |

## 📖 Common Commands

```bash
# Enter dev shell manually
nix develop

# Run command in dev shell
nix develop --command cmake --build build

# Update dependencies
nix flake update

# Check flake structure
nix flake show

# See what's in the environment
nix develop --command env | grep PATH
```

## 🐛 Troubleshooting

### direnv not activating

```bash
# Check if hook is installed
echo $DIRENV_DIR  # Should show something when in project

# Re-allow .envrc
direnv allow

# Check for errors
direnv status
```

### "Git tree is dirty" warning

This is normal - it just means you have uncommitted changes. The flake still works.

To suppress:
```bash
git add -A  # Stage all changes
# OR
git commit -m "WIP"  # Commit changes
```

### Environment not updating after changing flake.nix

```bash
direnv reload
# OR
nix flake update
direnv reload
```

## ✅ Benefits

**Before (shell.nix):**
```bash
cd ~/repos/OverWatch
nix-shell  # ← Manual step, easy to forget
# ... do work ...
exit       # ← Leave shell
```

**After (flake.nix + direnv):**
```bash
cd ~/repos/OverWatch  # ← Automatically activated!
# ... do work ...
cd ..                 # ← Automatically deactivated!
```

**Plus:** Exact dependency versions locked in `flake.lock`!
