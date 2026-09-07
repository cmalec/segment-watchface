from PIL import Image

im = Image.open('/tmp/emu-shots/seg1.png').convert('L')
w, h = im.size
print('screenshot size:', w, h)
px = im.load()

def bright_count(x0, y0, x1, y1, thresh=100):
    n = 0
    for y in range(max(0, y0), min(h, y1)):
        for x in range(max(0, x0), min(w, x1)):
            if px[x, y] > thresh:
                n += 1
    return n

# Bottom-left region where TEMP_HI should render (GRect 0,211,64,28 emery)
print('bottom-left (0..64, 205..228) bright px:', bright_count(0, 205, 64, 228))
# Bottom-right region TEMP_LO (GRect 138,211,67,28)
print('bottom-right (138..200, 205..228) bright px:', bright_count(138, 205, 200, 228))
# WR box region (71..129, 206..228) - should have the outline
print('WR box (71..129, 206..228) bright px:', bright_count(71, 206, 129, 228))
# Battery percent area (sanity, should have text)
print('battery pct (115..164, 57..79) bright px:', bright_count(115, 57, 164, 79))
# Health area (sanity)
print('health (25..140, 57..79) bright px:', bright_count(25, 57, 140, 79))
