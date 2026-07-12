
import numpy as np
from PIL import Image
import sys

def load_pixels(filepath):
    with open(filepath, 'r') as f:
        content = f.read()
    tokens = [t.strip() for t in content.replace('\n', ',').split(',') if t.strip()]
    values = [int(t, 16) for t in tokens]
    return values

def rgb332_to_rgb888(byte_val):
    r = (byte_val >> 5) & 0x07
    g = (byte_val >> 2) & 0x07
    b = byte_val & 0x03

    r8 = (r * 255) // 7
    g8 = (g * 255) // 7
    b8 = (b * 255) // 3
    return (r8, g8, b8)

def main():
    filepath = sys.argv[1] if len(sys.argv) > 1 else "pixels.txt"
    width, height = 640, 480

    values = load_pixels(filepath)

    expected = width * height
    if len(values) < expected:
        raise ValueError(f"Not enough pixel values: got {len(values)}, expected {expected}")
    values = values[:expected]

    img_array = np.zeros((height, width, 3), dtype=np.uint8)
    for idx, val in enumerate(values):
        row = idx // width
        col = idx % width
        img_array[row, col] = rgb332_to_rgb888(val)#bgr322_to_rgb888(val)

    img = Image.fromarray(img_array, 'RGB')
    img.save("output_image.png")
    img.show()

if __name__ == "__main__":
    main()
