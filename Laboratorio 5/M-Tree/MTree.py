import random
import heapq

# Infinito positivo y negativo. En Python 3.5 puede math.inf and -math.inf
PINF =  float('inf')
NINF = -float('inf')

# clase de nodo que compone el MTree
class Node(object):
    
    # toma una clave que es una lista de puntos, cada valor 
    # en el punto corresponde a otra dimensión
    # por ejemplo: key = [(1,2,3,4), (5,3,2,1), (9,0,8,7)]
    def __init__(self, pivot = None, radius = None):
        
        self.pivot = pivot      # tupla de clave y datos que divide pts en
                                # hijos izquierdos y derechos
        self.radius = radius    # la dist desde el pivote hasta el punto más lejanoin ball
        
        self.leftChild = None   # nodo con valores menores que el valor pivote
        self.rightChild = None  # nodo con valores mayores que los valores pivote
        
    
    def getPivotKey(self): return self.pivot[0]
    def getPivotData(self): return self.pivot[1]
    
    def __str__(self): return str(self.pivot)
    

class MTree(object):
    
    # constructor, toma una lista de pares de datos clave y construye un árbol de nodos
    # las claves son puntos con numDim número de dimensiones
    def __init__(self, kd, numDim):
        
        # el usuario no debe ingresar el número de dimensiones <= 0
        if(numDim <= 0): raise Exception("ERROR number of dimensions must be > 0")
        
        # el usuario debe ingresar un int como numDim
        if not isinstance(numDim, int): raise Exception("ERROR number of dimensions must be an int")
        
        self.__root = Node()
        self.__numDim = numDim
        
        # como parte del constructor, construya el MTree
        # comenzando en el nodo raíz
        self.__construct(self.__root, kd)
    
    # si se encuentra pt, devuelve data; de lo contrario, devuelve None
    def find(self, ptToFind):
        
        # si la distancia del pt al pivote de la raíz es mayor
        # que la dist desde el pivote hasta el punto más lejano en el Mtree (radio)
        # entonces el pt no existe en el árbol de bolas, así que devuelve falso
        if(self.__distance(ptToFind, self.__root.getPivotKey()) > self.__root.radius):
            return None           
        
        # de lo contrario, continúe buscando el pt dentro del árbol, comenzando en la raíz
        return self.__find(ptToFind, self.__root)
    
    # invocado por find      
    def __find(self, ptToFind, curNode):
        
        # si se encuentra una coincidencia exacta, devuelve data
        if(curNode.getPivotKey() == ptToFind): return curNode.getPivotData()
        
        # si ambos hijos existen y nuestro punto podría estar en cualquiera de ellos,
        # recurrir a ambos para ver dónde está, si es que está allí
        if(curNode.rightChild and self.__ptInBall(ptToFind, curNode.rightChild) and\
           curNode.leftChild and self.__ptInBall(ptToFind, curNode.leftChild)):
                
                leftFind = self.__find(ptToFind, curNode.leftChild)
                rightFind = self.__find(ptToFind, curNode.rightChild)
                
                # si se encuentra pt en cualquiera de los niños, devuelva los datos
                if leftFind: return leftFind
                elif rightFind: return rightFind
                
        # Si la pt solo podría estar en el hijo izquierdo, se usa recursividad  
        elif(curNode.leftChild and self.__ptInBall(ptToFind, curNode.leftChild)):
            leftFind = self.__find(ptToFind, curNode.leftChild)
            # si se encuentra, devuelve datos
            if leftFind: return leftFind
            
        # si el pt solo pudiera estar en el hijo derecho, se usa recursividad
        elif(curNode.rightChild and self.__ptInBall(ptToFind, curNode.rightChild)):
            # si se encuentra, devuelve datos
            rightFind = self.__find(ptToFind, curNode.rightChild)
            if rightFind: return rightFind
             
        # si hemos llegado a este punto, hemos buscado completamente en el árbol
        # y no he encontrado una coincidencia, así que devuelva None
        return None        
    
    # invocado por _find para comprobar si un punto podría estar dentro de un niño
    def __ptInBall(self, pt, node):
        
        
        # si dist <= radio: return True bc pt posiblemente podría encontrarse
        # en esa bola
        # si dist > radius: return False bc pt no se pudo encontrar
        # en esa bola
        return self.__distance(node.getPivotKey(), pt) <= node.radius
    
    # devuelve verdadero si el nodo hoja, falso si no
    def __isLeaf(self, cur): return not (cur.rightChild or cur.leftChild)        
    
    # ingrese un pt y cuántos vecinos (N) desea encontrar y devuelve
    # una lista de N vecinos más cercanos a pt
    def knnFind(self, pt, N):
        
        # crea un heap con N elementos
        # elemento: (distToPt, Key, Data)
        # nota: usaremos heapq así que invertimos los valores para tener la implementación de un heap máximo

        # los primeros n elementos tienen un dist imposible de NINF que
        # lo reduciremos más tarde
        # cada elemento = tupla de (invertedDist, Key, Data)
        heap = [(NINF, None, None)]*N
        
        # heap es una lista y es mutible, así que mutelo
        # para incluir los N vecinos más cercanos
        # nota: dado que el montón es mutible, no es necesario reasignar la var
        self.__knnFind(pt, self.__root, heap)
        
        # si no hubiera suficientes datos para dar n vecinos más cercanos,
        # reemplazar lugares 'vacíos' con str 'datos insuficientes'
        for i in range(len(heap)):
            
            if heap[i][0] == NINF:
                heap[i] = "insufficient data"
            else:
                heap[i] = (-(heap[i][0]),) + heap[i][1:]
        
        # devolver el montón actualizado
        return heap
    

    def __knnFind(self, pt, curNode, heap):

        # dist invertida
        dist = -self.__distance(curNode.getPivotKey(), pt)
        
        # la peor distancia está en la posición 0 del montón desde
        # heap máximo invertido
        worstDist = heap[0][0]
        
        # si esta dist de nuestro pt al nodo actual es
        # mejor que la peor distancia en el heap, actualice el heap
        # dist! = 0 asegura que estamos ignorando el punto que nos dieron
        if dist > worstDist and dist != 0:
            heapq.heappushpop(heap, (dist, curNode.getPivotKey(), curNode.getPivotData()))
        
    
        # si existe un hijo adecuado y los círculos formados por el
        # WorstDist de nuestro pt y la distancia de nuestro pt al
        # superposición de pivote, recursividad
        if(curNode.rightChild and self.__circlesIntersect(pt, curNode.rightChild, worstDist)):

            # nota: dado que el heap es mutable, no necesitamos reasignar la var
            self.__knnFind(pt, curNode.rightChild, heap)
                
        # si existe un hijo adecuado y los círculos formados por el
        # WorstDist de nuestro pt y la distancia de nuestro pt al
        # superposición de pivote, recursividad          
        if(curNode.leftChild and self.__circlesIntersect(pt, curNode.leftChild, worstDist)):
            
            # nota: dado que el heap es mutable, no necesitamos reasignar la var
            self.__knnFind(pt, curNode.leftChild, heap) 
        
        # si llegamos a este punto, hemos recurrido a un nodo hoja
        # así que devuelve el heap
        return heap
        
    # los círculos se cruzan cuando la distancia entre pivotes <= suma de radios
    # invocado por la función knn
    def __circlesIntersect(self, pt, cur, worstDist):
        
        distBtwnPiv = self.__distance(pt, cur.getPivotKey())
        # usa neg WorstDist para deshacerlo
        # nota: dist se invirtió para permitir una estructura de montón máxima
        sumRadii = -worstDist + cur.radius                    
        
        return distBtwnPiv <= sumRadii
    
    # devuelve kd sin dupKeys y deja que el cliente
    # saber si no se agregó un valor bc de la clave dup
    def __noDupKeys(self, kd):
        
        keys = []
        noDup = []
        for i in range(len(kd)):
            k = kd[i][0]
            # solo agregue el valor kd si k no es dup
            if k not in keys:
                noDup+=[kd[i]]
                keys+=[k]
            else:
                print("duplicate value: ", kd, " not added")
        
        return noDup
    
    # Construye el árbol, invocado por el constructor.
    def __construct(self, cur, kd):  
        
        numPts = len(kd)
        
        # si es la primera iteración, verifique kd para claves dup
        # y restablecerlo a ls sin teclas dup
        if cur == self.__root: 
            # si se insertó un kd vacío, lanzar excepción
            if numPts == 0: raise Exception("ERROR empty list inserted")
            kd = self.__noDupKeys(kd) 
      
        # obtener la dimensión con la extensión máxima,
        # tenga en cuenta que las dimensiones comienzan desde 0
        dimGreatestSpread = self.__getDimGreatestSpread(kd)
        
        # obtener el valor de pivote
        # Aproximaremos la mediana y la usaremos como pivote
        numMedians = 5                               # realizaremos una mediana de 5 para obtener la mediana
        if(numPts < numMedians): numMedians = numPts
        medians = []                                 # para ser llenado con numMedian número de nodos
        
        # rellenar medianas con numMedians nodos aleatorios en kd
        for i in range(numMedians): medians+=[kd[random.randint(0, numPts-1)]]
        
        # Ordene la matriz de la mediana en la dimensión con la mayor dispersión
        self.__selectionSort(medians, dimGreatestSpread)
        
        # el nodo mediano es el nodo en el medio del
        # ahora ordenada la matriz de medianas, y ese nodo será nuestro
        # pivote
        pivot = medians[len(medians)//2]
        
        # split basado en pivote
        leftChildren = []
        rightChildren = []
        
        # dividir nodos basados ​​en pivote
        # revise cada punto de la lista y agregue
        # al clúster secundario izquierdo o derecho basado en pivote
        for keyDat in kd:
            
            # data es el primer elemento
            curKey = keyDat[0]
            pivKey = pivot[0]
            
            # si un pt no tiene el mismo número de dimensiones
            # según el número de dimensiones especificado por el usuario, arroje
            # una excepción
            if(len(curKey) != self.__numDim): raise Exception("ERROR point " + str(keyDat) + " has an incorrect number of dimensions")
            
            # omitir el punto de pivote
            if(keyDat != pivot):
            
                if(curKey[dimGreatestSpread] > pivKey[dimGreatestSpread]):
                    rightChildren += [keyDat]
                elif(curKey[dimGreatestSpread] <= pivKey[dimGreatestSpread]):
                    leftChildren += [keyDat]
        
        # en este punto, leftChildren y rightChildren son listas de puntos (tuplas) que incluyen el pt y los datos
        # divididos por el valor de pivote
        
        # establecer atributos de nodo
        cur.pivot = pivot
        cur.radius = self.__furthestRadius(pivot, kd)
        
        # si hay ptos secundarios izquierdo / derecho, cree los nodos izquierdo / derecho
        # y aplicar recursividad hacia abajo para construir
        
        # si el nodo hoja finaliza la recursividad devolviendo None
        if(leftChildren == [] and rightChildren == []): return
        
        # si hay hijos de derecha y no de izquierda
        # construye los niños adecuados
        elif(leftChildren == []):
            cur.rightChild = Node()
            self.__construct(cur.rightChild, rightChildren)
        
        # si quedan hijos y no hay hijos correctos
        # construye los hijos de la izquierda
        elif(rightChildren == []): 
            cur.leftChild = Node()
            self.__construct(cur.leftChild, leftChildren)  
        
        # si hay hijos tanto derecho como izquierdo, construya ambos
        else:
            cur.rightChild = Node()
            cur.leftChild = Node()
            self.__construct(cur.rightChild, rightChildren)
            self.__construct(cur.leftChild, leftChildren)
             
        
        # si esta fue la primera recursividad, establezca el primer nodo
        # para ser la raíz, el resto de los hijos están conectados a
        # la raíz
        if(cur == self.__root):
            self.__root = cur
            
    def __furthestRadius(self, pivot, kd):
        
        greatestR = 0 # Establecer el mayor radio al más bajo que podría ser
        
        # recorrer cada punto en el nodo
        for keyDat in kd:
            # la clave es el primer elemento
            key = keyDat[0]
            # pasa por cada dimensión
            for dim in range(self.__numDim):
                
                # obtener la distancia entre el punto actual y el pivote (este es el radio)
                # los datos son el primer elemento en pivote (por eso ingresamos pivote [0])
                dist = self.__distance(pivot[0], key)
                
                # si la distancia es mayor que el radio mayor
                # establece el radio más grande para que sea esta distancia
                if(dist > greatestR): greatestR = dist
                    
        return greatestR   
            
            
   # return dist euclidiana
   # dist_euclidiana = sqrt(sum from 1 to n of (qsubi - psubi)**2)
    def __distance(self, pt1, pt2):
       
        squareSums = 0.0
        
        # sume la diferencia al cuadrado de cada dim de
        # cada pt
        for dim in range(self.__numDim): squareSums += (pt1[dim] - pt2[dim])**2
        
        return squareSums**(1/2)
    
    # ordenar matriz de nodos por valor en dimensiones específicas 
    def __selectionSort(self, kd, dim):
        
        length = len(kd)
        
        # pasar por el orden de selección
        for outer in range(length-1):
            min = outer
            for inner in range(outer+1, length):
                
                # 0 es el índice de los datos
                dat = kd[inner][0][dim]
                minDat = kd[min][0][dim]
                
                if dat < minDat: min = inner
            
            # swap
            kd[outer], kd[min] = kd[min], kd[outer]
    
    # devolver la dimensión con la mayor difusión
    def __getDimGreatestSpread(self,kd):
        
        # dado que se supone que todos los nodos tienen la misma atenuación,
        # podemos obtener el número de dimensiones de
        # el primer nodo de la lista
        
        # crea una lista para almacenar los diferenciales de cada dim
        # para que podamos ver cuál es el mejor
        spreads = [0]*self.__numDim
        
        # encuentra la luz de mayor difusión
        # para cada dimensión
        for dim in range(self.__numDim):
            
            # empezar con los valores más extremos
            minVal = PINF
            maxVal = NINF
            
            # recorrer cada nodo en la lista ingresada
            for pt in kd:
                
                # la clave es el primer valor de la tupla
                key = pt[0]
                
                # si ese valor es menor que el mínimo anterior
                # valor, establézcalo en minVal
                if(key[dim] < minVal):
                    minVal = key[dim]
                
                # si ese valor es mayor que el anterior
                # valor máximo, configúrelo en maxVal
                if(key[dim] > maxVal):
                    maxVal = key[dim]
                    
            # establecer la extensión de esa dimensión en maxVal - minVal
            spreads[dim] = maxVal-minVal
        
        # comience con la propagación máxima siendo el primer valor
        dimGreatestSpread = 0
        
        # ver qué dimensión tiene la mayor difusión
        for i in range(1,self.__numDim):
            if(spreads[i] > spreads[dimGreatestSpread]):
                dimGreatestSpread = i
        
        return dimGreatestSpread
    
    # imprime el árbol en grupos de puntos          
    def pTree(self):
    
        self.__pTree(self.__root, "ROOT:  ", "")
            
    def __pTree(self, cur, kind, indent):
        
        print("\n" + indent + kind, end="")
        if cur:
            print(cur, end="")
            if cur.leftChild:
                self.__pTree(cur.leftChild, "LEFT:  ", indent + "    ")
            if cur.rightChild:
                self.__pTree(cur.rightChild, "RIGHT:  ", indent + "    ")   