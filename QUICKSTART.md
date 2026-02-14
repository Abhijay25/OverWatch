# OverWatch Quick Start

Get up and running in 5 minutes!

## 1. Clone & Setup (1 minute)

```bash
git clone https://github.com/yourusername/OverWatch.git
cd OverWatch
nix-shell  # Downloads dependencies automatically
```

## 2. Configure GitHub Token (1 minute)

```bash
cp .env.example .env
nano .env  # Add your GitHub token
```

Get a token from: https://github.com/settings/tokens
- Required scope: `public_repo`

## 3. Build Scanner (1 minute)

```bash
cd scanner
cmake -B build -S .
cmake --build build
cd ..
```

## 4. Setup Bot (1 minute)

```bash
cd bot
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
cd ..
```

## 5. Run! (1 minute)

```bash
# Scan repositories (dry-run for safety)
cd scanner
nix-shell ../shell.nix --run "./build/scanner --max-repos 5 --dry-run"

# Check findings
cat ../data/findings.csv

# Create issues (dry-run first!)
cd ../bot
source venv/bin/activate
python bot.py --input ../data/findings.csv --dry-run
```

## Done! 🎉

For detailed instructions, see [USAGE.md](USAGE.md)

## Customization

**Change search keywords:**
```bash
nano config/keywords.yaml
```

**Add secret patterns:**
```bash
nano config/patterns.yaml
```

**Scanner options:**
```bash
./scanner/build/scanner --help
```

## Troubleshooting

**"GITHUB_TOKEN not set"**
→ Edit `.env` and add your token

**"Module not found"**
→ `source bot/venv/bin/activate && pip install -r requirements.txt`

**"Permission denied"**
→ `chmod +x bot/bot.py`

**Rate limit exhausted**
→ Wait or use authenticated token (5000/hour vs 60/hour)

## Safety Tips

✅ **Always use --dry-run first**
✅ **Test on your own repos before production**
✅ **Review findings manually for false positives**
✅ **Never commit .env file**

## Support

- 📖 Detailed guide: [USAGE.md](USAGE.md)
- 📋 Implementation details: [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)
- 🐛 Issues: https://github.com/yourusername/OverWatch/issues
