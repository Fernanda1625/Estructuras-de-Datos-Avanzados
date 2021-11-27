from rgb_color import RGB_Color

class OctreeNode(object):
    """
    clase nodo de octree para cuantificación
    """

    def __init__(self,level, parent):
        """
        inicializar nuevo nodo octárbol
        : param level
        : param parent:
        """
        self.color = RGB_Color(0,0,0)
        self.pixel_count = 0
        self.palette_index = 0
        self.children = [None for _ in xrange(8)]
        # agregar nodo al nivel actual
        if level < OctreeQuantizer.MAX_DEPTH - 1:
            parent.add_level_node(level, self)

    def is_leaf(self):
        """
        comprobar si el nodo es hoja
        :return:
        """
        return self.pixel_count > 0

    def get_leaf_nodes(self):
        """
        obtener todos los nodos hoja
        :return:
        """
        leaf_nodes = []
        for i in xrange(8):
            node = self.children[i]
            if node:
                if node.is_leaf():
                    leaf_nodes.append(node)
                else:
                    leaf_nodes.extend(node.get_leaf_nodes())
        return leaf_nodes

    def get_nodes_pixel_count(self):
        """
        obtener una suma del recuento de píxeles para el nodo y sus hijos
        :return:
        """
        sum_count = self.pixel_count
        for i in xrange(8):
            node = self.children[i]
            if node:
                sum_count += node.pixel_count
        return sum_count

    def add_color(self, color, level, parent):
        """
        Agrega `color` al árbol
        """
        if level >= OctreeQuantizer.MAX_DEPTH:
            self.color.red += color.red
            self.color.green += color.green
            self.color.blue += color.blue
            self.pixel_count += 1
            return
        index = self.get_color_index_for_level(color, level)
        if not self.children[index]:
            self.children[index] = OctreeNode(level, parent)
        self.children[index].add_color(color, level + 1, parent)

    def get_palette_index(self, color, level):
        """
        Obtener índice de paleta para 'color'
          usa 'nivel' para profundizar si el nodo no es una hoja
        :param color:
        :param level:
        :return:
        """
        if self.is_leaf():
            return self.palette_index
        index = self.get_color_index_for_level(color, level)
        if self.children[index]:
            return self.children[index].get_palette_index(color, level+1)
        else:
            # obtener el índice de paleta para el primer nodo hijo encontrado
            for i in xrange(8):
                if self.children[i]:
                    return self.children[i].get_palette_index(color, level+1)

    def remove_leaves(self):
        """
        agregue todos los canales de color y recuento de píxeles secundarios al nodo principal
        :return: la cantidad de hojas removidas
        """
        result = 0
        for i in xrange(8):
            node = self.children[i]
            if node:
                self.color.red += node.color.red
                self.color.green += node.color.green
                self.color.blue += node.color.blue
                self.pixel_count += node.pixel_count
                result += 1
        return result - 1

    def get_color_index_for_level(self,color,level):
        """
        obtener el índice para el siguiente 'nivel'
        :param color:
        :param level:
        :return:
        """
        index = 0
        mask = 0x80 >> level
        if color.red & mask:
            index |= 4
        if color.green & mask:
            index |= 2
        if color.blue & mask:
            index |= 1
        return index

    def get_color(self):
        """
        Obtenga un color promedio
        :return:
        """
        return RGB_Color(
            self.color.red / self.pixel_count,
            self.color.green / self.pixel_count,
            self.color.blue / self.pixel_count)


class OctreeQuantizer(object):
    """
    Clase de cuantificador de octárbol para cuantificación de imágenes
      use MAX_DEPTH para limitar una cantidad de niveles
    """

    MAX_DEPTH = 8

    def __init__(self):
        """
        cuantificador de octree init
        """
        self.levels = {i: [] for i in xrange(OctreeQuantizer.MAX_DEPTH)}
        self.root = OctreeNode(0, self)

    def get_leaves(self):
        """
        conseguir todas las hojas
        :return:
        """
        return [node for node in self.root.get_leaf_nodes()]

    def add_level_node(self, level, node):
        """
        agregar 'nodo' a los nodos en el 'nivel'
        :param level:
        :param node:
        :return:
        """
        self.levels[level].append(node)

    def add_color(self,color):
        """
        agregar 'color' al Octree
        :param color:
        :return:
        """
        #  pasa el valor propio como 'padre' para guardar los nodos en los niveles dictados
        self.root.add_color(color, 0, self)

    def make_palette(self, color_count):
        """
        hacer que la paleta de colores tenga el máximo de colores 'color_count'
        :param color_count:
        :return:
        """
        palette = []
        palette_index = 0
        leaf_count = len(self.get_leaves())
        """
        reducir los nodos
        se pueden reducir hasta 8 colores y el menor número de colores debe ser 248
        """
        for level in xrange(OctreeQuantizer.MAX_DEPTH - 1, -1, -1):
            if self.levels[level]:
                for node in self.levels[level]:
                    leaf_count -= node.remove_leaves()
                    if leaf_count <= color_count:
                        break
                if leaf_count <= color_count:
                    break
                self.levels[level] = []
        # paleta de construcción
        for node in self.get_leaves():
            if palette_index >= color_count:
                break
            if node.is_leaf():
                palette.append(node.get_color())
            node.palette_index = palette_index
            palette_index += 1
        return palette

    def get_palette_index(self,color):
        """
        obtener índice de paleta para 'color'
        :param color:
        :return:
        """
        return self.root.get_palette_index(color, 0)