non_rec_name=non_recursive
rec_name=recursive

maxtree_node_dir=../../include
utils_dir=../../include

source_non_rec = ${non_rec_name}.cpp
source_rec= ${rec_name}.cpp

vips_args = `pkg-config vips-cpp --cflags --libs`

dest_nonrec = exec/${non_rec_name}
dest_rec = exec/${rec_name}

all: non_rec rec wilkinson

non_rec: maxtree_node.o utils.o
	g++ -g -Wall $(source_non_rec) ${utils_dir}/utils.o ${maxtree_node_dir}/maxtree_node.o \
        -I${maxtree_node_dir}  $(vips_args) -o $(dest_nonrec)

rec: maxtree_node.o utils.o
	g++ -g -Wall $(source_rec) ${utils_dir}/utils.o ${maxtree_node_dir}/maxtree_node.o \
        -I${maxtree_node_dir} $(vips_args) -o $(dest_rec)

wilkinson: maxtree_node.o utils.o
	g++ -g -Wall non_recursive_wilk.cpp ${utils_dir}/utils.o ${maxtree_node_dir}/maxtree_node.o \
    -I ${maxtree_node_dir} $(vips_args) -o exec/non_recursive_wilk

maxtree_node.o: #${maxtree_node_dir}/maxtree_node.cpp ${maxtree_node_dir}/maxtree_node.hpp 
	g++ -g -c ${maxtree_node_dir}/maxtree_node.cpp -I${maxtree_node_dir} -o ${maxtree_node_dir}/maxtree_node.o

utils.o:
	g++ -g -c ${utils_dir}/utils.cpp $(vips_args) -I${utils_dir} -o ${utils_dir}/utils.o