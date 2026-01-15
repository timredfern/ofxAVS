#!/bin/bash
# Release script for ofxAVS
# Creates a Gitea release and uploads the DMG

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
GITEA_URL="https://git.eclectronics.org"
REPO="timredfern/ofxAVS"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check for required tools
check_requirements() {
    if ! command -v curl &> /dev/null; then
        echo -e "${RED}Error: curl is required${NC}"
        exit 1
    fi
    if ! command -v jq &> /dev/null; then
        echo -e "${RED}Error: jq is required (brew install jq)${NC}"
        exit 1
    fi
}

# Get version from argument, git tag, or prompt
get_version() {
    if [ -n "$1" ]; then
        VERSION="$1"
    else
        # Try to get from latest git tag
        VERSION=$(git describe --tags --abbrev=0 2>/dev/null || echo "")
        if [ -z "$VERSION" ]; then
            echo -e "${YELLOW}No git tag found.${NC}"
            read -p "Enter version (e.g., v1.0.0): " VERSION
        else
            echo -e "Latest tag: ${GREEN}$VERSION${NC}"
            read -p "Use this version? [Y/n]: " confirm
            if [[ "$confirm" =~ ^[Nn] ]]; then
                read -p "Enter version: " VERSION
            fi
        fi
    fi

    # Ensure version starts with 'v'
    if [[ ! "$VERSION" =~ ^v ]]; then
        VERSION="v$VERSION"
    fi

    echo "$VERSION"
}

# Check if tag exists, create if not
ensure_tag() {
    local version="$1"
    if ! git rev-parse "$version" &>/dev/null; then
        echo -e "${YELLOW}Tag $version does not exist.${NC}"
        read -p "Create tag $version? [Y/n]: " confirm
        if [[ ! "$confirm" =~ ^[Nn] ]]; then
            git tag -a "$version" -m "Release $version"
            echo -e "${GREEN}Created tag $version${NC}"
        else
            echo -e "${RED}Aborting: tag required for release${NC}"
            exit 1
        fi
    fi
}

# Detect architecture
detect_arch() {
    local arch_raw=$(uname -m)
    case "$arch_raw" in
        arm64)  echo "arm64" ;;
        x86_64) echo "intel" ;;
        *)      echo "$arch_raw" ;;
    esac
}

# Build the release DMG
build_release() {
    echo -e "${GREEN}Building release...${NC}"
    cd "$PROJECT_DIR/examples/chain"
    make package

    local arch=$(detect_arch)
    DMG_PATH="$PROJECT_DIR/examples/chain/bin/release/chain-${arch}.dmg"
    if [ ! -f "$DMG_PATH" ]; then
        echo -e "${RED}Error: DMG not found at $DMG_PATH${NC}"
        exit 1
    fi
    echo "$DMG_PATH"
}

# Create Gitea release and upload asset
create_release() {
    local version="$1"
    local dmg_path="$2"
    local token="$3"

    # Release name without 'v' prefix for display
    local release_name="ofxAVS ${version}"
    local arch=$(detect_arch)
    local dmg_name="ofxAVS-${version}-macOS-${arch}.dmg"

    echo -e "${GREEN}Creating release $version on Gitea...${NC}"

    # Check if release already exists
    existing=$(curl -s "${GITEA_URL}/api/v1/repos/${REPO}/releases/tags/${version}" \
        -H "Authorization: token $token" | jq -r '.id // empty')

    if [ -n "$existing" ]; then
        echo -e "${YELLOW}Release $version already exists (id: $existing)${NC}"
        read -p "Delete and recreate? [y/N]: " confirm
        if [[ "$confirm" =~ ^[Yy] ]]; then
            curl -s -X DELETE "${GITEA_URL}/api/v1/repos/${REPO}/releases/${existing}" \
                -H "Authorization: token $token"
            echo -e "${GREEN}Deleted existing release${NC}"
        else
            echo -e "${RED}Aborting${NC}"
            exit 1
        fi
    fi

    # Build release body
    local body="macOS application for AVS (Advanced Visualization Studio) - the legendary Winamp visualizer.

## Installation

1. Download the DMG for your Mac:
   - **Apple Silicon** (M1/M2/M3): \`ofxAVS-${version}-macOS-arm64.dmg\`
   - **Intel**: \`ofxAVS-${version}-macOS-intel.dmg\`
2. Open the DMG and drag to Applications
3. Run ofxAVS from Applications

## Features

- 46+ effects ported from original AVS
- Load original .avs preset files
- Real-time audio visualization (mic or audio file)
- Beat detection with BPM display
- Effect profiling (press P)

## Controls

| Key | Action |
|-----|--------|
| Space | Play/pause audio file |
| P | Toggle effect profiling |
| Drag & drop | Load .avs presets or audio files |

## Requirements

- macOS 11.0 or later"

    # Create the release
    release_response=$(curl -s -X POST "${GITEA_URL}/api/v1/repos/${REPO}/releases" \
        -H "Authorization: token $token" \
        -H "Content-Type: application/json" \
        -d "$(jq -n \
            --arg tag "$version" \
            --arg name "$release_name" \
            --arg body "$body" \
            '{tag_name: $tag, name: $name, body: $body, draft: false, prerelease: false}')")

    release_id=$(echo "$release_response" | jq -r '.id')
    if [ "$release_id" = "null" ] || [ -z "$release_id" ]; then
        echo -e "${RED}Failed to create release:${NC}"
        echo "$release_response" | jq .
        exit 1
    fi

    echo -e "${GREEN}Created release (id: $release_id)${NC}"

    # Upload the DMG
    echo -e "${GREEN}Uploading $dmg_name...${NC}"
    upload_response=$(curl -s -X POST \
        "${GITEA_URL}/api/v1/repos/${REPO}/releases/${release_id}/assets?name=${dmg_name}" \
        -H "Authorization: token $token" \
        -H "Content-Type: application/octet-stream" \
        --data-binary "@${dmg_path}")

    asset_id=$(echo "$upload_response" | jq -r '.id')
    if [ "$asset_id" = "null" ] || [ -z "$asset_id" ]; then
        echo -e "${RED}Failed to upload asset:${NC}"
        echo "$upload_response" | jq .
        exit 1
    fi

    echo -e "${GREEN}Uploaded DMG (asset id: $asset_id)${NC}"
    echo -e "${GREEN}Release URL: ${GITEA_URL}/${REPO}/releases/tag/${version}${NC}"
}

# Main
main() {
    check_requirements

    cd "$PROJECT_DIR"

    # Check for Gitea token
    if [ -z "$GITEA_TOKEN" ]; then
        echo -e "${YELLOW}GITEA_TOKEN not set.${NC}"
        echo "Create a token at: ${GITEA_URL}/user/settings/applications"
        read -sp "Enter Gitea token: " GITEA_TOKEN
        echo
    fi

    # Get version
    VERSION=$(get_version "$1")
    echo -e "Version: ${GREEN}$VERSION${NC}"

    # Ensure tag exists
    ensure_tag "$VERSION"

    # Build
    DMG_PATH=$(build_release)
    echo -e "DMG: ${GREEN}$DMG_PATH${NC}"

    # Create release
    create_release "$VERSION" "$DMG_PATH" "$GITEA_TOKEN"

    echo -e "${GREEN}Release complete!${NC}"
}

main "$@"
