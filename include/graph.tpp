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


template<typename Tv, typename Te>
Graph(std::vector<Tv> v = std::vector<Tv>(), std::vector< Edge<Te> > = std::vector<Edge<Te> >()){
    this->V = v;
    this->E = E;
}




#include "graph.hpp"