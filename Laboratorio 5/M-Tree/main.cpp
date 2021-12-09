#include <iostream>
#include <sstream>
#include <fstream>
//#include "mtree.h"
//#include "functions.h"

using namespace std;

#define NOMBRE_ARCHIVO "tabla1.csv"

typedef mt::mtree<int, size_t(*)(int,int)> MTree;
class PointMTree : public MTree {
  public:
	  PointMTree (PointMTree&&);
    PointMTree(size_t minNodeCapacity = MTree::DEFAULT_MIN_NODE_CAPACITY)
		: MTree(minNodeCapacity, -1)
		{}
};

int main(){
    PointMTree mtree;

    ifstream archivo(NOMBRE_ARCHIVO);
    ofstream salida("salida.txt");
    string linea;
    char delimitador = ',';
    getline(archivo, linea);
    while (getline(archivo, linea)){

        stringstream stream(linea);
        string ordenInsercion, pais, x, y;
        getline(stream, ordenInsercion, delimitador);
        getline(stream, pais, delimitador);
        getline(stream, x, delimitador);
        getline(stream, y, delimitador);

        salida << pais << "=" << "(" << x << "," << y << ")" << endl;
    }

    archivo.close();
    return 0;
}