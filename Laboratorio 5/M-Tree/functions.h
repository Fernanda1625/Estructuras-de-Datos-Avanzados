#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

#include <ext/algorithm>
#include <iterator>
#include <set>
#include <utility>
#include <vector>
#include <math.h>

namespace mt {
namespace functions {

/**
 * Un objeto de función de distancia que calcula la distancia euclidiana
 * entre dos objetos de datos que representan coordenadas.
 * Supone que los objetos de datos son secuencias de números del mismo
 * tamaño.
*/
struct euclidean_distance {
	/**
	 * El operador que realiza el cálculo.
	*/
	template <typename Sequence>
	double operator()(const Sequence& data1, const Sequence& data2) const {
		double distance = 0;
		for(auto i1 = data1.begin(), i2 = data2.begin(); i1 != data1.end()  &&  i2 != data2.end(); ++i1, ++i2) {
			double diff = *i1 - *i2;
			distance += diff * diff;
		}
		distance = sqrt(distance);
		return distance;
	}
};

/**
 * Un objeto de función de promoción que elige aleatoriamente dos objetos
 * de datos como promocionados.
*/
struct random_promotion {
/**
* El operador que realiza la promoción.
* Data: El tipo de objetos de datos.
* DistanceFunction: El tipo de función u objeto de función utilizado para
* calcular la distancia entre dos objetos de datos.
* Retorna un par con los objetos de datos promocionados.
*/
	template <typename Data, typename DistanceFunction>
	std::pair<Data, Data> operator()(const std::set<Data>& data_objects, DistanceFunction& distance_function) const {
		std::vector<Data> promoted;
		random_sample_n(data_objects.begin(), data_objects.end(), inserter(promoted, promoted.begin()), 2);
		assert(promoted.size() == 2);
		return {promoted[0], promoted[1]};
	}
};

/**
 * Un objeto de función de partición que distribuye equitativamente los
 * objetos de datos de acuerdo con sus distancias a los objetos de datos
 * promocionados.
 * El algoritmo es aproximadamente equivalente a esto:
 *
 *     data_objects := first_partition
 *     first_partition  := Empty
 *     second_partition := Empty
 *     Repita hasta que data_object esté vacío:
 *         X := El objeto en data_objects que es el más cercano a promoted.first
 *         Remover X de data_object
 *         Añadir X al first_partition
 *
 *         Y := El objeto en data_objects que es el más cercano a promoted.second
 *         Remover Y de data_object
 *         Añadir Y a second_partition
 *
 */
struct balanced_partition {
	/**
	 * El operador que realiza la partición.
	 * Data: El tipo de los objetos de datos.
	 * DistanceFunction: El tipo de función u objeto de función
   * utilizado para calcular la distancia entre dos datos
   *
	 * [in] promoted: Los objetos de datos promocionados.
	 * [in,out] first_partition : Inicialmente, es el conjunto que 
   * contiene todos los objetos que deben particionarse. Después 
   * de la partición, contiene los objetos relacionados con el 
   * primer objeto de datos promocionado, el cual es  
   * promoted.first.
	 * [out] second_partition : Inicialmente, es un conjunto
   * vacío. Después de la partición, contiene los objetos 
   * relacionados con el segundo objeto de datos promovido, que 
   * es promoted.second.
	 * [in] distance_function: La función de distancia o el 
   * objeto de función.
	 */
	template <typename Data, typename DistanceFunction>
	void operator()(const std::pair<Data, Data>& promoted,
	                std::set<Data>& first_partition,
	                std::set<Data>& second_partition,
	                DistanceFunction& distance_function
	            ) const
	{
		std::vector<Data> queue1(first_partition.begin(), first_partition.end());
		// Ordenar por distancia a los primeros datos promocionados
		std::sort(queue1.begin(), queue1.end(),
			[&](const Data& data1, const Data& data2) {
				double distance1 = distance_function(data1, promoted.first);
				double distance2 = distance_function(data2, promoted.first);
				return distance1 < distance2;
			}
		);

		std::vector<Data> queue2(first_partition.begin(), first_partition.end());
		// Ordenar por distancia al segundo dato promocionado
		std::sort(queue2.begin(), queue2.end(),
			[&](const Data& data1, const Data& data2) {
				double distance1 = distance_function(data1, promoted.second);
				double distance2 = distance_function(data2, promoted.second);
				return distance1 < distance2;
			}
		);

		first_partition.clear();

		typename std::vector<Data>::iterator i1 = queue1.begin();
		typename std::vector<Data>::iterator i2 = queue2.begin();

		while(i1 != queue1.end()  ||  i2 != queue2.end()) {
			while(i1 != queue1.end()) {
				Data& data = *i1;
				++i1;
				if(second_partition.find(data) == second_partition.end()) {
					first_partition.insert(data);
					break;
				}
			}

			while(i2 != queue2.end()) {
				Data& data = *i2;
				++i2;
				if(first_partition.find(data) == first_partition.end()) {
					second_partition.insert(data);
					break;
				}
			}
		}
	}
};

/**
 * Un objeto de función que define una función dividida 
 * componiendo una
 * función de promoción y función de partición.
 * PromotionFunction: El tipo de función u objeto de función que
 * implementa una función de promoción.
 * PartitionFunction: El tipo de función u objeto de función que
 * implementa una función de partición.
 */
template <typename PromotionFunction, typename PartitionFunction>
struct split_function {
	/** */
	typedef PromotionFunction promotion_function_type;

