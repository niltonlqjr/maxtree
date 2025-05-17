#Flags on comand line:
#	OPT flag to describe the optimization flags (e.g make OPT=-O3)
#	TYPE flag to describe if you want to compile for (D)ebug or (R)elease, default is Debug (e.g. make TYPE=R)

non_rec_wilk_name=non_recursive_wilk
non_rec_name=non_recursive
rec_name=recursive

ifeq (${TYPE},R)
	OBJ_CPP_FLAGS=-c 
else 
	OBJ_CPP_FLAGS=-g -c
endif


test_files=../../testes/fig.png ../../testes/example.png ../../testes/high_border.png


maxtree_node_dir=../../include
utils_dir=../../include
maxtree_dir=../../include

source_non_rec_wilk= ${non_rec_wilk_name}.cpp
source_non_rec = ${non_rec_name}.cpp
source_rec= ${rec_name}.cpp

vips_args = `pkg-config vips-cpp --cflags --libs`

MKDIR=@mkdir
RM=rm -rf


dest_nonrec_wilk = exec/${non_rec_wilk_name}
dest_nonrec = exec/${non_rec_name}
dest_rec = exec/${rec_name}

all: non_rec rec wilkinson

non_rec: maxtree_node.o utils.o maxtree.o
	${MKDIR} -p exec
	g++ ${OPT} ${source_non_rec} ${utils_dir}/utils.o ${maxtree_node_dir}/maxtree_node.o ${maxtree_dir}/maxtree.o \
        -I${maxtree_node_dir}  -I${maxtree_dir} -I${utils_dir}  ${vips_args} -o ${dest_nonrec}

rec: maxtree_node.o utils.o maxtree.o
	${MKDIR} -p exec
	g++ ${OPT} ${source_rec} ${utils_dir}/utils.o ${maxtree_node_dir}/maxtree_node.o ${maxtree_dir}/maxtree.o \
        -I${maxtree_node_dir}  -I${maxtree_dir} -I${utils_dir} ${vips_args} -o ${dest_rec}

wilkinson: maxtree_node.o utils.o maxtree.o
	${MKDIR} -p exec
	g++ ${OPT} ${source_non_rec_wilk} ${utils_dir}/utils.o ${maxtree_node_dir}/maxtree_node.o ${maxtree_dir}/maxtree.o \
    -I${maxtree_node_dir}  -I${maxtree_dir} -I${utils_dir} -I${vips_args} -o ${dest_nonrec_wilk}

maxtree_node.o: #${maxtree_node_dir}/maxtree_node.cpp ${maxtree_node_dir}/maxtree_node.hpp 
	g++ ${OBJ_CPP_FLAGS} ${maxtree_node_dir}/maxtree_node.cpp -I${maxtree_node_dir} -o ${maxtree_node_dir}/maxtree_node.o

maxtree.o:
	g++ ${OBJ_CPP_FLAGS} ${maxtree_dir}/maxtree.cpp -I${maxtree_dir} ${vips_args} -o ${maxtree_dir}/maxtree.o

utils.o:
	g++ ${OBJ_CPP_FLAGS} ${utils_dir}/utils.cpp ${vips_args} -I${utils_dir} -o ${utils_dir}/utils.o


clean:
	${RM} exec/${non_rec_wilk_name} exec/${non_rec_name} exec/${rec_name} ${utils_dir}/*.o ${maxtree_node_dir}/*.o

tests: all
	for input_test in ${test_files} ; do \
		./${dest_nonrec_wilk} $$input_test ; \
		./${dest_nonrec} $$input_test ; \
		./${dest_rec} $$input_test ; \
	done