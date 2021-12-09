#ifndef MTREE_H_
#define MTREE_H_

#include <iterator>
#include <limits>
#include <map>
#include <queue>
#include <utility>
#include <iostream>
#include "functions.h"

namespace mt {

/**
 * La clase principal que implementa M-Tree.
 *
 * Data: El tipo de datos que indexará el M-Tree. Este tipo debe 
 * ser un tipo asignable y se debe definir un orden débil estricto
 * por less<Data>.
 * DistanceFunction: El tipo de función que se utilizará para 
 * calcular la distancia entre dos objetos 
 * ::mt::functions::euclidean_distance.
 * SplitFunction: El tipo de función que se utilizará para 
 * dividir un nodo cuando está en su capacidad máxima y un nuevo 
 * hijo debe ser add y default, es una composición de
 * ::mt::functions::random_promotion and
 * ::mt::functions::balanced_partition.
 *
 */
template <
	typename Data,
	typename DistanceFunction = ::mt::functions::euclidean_distance,
	typename SplitFunction = ::mt::functions::split_function<
	        ::mt::functions::random_promotion,
	        ::mt::functions::balanced_partition
		>
>
class mtree {
public:
	typedef DistanceFunction distance_function_type;
	typedef SplitFunction    split_function_type;
	typedef functions::cached_distance_function<Data, DistanceFunction> cached_distance_function_type;

private:
	class Node;
	class Entry;

	// Clases de excepción
	class SplitNodeReplacement {
	public:
		enum { NUM_NODES = 2 };
		Node* newNodes[NUM_NODES];
		SplitNodeReplacement(Node* newNodes[NUM_NODES]) {
			for(int i = 0; i < NUM_NODES; ++i) {
				this->newNodes[i] = newNodes[i];
			}

		}
	};

	class RootNodeReplacement {
	public:
		Node* newRoot;
	};

	class NodeUnderCapacity { };

	class DataNotFound {
	public:
		Data data;
	};


public:

	/**
	 * Una clase similar a un contenedor que se puede iterar para 
   * obtener los resultados de una consulta de vecinos más 
   * cercanos.
	 * Los vecinos se presentan en orden no decreciente a partir del
	 * argumento query_data a la llamada mtree::get_nearest().
	 *
	 * La consulta en el M-Tree se ejecuta durante la iteración, a 
   * medida que se obtienen los resultados. Significa que, para 
   * el momento en que se obtenga el resultado n-ésimo el momento 
   * en que el n-ésimo, es posible que aún no se conozca el 
   * siguiente resultado y que los recursos asignados sean solo 
   * los necesarios para identificar los n primeros resultados.
	 *
	 * Los objetos en el contenedor son instancias mtree :: 
   * query_result, que contienen un objeto de datos y la 
   * distancia desde el objeto de datos de la consulta.
	 * Ver mtree::get_nearest()
	 */
	class query {
	public:

		/**
		 * El tipo de resultados de las consultas del vecino más 
     * cercano.
		 */
		class result_item {
		public:
			/** Un vecino más cercano */
			Data data;

			/** La distancia desde el vecino más cercano al parámetro 
       * del objeto de datos de consulta.
			*/
			double distance;

			/** Constructor predeterminado */
			result_item() = default;

			/** Copiar constructor */
			result_item(const result_item&) = default;

			/** Mover constructor */
			result_item(result_item&&) = default;

			~result_item() = default;

			/** Copiar tarea */
			result_item& operator=(const result_item&) = default;

			/** Mover asignación */
			result_item& operator=(result_item&& ri) {
				if(this != &ri) {
					data = std::move(ri.data);
					distance = ri.distance;
				}
				return *this;
			}
		};


		typedef result_item value_type;


		query() = delete;

		/**
		 * Copiar constructor.
		 */
		query(const query&) = default;

		/**
		 * Mover constructor.
		 */
		query(query&&) = default;

		query(const mtree* _mtree, const Data& data, double range, size_t limit)
			: _mtree(_mtree), data(data), range(range), limit(limit)
			{}


		/** Copiar tarea. */
		query& operator=(const query&) = default;

