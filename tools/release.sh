#!/bin/bash
# Release script - sync master to main, optionally tag, and push
# Usage: ./tools/release.sh [version]
# Examples:
#   ./tools/release.sh v1.0.2   # Sync and tag
#   ./tools/release.sh          # Sync only (no tag)

set -e

VERSION="$1"

OFXAVS_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
AVSLIB_ROOT="$OFXAVS_ROOT/libs/avs_lib"

# Files to exclude from main (dev-only files)
EXCLUDE_FILES=(
    "CLAUDE.md"
    "GEMINI.md"
    "WORKLOG.md"
    "RELEASE_WORKFLOW.md"
    "docs/BEAT_DETECTION.md"
    "docs/NODE_GRAPH_PLAN.md"
    "docs/optimisation_dialog.txt"
    "examples/AVS_standard/debug_output.txt"
)

AVSLIB_EXCLUDE_FILES=(
    "APE.md"
    "AVS_PARAMS.md"
    "COORDINATEGRID.md"
    "GPU.md"
    "LASER.md"
    "OLDSTYLE.md"
    "OPTIMISATION.md"
    "PLUGINS.md"
    "SCRIPT_ARCHITECTURE.md"
    "docs/AUDIO.md"
    "docs/RESAMPLING.md"
    "tools/TESTING.md"
)

echo "=== Releasing $VERSION ==="

# 1. Update avs_lib main from master
echo ""
echo "=== avs_lib: syncing main from master ==="
cd "$AVSLIB_ROOT"
git checkout main
git merge --squash --allow-unrelated-histories master || true  # May fail if already up to date
# Resolve any conflicts by taking master's version
CONFLICTED=$(git diff --name-only --diff-filter=U 2>/dev/null || true)
if [ -n "$CONFLICTED" ]; then
    echo "Resolving conflicts (taking master's version)..."
    echo "$CONFLICTED" | xargs -I {} git checkout --theirs {} 2>/dev/null || true
    echo "$CONFLICTED" | xargs git add 2>/dev/null || true
fi
for f in "${AVSLIB_EXCLUDE_FILES[@]}"; do
    [ -f "$f" ] && git rm -f "$f" 2>/dev/null || true
done
if [ -n "$VERSION" ]; then
    git diff --cached --quiet || git commit -m "$VERSION - sync from master"
    git tag -f "$VERSION"
    echo "avs_lib: tagged $VERSION"
else
    git diff --cached --quiet || git commit -m "Sync from master"
    echo "avs_lib: synced (no tag)"
fi

# 2. Update ofxAVS main from master
echo ""
echo "=== ofxAVS: syncing main from master ==="
cd "$OFXAVS_ROOT"
git checkout main
git merge --squash --allow-unrelated-histories master || true
# Resolve any conflicts by taking master's version
CONFLICTED=$(git diff --name-only --diff-filter=U 2>/dev/null || true)
if [ -n "$CONFLICTED" ]; then
    echo "Resolving conflicts (taking master's version)..."
    echo "$CONFLICTED" | xargs -I {} git checkout --theirs {} 2>/dev/null || true
    echo "$CONFLICTED" | xargs git add 2>/dev/null || true
fi
for f in "${EXCLUDE_FILES[@]}"; do
    [ -f "$f" ] && git rm -f "$f" 2>/dev/null || true
done

# 3. Update .gitmodules to use GitHub URL (for public cloning)
echo ""
echo "=== Updating submodule URL for GitHub ==="
sed -i '' 's|ssh://git@git.eclectronics.org:2222/timredfern/avs_lib.git|git@github.com:timredfern/avs_lib.git|' .gitmodules
git add .gitmodules

# 4. Update submodule to avs_lib main
echo ""
echo "=== Updating avs_lib submodule reference ==="
cd "$AVSLIB_ROOT"
git checkout main
cd "$OFXAVS_ROOT"
git add libs/avs_lib

# 5. Commit and tag ofxAVS
if [ -n "$VERSION" ]; then
    git diff --cached --quiet || git commit -m "$VERSION - sync from master"
    git tag -f "$VERSION"
    echo "ofxAVS: tagged $VERSION"
else
    git diff --cached --quiet || git commit -m "Sync from master"
    echo "ofxAVS: synced (no tag)"
fi

# 6. Push both
echo ""
echo "=== Pushing to GitHub ==="
cd "$AVSLIB_ROOT"
git push github main
[ -n "$VERSION" ] && git push github "$VERSION" --force

cd "$OFXAVS_ROOT"
git push github main
[ -n "$VERSION" ] && git push github "$VERSION" --force

# 7. Return to master
echo ""
echo "=== Returning to master ==="
cd "$AVSLIB_ROOT"
git checkout master
cd "$OFXAVS_ROOT"
git checkout master

echo ""
echo "=== Done! ==="
if [ -n "$VERSION" ]; then
    echo "avs_lib main and ofxAVS main updated and tagged $VERSION"
else
    echo "avs_lib main and ofxAVS main synced from master"
fi
echo "Pushed to GitHub"
