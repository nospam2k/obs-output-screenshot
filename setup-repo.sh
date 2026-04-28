#!/bin/bash
# setup-repo.sh — run once to initialise git and push to GitHub
# Usage: ./setup-repo.sh <your-github-username>
set -e

if [ -z "$1" ]; then
  echo "Usage: ./setup-repo.sh <github-username>"
  exit 1
fi

GITHUB_USER="$1"
REPO="obs-output-screenshot"

echo "→ Initialising git repo..."
git init
git add .
git commit -m "Initial commit: obs-output-screenshot plugin"

echo ""
echo "→ Next steps:"
echo ""
echo "  1. Create a new repo on GitHub:"
echo "     https://github.com/new"
echo "     Name it: ${REPO}"
echo "     Make it public (so Actions are free)"
echo "     Do NOT initialise with README/gitignore"
echo ""
echo "  2. Then run:"
echo "     git remote add origin git@github.com:${GITHUB_USER}/${REPO}.git"
echo "     git branch -M main"
echo "     git push -u origin main"
echo ""
echo "  3. GitHub Actions will immediately start building for macOS + Windows."
echo "     Check progress at: https://github.com/${GITHUB_USER}/${REPO}/actions"
echo ""
echo "  4. To trigger a release build with downloadable zips:"
echo "     git tag v1.0.0"
echo "     git push origin v1.0.0"