		/** Mover asignación. */
		query& operator=(query&& q) {
			if(this != &q) {
				this->_mtree = q._mtree;
				this->range = q.range;
				this->limit = q.limit;
				this->data = std::move(q.data);
			}
			return *this;
		}



		/**
		 * El iterador para acceder a los resultados de las consultas 
     * del vecino más cercano.
		 */
		class iterator {
		public:
			typedef std::input_iterator_tag iterator_category;
			typedef result_item             value_type;
			typedef signed long int         difference_type;
			typedef result_item*            pointer;
			typedef result_item&            reference;


			iterator() : isEnd(true) {}


			explicit iterator(const query* _query)
				: _query(_query),
				  isEnd(false),
				  yieldedCount(0)
			{
				if(_query->_mtree->root == NULL) {
					isEnd = true;
					return;
				}

				double distance = _query->_mtree->distance_function(_query->data, _query->_mtree->root->data);
				double minDistance = std::max(distance - _query->_mtree->root->radius, 0.0);

				pendingQueue.push({_query->_mtree->root, distance, minDistance});
				nextPendingMinDistance = minDistance;

				fetchNext();
			}


			/** Copiar constructor. */
			iterator(const iterator&) = default;

			/**  Mover constructor. */
			iterator(iterator&&) = default;

			~iterator() = default;

			/** Copiar tarea. */
			iterator& operator=(const iterator&) = default;

			/** Mover tarea. */
			iterator& operator=(iterator&& i) {
				if(this != &i) {
					this->_query = i._query;
					this->currentResultItem = std::move(i.currentResultItem);
					this->isEnd = i.isEnd;
					this->pendingQueue = std::move(i.pendingQueue);
					this->nextPendingMinDistance = i.nextPendingMinDistance;
					this->nearestQueue = std::move(i.nearestQueue);
					this->yieldedCount = i.yieldedCount;
				}
				return *this;
			}


			bool operator==(const iterator& ri) const {
				if(this->isEnd  &&  ri.isEnd) {
					return true;
				}

				if(this->isEnd  ||  ri.isEnd) {
					return false;
				}

				return  this->_query == ri._query
				    &&  this->yieldedCount == ri.yieldedCount;
			}

			bool operator!=(const iterator& ri) const {
				return ! this->operator==(ri);
			}


			/**
			 * Haga avanzar el iterador al siguiente resultado.
			 */
			
			// prefijo
			iterator& operator++() {
				fetchNext();
				return *this;
			}

			// posfijo
			iterator operator++(int) {
				iterator aCopy = *this;
				operator++();
				return aCopy;
			}
			
			/**
			 * Da acceso al resultado de la consulta actual.
			 * Una instancia de iterador siempre se refiere a la misma 
       * instancia de result_item; solo los campos de result_item 
       * se actualizan cuando cambia el iterador.
			 */
			
			const result_item& operator*() const {
				return currentResultItem;
			}

			const result_item* operator->() const {
				return &currentResultItem;
			}
			

		private:
			template <typename U>
			struct ItemWithDistances {
				const U* item;
				double distance;
				double minDistance;

				ItemWithDistances(const U* item, double distance, double minDistance)
					: item(item), distance(distance), minDistance(minDistance)
					{ }

				bool operator<(const ItemWithDistances& that) const {
					return (this->minDistance > that.minDistance);
				}

			};

			void fetchNext() {

				if(isEnd  ||  yieldedCount >= _query->limit) {
					isEnd = true;
					return;
				}

				 while(!pendingQueue.empty()  ||  !nearestQueue.empty()) {
					if(prepareNextNearest()) {
						return;
					}

					assert(!pendingQueue.empty());

					ItemWithDistances<Node> pending = pendingQueue.top();
					pendingQueue.pop();

					const Node* node = pending.item;

					for(typename Node::ChildrenMap::const_iterator i = node->children.begin(); i != node->children.end(); ++i) {
						IndexItem* child = i->second;
						if(std::abs(pending.distance - child->distanceToParent) - child->radius <= _query->range) {
							double childDistance = _query->_mtree->distance_function(_query->data, child->data);
							double childMinDistance = std::max(childDistance - child->radius, 0.0);
							if(childMinDistance <= _query->range) {
								Entry* entry = dynamic_cast<Entry*>(child);
								if(entry != NULL) {
									nearestQueue.push({entry, childDistance, childMinDistance});
								} else {
									Node* node = dynamic_cast<Node*>(child);
									assert(node != NULL);
									pendingQueue.push({node, childDistance, childMinDistance});
								}
							}
						}
					}

					if(pendingQueue.empty()) {
						nextPendingMinDistance = std::numeric_limits<double>::infinity();
					} else {
						nextPendingMinDistance = pendingQueue.top().minDistance;
					}

				}

				isEnd = true;
			}


