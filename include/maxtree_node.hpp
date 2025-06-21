#include <string>
#include <cinttypes>
#include <vector>
#ifndef __MAXTREE_NODE_HPP__
#define __MAXTREE_NODE_HPP__

enum maxtee_node_field {
    PARENT, LABEL, IDX, GVAL, LEVELROOT, ATTRIBUTE, GLOBAL_IDX
};

#define Tattribute uint64_t
#define Tattr_default 1

enum borders{
    LEFT_BORDER,
    TOP_BORDER, 
    RIGHT_BORDER, 
    BOTTOM_BORDER
};

static const std::vector<enum borders> TBordersVector({
    LEFT_BORDER,
    TOP_BORDER, 
    RIGHT_BORDER, 
    BOTTOM_BORDER
});

static const std::vector<std::string> NamesBordersVector({
    "LEFT_BORDER",
    "TOP_BORDER", 
    "RIGHT_BORDER", 
    "BOTTOM_BORDER"
});

enum merge_directions{
    MERGE_HORIZONTAL,
    MERGE_VERTICAL
};


class maxtree_node{
    public:
        int64_t parent;
        int64_t label;
        uint64_t idx;//local index
        uint64_t global_idx;
        double gval;
        bool visited;
        Tattribute attribute;

        maxtree_node(double g, uint64_t i, uint64_t global_idx = 0, Tattribute attr = Tattr_default);
        void compute_attribute(Tattribute);

        bool operator>(const maxtree_node &r);
        bool operator>=(const maxtree_node &r);
        bool operator<(const maxtree_node &r);
        bool operator<=(const maxtree_node &r);
        bool operator==(const maxtree_node &r);
        bool operator!=(const maxtree_node &r);
};

#endif




