"""
Construye el objeto de color RGB para realizar la cuantificación del color
"""
class RGB_Color(object):

    def __init__(self, red = 0, green = 0, blue = 0, alpha = None):
        """
        inicializar color
        :param red:
        :param green:
        :param blue:
        :param alpha:
        """
        self.red = red
        self.green = green
        self.blue = blue
        self.alpha = alpha