			bool prepareNextNearest() {
				if(!nearestQueue.empty()) {
					ItemWithDistances<Entry> nextNearest = nearestQueue.top();
					if(nextNearest.distance <= nextPendingMinDistance) {
						nearestQueue.pop();
						currentResultItem.data = nextNearest.item->data;
						currentResultItem.distance = nextNearest.distance;
						++yieldedCount;
						return true;
					}
				}

				return false;
			}


			const query* _query;
			result_item currentResultItem;
			bool isEnd;
			std::priority_queue<ItemWithDistances<Node>> pendingQueue;
			double nextPendingMinDistance;
			std::priority_queue<ItemWithDistances<Entry>> nearestQueue;
			size_t yieldedCount;
		};


		/**
		 * Inicia la ejecución de la consulta y devuelve un interador 
     * que hace referencia al primer resultado.
		 */
		iterator begin() const {
			return iterator(this);
		}


		/**
		 * Devuelve un iterador que informa que no hay más resultados.
		 */
		iterator end() const {
			return {};
		}

	private:
		const mtree* _mtree;
		Data data;
		double range;
		size_t limit;
	};


	enum {
		/**
		 * La capacidad mínima predeterminada de los nodos en un 
     * M-Tree, cuando no se especifica en la llamada al 
     * constructor.
		 */
		DEFAULT_MIN_NODE_CAPACITY = 50
	};


	/**
	 * El constructor principal de un M-Tree.
	 *
	 * min_node_capacity:La capacidad mínima de los nodos de un 
   * M-Tree. Debe ser al menos 2.
	 * max_node_capacity: La capacidad máxima de los nodos de un 
   * M-Tree. Debe ser mayor que min_node_capacity. Si se pasa -1, 
   * se usa el valor 2 * min_node_capacity - 1.
	 * distance_function: Una instancia de DistanceFunction.
	 * split_function: Una instancia de SplitFunction.
	 *
	 */
	explicit mtree(
			size_t min_node_capacity = DEFAULT_MIN_NODE_CAPACITY,
			size_t max_node_capacity = -1,
			const DistanceFunction& distance_function = DistanceFunction(),
			const SplitFunction& split_function = SplitFunction()
		)
		: minNodeCapacity(min_node_capacity),
		  maxNodeCapacity(max_node_capacity),
		  root(NULL),
		  distance_function(distance_function),
		  split_function(split_function)
	{
		if(max_node_capacity == size_t(-1)) {
			this->maxNodeCapacity = 2 * min_node_capacity - 1;
		}
	}

	// ¡No puede copiar!
	mtree(const mtree&) = delete;

	// ... pero moverlo está bien.
	/** Mover constructor. */
	mtree(mtree&& that)
		: root(that.root),
		  maxNodeCapacity(that.maxNodeCapacity),
		  minNodeCapacity(that.minNodeCapacity),
		  distance_function(that.distance_function),
		  split_function(that.split_function)
	{
		that.root = NULL;
	}


	~mtree() {
		delete root;
	}

	// ¡No puede copiar!
	mtree& operator=(const mtree&) = delete;

	// ... pero moverlo está bien
	/** Mover tarea. */
	mtree& operator=(mtree&& that) {
		if(&that != this) {
			std::swap(this->root, that.root);
			this->minNodeCapacity = that.minNodeCapacity;
			this->maxNodeCapacity = that.maxNodeCapacity;
			this->distance_function = std::move(that.distance_function);
			this->split_function = std::move(that.split_function);
		}
		return *this;
	}


