#Flags on comand line:
#	OPT flag to describe the optimization flags (e.g make OPT=-O3)
#	TYPE flag to describe if you want to compile for (D)ebug or (R)elease, default is Debug (e.g. make TYPE=R)

quadratic=quadratic

ifeq (${TYPE},R)
	OBJ_CPP_FLAGS=-c 
else 
	OBJ_CPP_FLAGS=-g -c
endif


test_files=../../testes/fig.png ../../testes/example.png ../../testes/high_border.png


maxtree_node_dir=../../include
utils_dir=../../include
maxtree_dir=../../include

src_quadratic = ${quadratic}.cpp


vips_args = `pkg-config vips-cpp --cflags --libs`

MKDIR=@mkdir
RM=rm -rf


dest_quadratic = exec/${quadratic}


all: quadratic

quadratic: maxtree_node.o utils.o maxtree.o
	${MKDIR} -p exec
	g++ ${OPT} ${src_quadratic} ${utils_dir}/utils.o ${maxtree_node_dir}/maxtree_node.o ${maxtree_dir}/maxtree.o \
        -I${maxtree_node_dir}  -I${maxtree_dir} -I${utils_dir} ${vips_args} -o ${dest_quadratic}

maxtree.o:
	g++ ${OBJ_CPP_FLAGS} ${maxtree_dir}/maxtree.cpp -I${maxtree_dir} ${vips_args} -o ${maxtree_dir}/maxtree.o

maxtree_node.o: 
	g++ ${OBJ_CPP_FLAGS} ${maxtree_node_dir}/maxtree_node.cpp -I${maxtree_node_dir} -I${utils_dir} ${vips_args} -o ${maxtree_node_dir}/maxtree_node.o

utils.o:
	g++ ${OBJ_CPP_FLAGS} ${utils_dir}/utils.cpp ${vips_args} -I${utils_dir} -o ${utils_dir}/utils.o


clean:
	${RM} exec/${dest_quadratic} ${utils_dir}/*.o ${maxtree_node_dir}/*.o

tests: all
	for input_test in ${test_files} ; do \
		./${dest_quadratic} $$input_test ; \
	done