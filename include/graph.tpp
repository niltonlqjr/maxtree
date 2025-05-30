#include <vector>
template<typename T>
Edge::Edge(uint64_t u, uint64_t v){
    this->v1 = u;
    this->v2 = v;
}

template<typename T>
Edge::Edge(uint64_t u, uint64_t v, T w){
    this->v1 = u;
    this->v2 = v;
    this->w = w;
}


template<typename Tv, typename Ted>
Graph::Graph(){
    
}

template<typename Tv, typename Ted>
void Graph::add_vertex(Tv v){
    uint64_t pos;
    pos = this->V.len();
    map_V_vector[v] = pos;
    this->V.push_back(v);

}



#include "graph.hpp"