	/**
	 * Agrega e indexa un objeto de datos.
	 * No se debe agregar un objeto que ya está indexado. No hay 
   * validación y el comportamiento no está definido si se 
   * realiza.
	 * data: El objeto de datos que se indexará.
	 */
	void add(const Data& data) {
		if(root == NULL) {
			root = new RootLeafNode(data);
			root->addData(data, 0, this);
		} else {
			double distance = distance_function(data, root->data);
			try {
				root->addData(data, distance, this);
			} catch(SplitNodeReplacement& e) {
				Node* newRoot = new RootNode(root->data);
				delete root;
				root = newRoot;
				for(int i = 0; i < SplitNodeReplacement::NUM_NODES; ++i) {
					Node* newNode = e.newNodes[i];
					double distance = distance_function(root->data, newNode->data);
					root->addChild(newNode, distance, this);
				}
			}
		}
	}


	/**
	 * Elimina un objeto de datos del M-Tree.
	 * data: El objeto de datos que se eliminará.
	 * Retorna true si y solo si se encontró el objeto.
	 */
	bool remove(const Data& data) {
		if(root == NULL) {
			return false;
		}

		double distanceToRoot = distance_function(data, root->data);
		try {
			root->removeData(data, distanceToRoot, this);
		} catch(RootNodeReplacement& e) {
			delete root;
			root = e.newRoot;
		} catch(DataNotFound) {
			return false;
		}
		return true;
	}


	/**
	 * Realiza una consulta de vecinos más cercanos en el árbol M, 
   * restringido por la distancia.
	 * query_data: El objeto de datos de la consulta.
	 * range: La distancia máxima desde query_data a los vecinos buscados.
	 * Retorna un objeto de consulta.
	 */
	query get_nearest_by_range(const Data& query_data, double range) const {
		return get_nearest(query_data, range, std::numeric_limits<unsigned int>::max());
	}

	/**
	 * Realiza una consulta de vecinos más cercanos en el árbol M, 
   * limitado por el número de vecinos.
	 * query_data: El objeto de datos de consulta.
	 * limit: El número máximo de vecinos que se van a buscar.
	 * Retorna  un objeto de consulta. 
	 */
	query get_nearest_by_limit(const Data& query_data, size_t limit) const {
		return get_nearest(query_data, std::numeric_limits<double>::infinity(), limit);
	}

	/**
	 * Realiza una consulta de vecino más cercano en el M-Tree, 
   * limitado por la distancia y / o el número de vecinos.
	 * query_data: El objeto de datos de la consulta.
	 * range: La distancia máxima desde query_data a los vecinos 
   * buscados.
	 * limit: El número máximo de vecinos que se van a buscar.
	 * Retorna un objeto de consulta
	 */
	query get_nearest(const Data& query_data, double range, size_t limit) const {
		return {this, query_data, range, limit};
	}

	/**
	 * Realiza una consulta de vecino más cercano en M-Tree, sin 
   * restricciones.
	 * query_data: El objeto de datos de la consulta.
	 * Retorna un objeto de consulta.
	 */
	query get_nearest(const Data& query_data) const {
		return {
			this,
			query_data,
			std::numeric_limits<double>::infinity(),
			std::numeric_limits<unsigned int>::max()
		};
	}

protected:

	void _check() const {
#ifndef NDEBUG
			if(root != NULL) {
				root->_check(this);
			}
#endif
	}

private:

	typedef std::pair<Data, Data> PromotedPair;
	typedef std::set<Data> Partition;


	size_t minNodeCapacity;
	size_t maxNodeCapacity;
	Node* root;

protected:
	DistanceFunction distance_function;
	SplitFunction split_function;

public:
	class IndexItem {
	public:
		Data data;
		double radius;
		double distanceToParent;

		virtual ~IndexItem() { };

		IndexItem() = delete;
		IndexItem(const IndexItem&) = delete;
		IndexItem(IndexItem&&) = delete;
		IndexItem& operator=(const IndexItem&) = delete;
		IndexItem& operator=(IndexItem&&) = delete;

	protected:
		IndexItem(const Data& data)
			: data(data),
			  radius(0),
			  distanceToParent(-1)
			{ }

