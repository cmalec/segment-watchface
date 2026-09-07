from PIL import Image

im = Image.open('/tmp/emu-shots/seg1.png').convert('L')
w, h = im.size
px = im.load()

# Full-screen 3x3 downsample ASCII to see the whole face at a glance
# 200x228 -> 66x76 chars (every 3rd px)
for y in range(0, h, 3):
    row = ''
    for x in range(0, w, 3):
        vals = [px[x+dx, y+dy] for dx in (0,1,2) if x+dx < w for dy in (0,1,2) if y+dy < h]
        avg = sum(vals)/len(vals)
        row += '#' if avg > 128 else ('+' if avg > 60 else '.')
    print(row)
