from PIL import Image

im = Image.open('/tmp/emu-shots/seg3.png').convert('L')
px = im.load()
w, h = im.size

# ASCII of the top strip (y 50..85), full width, 2x downsample
for y in range(50, 86, 2):
    row = ''
    for x in range(0, w, 2):
        vals = [px[x+dx, y+dy] for dx in (0,1) if x+dx < w for dy in (0,1) if y+dy < h]
        avg = sum(vals)/len(vals)
        row += '#' if avg > 128 else ('+' if avg > 60 else '.')
    print(f'{y:3d} {row}')