	public:
		virtual size_t _check(const mtree* mtree) const {
			_checkRadius();
			_checkDistanceToParent();
			return 1;
		}

	private:
		void _checkRadius() const {
			assert(radius >= 0);
		}

	protected:
		virtual void _checkDistanceToParent() const {
			assert(dynamic_cast<const RootNodeTrait*>(this) == NULL);
			assert(distanceToParent >= 0);
		}
	};


private:
	class Node : public IndexItem {
	public:
		virtual ~Node() {
			for(typename ChildrenMap::iterator i = children.begin(); i != children.end(); ++i) {
				IndexItem* child = i->second;
				delete child;
			}
		}

		void addData(const Data& data, double distance, const mtree* mtree)
    {
			doAddData(data, distance, mtree);
			checkMaxCapacity(mtree);
		}

#ifndef NDEBUG
		size_t _check(const mtree* mtree) const {
			IndexItem::_check(mtree);
			_checkMinCapacity(mtree);
			_checkMaxCapacity(mtree);

			bool   childHeightKnown = false;
			size_t childHeight;
			for(typename ChildrenMap::const_iterator i = children.begin(); i != children.end(); ++i) {
#ifndef NDEBUG
				const Data& data = i->first;
#endif
				IndexItem* child = i->second;

				assert(child->data == data);
				_checkChildClass(child);
				_checkChildMetrics(child, mtree);

				size_t height = child->_check(mtree);
				if(childHeightKnown) {
					//assert(childHeight == height);
				} else {
					childHeight = height;
					childHeightKnown = true;
				}
			}

			return childHeight + 1;
		}
#endif

		typedef std::map<Data, IndexItem*> ChildrenMap;

		ChildrenMap children;

	protected:
		Node(const Data& data) : IndexItem(data) { }

		Node() : IndexItem(*((Data*)(0))) { //assert(!"THIS SHOULD NEVER BE CALLED"); 
    };

		Node(const Node&) = delete;
		Node(Node&&) = delete;
		Node& operator=(const Node&) = delete;
		Node& operator=(Node&&) = delete;

		virtual void doAddData(const Data& data, double distance, const mtree* mtree) = 0;

		virtual void doRemoveData(const Data& data, double distance, const mtree* mtree); // throw (DataNotFound) = 0;

	public:
		void checkMaxCapacity(const mtree* mtree) //throw (SplitNodeReplacement) 
    {
			if(children.size() > mtree->maxNodeCapacity) {
				Partition firstPartition;
				for(typename ChildrenMap::iterator i = children.begin(); i != children.end(); ++i) {
					firstPartition.insert(i->first);
				}

				cached_distance_function_type cachedDistanceFunction(mtree->distance_function);

				Partition secondPartition;
				PromotedPair promoted = mtree->split_function(firstPartition, secondPartition, cachedDistanceFunction);

				Node* newNodes[2];
				for(int i = 0; i < 2; ++i) {
					Data& promotedData    = (i == 0) ? promoted.first : promoted.second;
					Partition& partition = (i == 0) ? firstPartition : secondPartition;

					Node* newNode = newSplitNodeReplacement(promotedData);
					for(typename Partition::iterator j = partition.begin(); j != partition.end(); ++j) {
						const Data& data = *j;
						IndexItem* child = children[data];
						children.erase(data);
						double distance = cachedDistanceFunction(promotedData, data);
						newNode->addChild(child, distance, mtree);
					}

					newNodes[i] = newNode;
				}
				assert(children.empty());

				throw SplitNodeReplacement(newNodes);
			}

		}

	protected:
		virtual Node* newSplitNodeReplacement(const Data&) const = 0;

	public:
		virtual void addChild(IndexItem* child, double distance, const mtree* mtree) = 0;

		virtual void removeData(const Data& data, double distance, const mtree* mtree) //throw (RootNodeReplacement, NodeUnderCapacity, DataNotFound) 
    {
			doRemoveData(data, distance, mtree);
			if(children.size() < getMinCapacity(mtree)) {
				throw NodeUnderCapacity();
			}
		}

