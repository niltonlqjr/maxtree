#Flags on comand line:
#	OPT flag to describe the optimization flags (e.g make OPT=-O3)
#	TYPE flag to describe if you want to compile for (D)ebug or (R)elease, default is Debug (e.g. make TYPE=R)

maxtree_node_dir=../include
maxtree_dir=../include
utils_dir=../include
heap_dir=../include


vips_args = `pkg-config vips-cpp --cflags --libs`

ifeq (${TYPE},R)
	OBJ_CPP_FLAGS=-c 
	EXE_CPP_FLAGS=-pthread
else 
	OBJ_CPP_FLAGS=-g -c
	EXE_CPP_FLAGS=-g -pthread
endif

divide_name=divide
source_divide=${divide_name}.cpp
dest_divide = exec/${divide_name}

all: divide

divide: maxtree_node.o maxtree.o utils.o
	mkdir -p exec
	g++ ${OPT} ${EXE_CPP_FLAGS} ${source_divide} ${utils_dir}/utils.o \
	 	${maxtree_node_dir}/maxtree_node.o \
		${maxtree_dir}/maxtree.o \
		-I${maxtree_node_dir} -I${utils_dir} -I${maxtree_dir} -I${heap_dir} \
		${vips_args} -o ${dest_divide}

maxtree.o:
	g++ ${OBJ_CPP_FLAGS} ${maxtree_dir}/maxtree.cpp -I${maxtree_dir} ${vips_args} -o ${maxtree_dir}/maxtree.o

maxtree_node.o: #${maxtree_node_dir}/maxtree_node.cpp ${maxtree_node_dir}/maxtree_node.hpp 
	g++ ${OBJ_CPP_FLAGS} ${maxtree_node_dir}/maxtree_node.cpp -I${maxtree_node_dir} -o ${maxtree_node_dir}/maxtree_node.o

utils.o:
	g++ ${OBJ_CPP_FLAGS} ${utils_dir}/utils.cpp ${vips_args} -I${utils_dir} -o ${utils_dir}/utils.o

#heap.o:
#	g++ ${OBJ_CPP_FLAGS} ${heap_dir}/heap.cpp -I${heap_dir} -o ${heap_dir}/heap.o