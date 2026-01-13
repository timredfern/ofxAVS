#!/bin/bash
# Generate a DMG background image with an arrow
# Usage: create-dmg-background.sh <output-path> [width] [height]

OUTPUT="${1:-background.png}"
WIDTH="${2:-540}"
HEIGHT="${3:-380}"

# Create SVG with arrow
SVG_CONTENT="<svg xmlns='http://www.w3.org/2000/svg' width='$WIDTH' height='$HEIGHT'>
  <defs>
    <linearGradient id='bg' x1='0%' y1='0%' x2='0%' y2='100%'>
      <stop offset='0%' style='stop-color:#3a3a3a'/>
      <stop offset='100%' style='stop-color:#2a2a2a'/>
    </linearGradient>
  </defs>
  <rect width='100%' height='100%' fill='url(#bg)'/>
  <!-- Arrow pointing right -->
  <g transform='translate($((WIDTH/2)), $((HEIGHT/2 + 20)))'>
    <path d='M-40,0 L30,0 M10,-20 L30,0 L10,20'
          stroke='#666666'
          stroke-width='8'
          stroke-linecap='round'
          stroke-linejoin='round'
          fill='none'/>
  </g>
  <!-- Drag to install text -->
  <text x='$((WIDTH/2))' y='$((HEIGHT - 50))'
        font-family='Helvetica Neue, Arial, sans-serif'
        font-size='13'
        fill='#888888'
        text-anchor='middle'>
    Drag to Applications to install
  </text>
</svg>"

# Check if we can use rsvg-convert or sips
if command -v rsvg-convert &> /dev/null; then
    echo "$SVG_CONTENT" | rsvg-convert -o "$OUTPUT"
elif command -v convert &> /dev/null; then
    echo "$SVG_CONTENT" | convert svg:- "$OUTPUT"
else
    # Fallback: create a simple solid background using sips
    # First create a tiny PNG, then resize
    # This won't have the arrow but will work

    # Create a 1x1 pixel PNG with the background color using printf and xxd
    # Actually, let's use Python if available
    if command -v python3 &> /dev/null; then
        python3 << PYEOF
from PIL import Image, ImageDraw, ImageFont
import sys

try:
    # Create image with gradient-like solid color
    img = Image.new('RGB', ($WIDTH, $HEIGHT), '#303030')
    draw = ImageDraw.Draw(img)

    # Draw arrow
    cx, cy = $WIDTH // 2, $HEIGHT // 2 + 20
    arrow_color = '#666666'
    line_width = 6

    # Arrow line
    draw.line([(cx - 40, cy), (cx + 30, cy)], fill=arrow_color, width=line_width)
    # Arrow head
    draw.line([(cx + 10, cy - 20), (cx + 30, cy)], fill=arrow_color, width=line_width)
    draw.line([(cx + 10, cy + 20), (cx + 30, cy)], fill=arrow_color, width=line_width)

    # Text
    try:
        font = ImageFont.truetype('/System/Library/Fonts/Helvetica.ttc', 13)
    except:
        font = ImageFont.load_default()

    text = "Drag to Applications to install"
    bbox = draw.textbbox((0, 0), text, font=font)
    text_width = bbox[2] - bbox[0]
    draw.text(($WIDTH // 2 - text_width // 2, $HEIGHT - 60), text, fill='#888888', font=font)

    img.save('$OUTPUT')
except ImportError:
    # No PIL, create blank image
    img = Image.new('RGB', ($WIDTH, $HEIGHT), '#303030')
    img.save('$OUTPUT')
except Exception as e:
    print(f"Warning: Could not create background: {e}", file=sys.stderr)
    sys.exit(1)
PYEOF
    else
        echo "Warning: Cannot create background image (no rsvg-convert, convert, or python3 with PIL)"
        exit 1
    fi
fi

echo "Created: $OUTPUT"