		virtual size_t getMinCapacity(const mtree* mtree) const = 0;

	protected:
		void updateMetrics(IndexItem* child, double distance) {
			child->distanceToParent = distance;
			updateRadius(child);
		}

		void updateRadius(IndexItem* child) {
			this->radius = std::max(this->radius, child->distanceToParent + child->radius);
		}


		virtual void _checkMinCapacity(const mtree* mtree) const = 0;

	private:
		void _checkMaxCapacity(const mtree* mtree) const {
			assert(children.size() <= mtree->maxNodeCapacity);
		}

	protected:
		virtual void _checkChildClass(IndexItem* child) const = 0;

	private:
#ifndef NDEBUG
		void _checkChildMetrics(IndexItem* child, const mtree* mtree) const {
			double dist = mtree->distance_function(child->data, this->data);
			assert(child->distanceToParent == dist);
			double sum = child->distanceToParent + child->radius;
			assert(sum <= this->radius);
		}
#endif
	};


	class RootNodeTrait : public virtual Node {
		void _checkDistanceToParent() const {
			assert(this->distanceToParent == -1);
		}
	};


	class NonRootNodeTrait : public virtual Node {
		size_t getMinCapacity(const mtree* mtree) const {
			return mtree->minNodeCapacity;
		}

		void _checkMinCapacity(const mtree* mtree) const {
			assert(this->children.size() >= mtree->minNodeCapacity);
		}
	};


	class LeafNodeTrait : public virtual Node {
		void doAddData(const Data& data, double distance, const mtree* mtree) {
			Entry* entry = new Entry(data);
			assert(this->children.find(data) == this->children.end());
			this->children[data] = entry;
			assert(this->children.find(data) != this->children.end());
			updateMetrics(entry, distance);
		}

		void addChild(IndexItem* child, double distance, const mtree* mtree) {
			assert(this->children.find(child->data) == this->children.end());
			this->children[child->data] = child;
			assert(this->children.find(child->data) != this->children.end());
			updateMetrics(child, distance);
		}

		Node* newSplitNodeReplacement(const Data& data) const {
			return new LeafNode(data);
		}

		void doRemoveData(const Data& data, double distance, const mtree* mtree) //throw (DataNotFound) 
    {
			if(this->children.erase(data) == 0) {
				throw DataNotFound{data};
			}
		}


		void _checkChildClass(IndexItem* child) const {
			assert(dynamic_cast<Entry*>(child) != NULL);
		}
	};


	class NonLeafNodeTrait : public virtual Node {
		void doAddData(const Data& data, double distance, const mtree* mtree) {
			struct CandidateChild {
				Node* node;
				double distance;
				double metric;
			};

			CandidateChild minRadiusIncreaseNeeded = { NULL, -1.0, std::numeric_limits<double>::infinity() };
			CandidateChild nearestDistance         = { NULL, -1.0, std::numeric_limits<double>::infinity() };

			for(typename Node::ChildrenMap::iterator i = this->children.begin(); i != this->children.end(); ++i) {
				Node* child = dynamic_cast<Node*>(i->second);
				assert(child != NULL);
				double distance = mtree->distance_function(child->data, data);
				if(distance > child->radius) {
					double radiusIncrease = distance - child->radius;
					if(radiusIncrease < minRadiusIncreaseNeeded.metric) {
						minRadiusIncreaseNeeded = { child, distance, radiusIncrease };
					}
				} else {
					if(distance < nearestDistance.metric) {
						nearestDistance = { child, distance, distance };
					}
				}
			}

			CandidateChild chosen = (nearestDistance.node != NULL)
			                      ? nearestDistance
			                      : minRadiusIncreaseNeeded;

			Node* child = chosen.node;
			try {
				child->addData(data, chosen.distance, mtree);
				updateRadius(child);
			} catch(SplitNodeReplacement& e) {
				// Reemplazar al hijo actual con nuevos nodos
#ifndef NDEBUG
				size_t _ =
#endif
					this->children.erase(child->data);
				//assert(_ == 1);
				delete child;

				for(int i = 0; i < e.NUM_NODES; ++i) {
					Node* newChild = e.newNodes[i];
					double distance = mtree->distance_function(this->data, newChild->data);
					addChild(newChild, distance, mtree);
				}
			}
		}


