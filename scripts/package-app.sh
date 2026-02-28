#!/bin/bash
# Package an OpenFrameworks app as a distributable macOS .app bundle
# Usage: package-app.sh <app-name> [version] [bin-dir]

set -e

APP_NAME="${1:-ofxAVSExample}"
VERSION="${2:-}"
BIN_DIR="${3:-.}"
APP_PATH="$BIN_DIR/${APP_NAME}.app"
RELEASE_DIR="$BIN_DIR/release"

# Detect architecture
ARCH_RAW=$(uname -m)
case "$ARCH_RAW" in
    arm64)  ARCH="arm64" ;;
    x86_64) ARCH="intel" ;;
    *)      ARCH="$ARCH_RAW" ;;
esac

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check if .app exists
if [ ! -d "$APP_PATH" ]; then
    log_error "App not found: $APP_PATH"
    log_info "Build the app first with: make Release"
    exit 1
fi

log_info "Packaging $APP_NAME..."

# Create release directory
mkdir -p "$RELEASE_DIR"

# Copy .app to release directory
log_info "Copying app bundle..."
rm -rf "$RELEASE_DIR/${APP_NAME}.app"
cp -R "$APP_PATH" "$RELEASE_DIR/"

# Fix dylib paths if needed
MACOS_DIR="$RELEASE_DIR/${APP_NAME}.app/Contents/MacOS"
FRAMEWORKS_DIR="$RELEASE_DIR/${APP_NAME}.app/Contents/Frameworks"

# Find and bundle any external dylibs
log_info "Checking for external dependencies..."
EXECUTABLE="$MACOS_DIR/$APP_NAME"

if [ -f "$EXECUTABLE" ]; then
    # Get list of non-system dylibs
    DYLIBS=$(otool -L "$EXECUTABLE" 2>/dev/null | grep -v "^\s*/System" | grep -v "^\s*/usr/lib" | grep -v "@executable_path" | grep -v "@rpath" | grep "dylib" | awk '{print $1}' || true)

    if [ -n "$DYLIBS" ]; then
        log_info "Found external dylibs, bundling..."
        mkdir -p "$FRAMEWORKS_DIR"

        for DYLIB in $DYLIBS; do
            if [ -f "$DYLIB" ]; then
                DYLIB_NAME=$(basename "$DYLIB")
                log_info "  Bundling: $DYLIB_NAME"
                cp "$DYLIB" "$FRAMEWORKS_DIR/"

                # Fix the reference in the executable
                install_name_tool -change "$DYLIB" "@executable_path/../Frameworks/$DYLIB_NAME" "$EXECUTABLE" 2>/dev/null || true
            fi
        done
    else
        log_info "No external dylibs to bundle"
    fi
fi

# Ad-hoc code sign the app (required for Apple Silicon)
log_info "Code signing app..."
codesign --force --deep --sign - "$RELEASE_DIR/${APP_NAME}.app" 2>/dev/null || {
    log_warn "Code signing failed (may need Xcode command line tools)"
}

# Verify the app works
log_info "Verifying app bundle..."
if [ -x "$EXECUTABLE" ]; then
    log_info "Executable found and is runnable"
else
    log_warn "Executable may have issues"
fi

# Create styled DMG with Applications symlink
log_info "Creating DMG for $ARCH..."
if [ -n "$VERSION" ]; then
    DMG_PATH="$RELEASE_DIR/${APP_NAME}-${VERSION}-${ARCH}.dmg"
else
    DMG_PATH="$RELEASE_DIR/${APP_NAME}-${ARCH}.dmg"
fi
DMG_TEMP="$RELEASE_DIR/.dmg-temp.dmg"
DMG_STAGING="$RELEASE_DIR/.dmg-staging"
VOLUME_NAME="$APP_NAME"
DMG_WINDOW_WIDTH=540
DMG_WINDOW_HEIGHT=380
ICON_SIZE=128
APP_ICON_X=135
APP_ICON_Y=170
APPLICATIONS_ICON_X=405
APPLICATIONS_ICON_Y=170

rm -f "$DMG_PATH" "$DMG_TEMP"
rm -rf "$DMG_STAGING"

# Create staging directory with app and Applications symlink
mkdir -p "$DMG_STAGING"
cp -R "$RELEASE_DIR/${APP_NAME}.app" "$DMG_STAGING/"
ln -s /Applications "$DMG_STAGING/Applications"


# Calculate DMG size (app size + 20MB buffer)
DMG_SIZE_MB=$(( $(du -sm "$DMG_STAGING" | cut -f1) + 20 ))

# Create read-write DMG
log_info "Creating DMG image..."
hdiutil create \
    -volname "$VOLUME_NAME" \
    -srcfolder "$DMG_STAGING" \
    -fs HFS+ \
    -fsargs "-c c=64,a=16,e=16" \
    -format UDRW \
    -size ${DMG_SIZE_MB}m \
    "$DMG_TEMP" 2>/dev/null

# Mount the DMG
log_info "Configuring DMG appearance..."
MOUNT_DIR="/Volumes/$VOLUME_NAME"

# Unmount if already mounted
hdiutil detach "$MOUNT_DIR" 2>/dev/null || true

# Mount the temp DMG (no -nobrowse so Finder AppleScript can access it)
hdiutil attach "$DMG_TEMP" -mountpoint "$MOUNT_DIR" -quiet


# Use AppleScript to configure the DMG window (icon positions, no background)
osascript << APPLESCRIPT
    tell application "Finder"
        tell disk "$VOLUME_NAME"
            open
            set current view of container window to icon view
            set toolbar visible of container window to false
            set statusbar visible of container window to false
            set bounds of container window to {100, 100, $((100 + DMG_WINDOW_WIDTH)), $((100 + DMG_WINDOW_HEIGHT))}
            set viewOptions to the icon view options of container window
            set arrangement of viewOptions to not arranged
            set icon size of viewOptions to $ICON_SIZE
            set position of item "$APP_NAME.app" of container window to {$APP_ICON_X, $APP_ICON_Y}
            set position of item "Applications" of container window to {$APPLICATIONS_ICON_X, $APPLICATIONS_ICON_Y}
            close
            open
            update without registering applications
            delay 1
            close
        end tell
    end tell
APPLESCRIPT

# Give Finder time to update
sleep 2

# Unmount
hdiutil detach "$MOUNT_DIR" -quiet

# Convert to compressed read-only DMG
log_info "Compressing DMG..."
hdiutil convert "$DMG_TEMP" -format UDZO -imagekey zlib-level=9 -o "$DMG_PATH" -quiet

# Clean up
rm -f "$DMG_TEMP"
rm -rf "$DMG_STAGING"

if [ -f "$DMG_PATH" ]; then
    DMG_SIZE=$(du -h "$DMG_PATH" | cut -f1)
    log_info "Created: $DMG_PATH ($DMG_SIZE)"
else
    log_warn "DMG creation failed"
fi

# Summary
echo ""
log_info "=== Packaging Complete ==="
log_info "App:   $RELEASE_DIR/${APP_NAME}.app"
[ -f "$DMG_PATH" ] && log_info "DMG:   $DMG_PATH"
echo ""
log_info "To test: open \"$RELEASE_DIR/${APP_NAME}.app\""
log_info "To distribute: share the .dmg file"