	/** */
	typedef PartitionFunction partition_function_type;

	PromotionFunction promotion_function;
	PartitionFunction partition_function;

	/** */
	explicit split_function(
			PromotionFunction promotion_function = PromotionFunction(),
			PartitionFunction partition_function = PartitionFunction()
		)
	: promotion_function(promotion_function),
	  partition_function(partition_function)
	{}

	/**
	 * El operador que realiza la división.
	 * Data: El tipo de objetos de datos.
	 * DistanceFunction: El tipo de función u objeto de función 
   * utilizado para calcular la distancia entre dos objetos
	 * [in,out] first_partition: Inicialmente, es el conjunto que 
   * contiene todos los objetos que deben particionarse. Después 
   * la partición, contiene los objetos relacionado con el primer 
   * objeto de datos promocionado.
	 * [out] second_partition: Inicialmente, es un conjunto vacío. 
   * Después de la partición, contiene los objetos relacionados 
   * con el segundo objeto de datos promocionado.
	 * [in] distance_function: La función de distancia o el objeto 
   * de función.
	 * Retorna un par con los objetos de datos promocionados.
	*/
	template <typename Data, typename DistanceFunction>
	std::pair<Data, Data> operator()(
				std::set<Data>& first_partition,
				std::set<Data>& second_partition,
				DistanceFunction& distance_function
			) const
	{
		std::pair<Data, Data> promoted = promotion_function(first_partition, distance_function);
		partition_function(promoted, first_partition, second_partition, distance_function);
		return promoted;
	}
};

template <typename Data, typename DistanceFunction>
class cached_distance_function {
public:
	explicit cached_distance_function(const DistanceFunction& distance_function)
		: distance_function(distance_function)
		{}

	double operator()(const Data& data1, const Data& data2) {
		typename CacheType::iterator i = cache.find(make_pair(data1, data2));
		if(i != cache.end()) {
			return i->second;
		}

		i = cache.find(make_pair(data2, data1));
		if(i != cache.end()) {
			return i->second;
		}

		// No encontrado en caché
		double distance = distance_function(data1, data2);

		// Almacenar en caché
		cache.insert(make_pair(make_pair(data1, data2), distance));
		cache.insert(make_pair(make_pair(data2, data1), distance));

		return distance;
	}

private:
	typedef std::map<std::pair<Data, Data>, double> CacheType;

	const DistanceFunction& distance_function;
	CacheType cache;
};

}
}

#endif /* FUNCTIONS_H_ */