		void addChild(IndexItem* newChild_, double distance, const mtree* mtree) {
			Node* newChild = dynamic_cast<Node*>(newChild_);
			assert(newChild != NULL);

			struct ChildWithDistance {
				Node* child;
				double distance;
			};

			std::vector<ChildWithDistance> newChildren;
			newChildren.push_back(ChildWithDistance{newChild, distance});

			while(!newChildren.empty()) {
				ChildWithDistance cwd = newChildren.back();
				newChildren.pop_back();

				newChild = cwd.child;
				distance = cwd.distance;
				typename Node::ChildrenMap::iterator i = this->children.find(newChild->data);
				if(i == this->children.end()) {
					this->children[newChild->data] = newChild;
					updateMetrics(newChild, distance);
				} else {
					Node* existingChild = dynamic_cast<Node*>(this->children[newChild->data]);
					assert(existingChild != NULL);
					assert(existingChild->data == newChild->data);

					// Transferir _children_ de newChild a existingChild
					for(typename Node::ChildrenMap::iterator i = newChild->children.begin(); i != newChild->children.end(); ++i) {
						IndexItem* grandchild = i->second;
						existingChild->addChild(grandchild, grandchild->distanceToParent, mtree);
					}
					newChild->children.clear();
					delete newChild;

					try {
						existingChild->checkMaxCapacity(mtree);
					} catch(SplitNodeReplacement& e) {
#ifndef NDEBUG
						size_t _ =
#endif
							this->children.erase(existingChild->data);
						//assert(_ == 1);
						delete existingChild;

						for(int i = 0; i < e.NUM_NODES; ++i) {
							Node* newNode = e.newNodes[i];
							double distance = mtree->distance_function(this->data, newNode->data);
							newChildren.push_back(ChildWithDistance{newNode, distance});
						}
					}
				}
			}
		}


		Node* newSplitNodeReplacement(const Data& data) const {
			return new InternalNode(data);
		}


		void doRemoveData(const Data& data, double distance, const mtree* mtree) //throw (DataNotFound) 
    {
			for(typename Node::ChildrenMap::iterator i = this->children.begin(); i != this->children.end(); ++i) {
				Node* child = dynamic_cast<Node*>(i->second);
				assert(child != NULL);
				if(abs(distance - child->distanceToParent) <= child->radius) {
					double distanceToChild = mtree->distance_function(data, child->data);
					if(distanceToChild <= child->radius) {
						try {
							child->removeData(data, distanceToChild, mtree);
							updateRadius(child);
							return;
						} catch(DataNotFound&) {
							// Si se lanzó DataNotFound, los datos no se encontraron en el child
						} catch(NodeUnderCapacity&) {
							Node* expandedChild = balanceChildren(child, mtree);
							updateRadius(expandedChild);
							return;
						}
					}
				}
			}

			throw DataNotFound{data};
		}


