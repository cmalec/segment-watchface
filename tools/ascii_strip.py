from PIL import Image

im = Image.open('/tmp/emu-shots/seg1.png').convert('L')
w, h = im.size
px = im.load()

# Render the bottom strip (y 200..228) as ASCII art to SEE what's there.
# 2x2 downsample to 100 wide x 14 tall
for y in range(200, 228, 2):
    row = ''
    for x in range(0, 200, 2):
        vals = [px[x+dx, y+dy] for dx in (0,1) for dy in (0,1)]
        avg = sum(vals)/4
        row += '#' if avg > 128 else ('+' if avg > 60 else '.')
    print(row)
