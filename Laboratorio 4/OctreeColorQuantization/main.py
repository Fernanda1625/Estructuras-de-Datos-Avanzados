from PIL import Image
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import  matplotlib.colors as mcolors
from rgb_color import RGB_Color
from rgb_quantizer import OctreeQuantizer

#  ---- inserta todos los colores de la imagen en octree
def quantize_color(path,num):
    image = Image.open(path)
    pixels = image.load()
    width, height = image.size

    octree = OctreeQuantizer()

    # ---- agregar colores a octree
    for j in xrange(height):
        for i in xrange(width):
            octree.add_color(RGB_Color(*pixels[i, j]))

    # ---- 256 colores para "num" bits por imagen de salida de píxel
    palette_object= octree.make_palette(num)

    # ---- guardar imagen de salida
    out_image = Image.new('RGB', (width, height))
    out_pixels = out_image.load()
    for j in xrange(height):
        for i in xrange(width):
            index = octree.get_palette_index(RGB_Color(*pixels[i, j]))
            color = palette_object[index]
            out_pixels[i, j] = (color.red, color.green, color.blue)
    out_image.save('img/sky/quantized_img/img%02d.png' % num)

    # ---- obtener la matriz de paleta de colores RGB
    rgb_palette = []
    for color in palette_object:
        R = color.red / 255.0
        G = color.green / 255.0
        B = color.blue / 255.0
        rgb = [R,G,B]
        rgb_palette.append(rgb)

    # ---- visualizar la paleta de colores
    img_palette = mcolors.ListedColormap(rgb_palette)
    plt.figure(figsize=(num, 0.5))
    plt.title('color theme')
    plt.pcolormesh(np.arange(img_palette.N).reshape(1, -1), cmap=img_palette)
    plt.gca().yaxis.set_visible(False)
    plt.gca().set_xlim(0, img_palette.N)
    plt.axis('off')
    plt.savefig('img/sky/quantized_palette/img_palette%02d.png' % num)

def main():
  for i in [20]:
      quantize_color('/img/sky.jpg',i)

if __name__ == '__main__':
    main()