		Node* balanceChildren(Node* theChild, const mtree* mtree) {
			// Trata de encontrar anotherChild que puede donar un grand-child a theChild.

			Node* nearestDonor = NULL;
			double distanceNearestDonor = std::numeric_limits<double>::infinity();

			Node* nearestMergeCandidate = NULL;
			double distanceNearestMergeCandidate = std::numeric_limits<double>::infinity();

			for(typename Node::ChildrenMap::iterator i = this->children.begin(); i != this->children.end(); ++i) {
				Node* anotherChild = dynamic_cast<Node*>(i->second);
				assert(anotherChild != NULL);
				if(anotherChild == theChild) continue;

				double distance = mtree->distance_function(theChild->data, anotherChild->data);
				if(anotherChild->children.size() > anotherChild->getMinCapacity(mtree)) {
					if(distance < distanceNearestDonor) {
						distanceNearestDonor = distance;
						nearestDonor = anotherChild;
					}
				} else {
					if(distance < distanceNearestMergeCandidate) {
						distanceNearestMergeCandidate = distance;
						nearestMergeCandidate = anotherChild;
					}
				}
			}

			if(nearestDonor == NULL) {
				// Merge
				for(typename Node::ChildrenMap::iterator i = theChild->children.begin(); i != theChild->children.end(); ++i) {
					IndexItem* grandchild = i->second;
					double distance = mtree->distance_function(grandchild->data, nearestMergeCandidate->data);
					nearestMergeCandidate->addChild(grandchild, distance, mtree);
				}

				theChild->children.clear();
				this->children.erase(theChild->data);
				delete theChild;
				return nearestMergeCandidate;
			} else {
				// Donar
				// Busca el grandchild más cercano
				IndexItem* nearestGrandchild;
				double nearestGrandchildDistance = std::numeric_limits<double>::infinity();
				for(typename Node::ChildrenMap::iterator i = nearestDonor->children.begin(); i != nearestDonor->children.end(); ++i) {
					IndexItem* grandchild = i->second;
					double distance = mtree->distance_function(grandchild->data, theChild->data);
					if(distance < nearestGrandchildDistance) {
						nearestGrandchildDistance = distance;
						nearestGrandchild = grandchild;
					}
				}

#ifndef NDEBUG
				size_t _ =
#endif
					nearestDonor->children.erase(nearestGrandchild->data);
				//assert(_ == 1);
				theChild->addChild(nearestGrandchild, nearestGrandchildDistance, mtree);
				return theChild;
			}
		}


		void _checkChildClass(IndexItem* child) const {
			assert(dynamic_cast<InternalNode*>(child) != NULL
			   ||  dynamic_cast<LeafNode*>(child)     != NULL);
		}
	};


	class RootLeafNode : public RootNodeTrait, public LeafNodeTrait {
	public:
		RootLeafNode(const Data& data) : Node(data) { }

		void removeData(const Data& data, double distance, const mtree* mtree) //throw (RootNodeReplacement, DataNotFound)
     {
			try {
				Node::removeData(data, distance, mtree);
			} catch (NodeUnderCapacity&) {
				assert(this->children.empty());
				throw RootNodeReplacement{NULL};
			}
		}

		size_t getMinCapacity(const mtree* mtree) const {
			return 1;
		}

		void _checkMinCapacity(const mtree* mtree) const {
			assert(this->children.size() >= 1);
		}
	};

	class RootNode : public RootNodeTrait, public NonLeafNodeTrait {
	public:
		RootNode(const Data& data) : Node(data) {}

	private:
		void removeData(const Data& data, double distance, const mtree* mtree) //throw (RootNodeReplacement, NodeUnderCapacity, DataNotFound) 
    {
			try {
				Node::removeData(data, distance, mtree);
			} catch(NodeUnderCapacity&) {
				// Promover al hijo único a enraizar
				Node* theChild = dynamic_cast<Node*>(this->children.begin()->second);
				Node* newRoot;
				if(dynamic_cast<InternalNode*>(theChild) != NULL) {
					newRoot = new RootNode(theChild->data);
				} else {
					assert(dynamic_cast<LeafNode*>(theChild) != NULL);
					newRoot = new RootLeafNode(theChild->data);
				}

				for(typename Node::ChildrenMap::iterator i = theChild->children.begin(); i != theChild->children.end(); ++i) {
					IndexItem* grandchild = i->second;
					double distance = mtree->distance_function(newRoot->data, grandchild->data);
					newRoot->addChild(grandchild, distance, mtree);
				}
				theChild->children.clear();

				throw RootNodeReplacement{newRoot};
			}
		}


		size_t getMinCapacity(const mtree* mtree) const {
			return 2;
		}

		void _checkMinCapacity(const mtree* mtree) const {
			assert(this->children.size() >= 2);
		}
	};


	class InternalNode : public NonRootNodeTrait, public NonLeafNodeTrait {
	public:
		InternalNode(const Data& data) : Node(data) { }
	};


	class LeafNode : public NonRootNodeTrait, public LeafNodeTrait {
	public:
		LeafNode(const Data& data) : Node(data) { }
	};


	class Entry : public IndexItem {
	public:
		Entry(const Data& data) : IndexItem(data) { }
	};
};

}

#endif /* MTREE_H_ */