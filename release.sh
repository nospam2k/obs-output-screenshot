#!/usr/bin/env bash
set -euo pipefail

# Commit any changes, push, and optionally tag a release.
# Usage:
#   ./release.sh                   # commit + push (triggers CI build, no release)
#   ./release.sh --release         # commit + push + bump patch tag (triggers release)
#   ./release.sh --release v1.2.0  # commit + push + specific tag (triggers release)

RELEASE=false
TAG=""

for arg in "$@"; do
  case "$arg" in
    --release) RELEASE=true ;;
    v*) TAG="$arg" ;;
    *) echo "Unknown argument: $arg"; exit 1 ;;
  esac
done

# --- commit staged + unstaged changes ---
if git diff --quiet && git diff --cached --quiet; then
  echo "Nothing to commit."
else
  echo "Changed files:"
  git status --short
  echo
  read -rp "Commit message: " MSG
  [[ -z "$MSG" ]] && { echo "Commit message required."; exit 1; }
  git add -A
  git commit -m "$MSG"
fi

git push origin main
echo "Pushed to main — CI build triggered."

# --- tag for release ---
if $RELEASE; then
  if [[ -z "$TAG" ]]; then
    LATEST=$(git tag --sort=-v:refname | grep -E '^v[0-9]+\.[0-9]+\.[0-9]+$' | head -1)
    if [[ -z "$LATEST" ]]; then
      TAG="v1.0.0"
    else
      IFS='.' read -r MAJOR MINOR PATCH <<< "${LATEST#v}"
      TAG="v${MAJOR}.${MINOR}.$((PATCH + 1))"
    fi
  fi
  read -rp "Create and push tag $TAG? [y/N] " CONFIRM
  [[ "$CONFIRM" =~ ^[Yy]$ ]] || { echo "Aborted."; exit 0; }
  git tag "$TAG"
  git push origin "$TAG"
  echo "Tagged $TAG — release build triggered."
fi
