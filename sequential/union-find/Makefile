
quadratic=quadratic


test_files=../../testes/fig.png ../../testes/example.png ../../testes/high_border.png

maxtree_node_dir=../../include
utils_dir=../../include

src_quadratic = ${quadratic}.cpp


vips_args = `pkg-config vips-cpp --cflags --libs`

MKDIR=@mkdir
RM=rm -rf


dest_quadratic = exec/${quadratic}


all: quadratic

quadratic: maxtree_node.o utils.o
	${MKDIR} -p exec
	g++ -g -Wall ${src_quadratic} ${utils_dir}/utils.o ${maxtree_node_dir}/maxtree_node.o \
        -I${maxtree_node_dir}  ${vips_args} -o ${dest_quadratic}


maxtree_node.o: #${maxtree_node_dir}/maxtree_node.cpp ${maxtree_node_dir}/maxtree_node.hpp 
	g++ -g -c ${maxtree_node_dir}/maxtree_node.cpp -I${maxtree_node_dir} -o ${maxtree_node_dir}/maxtree_node.o

utils.o:
	g++ -g -c ${utils_dir}/utils.cpp ${vips_args} -I${utils_dir} -o ${utils_dir}/utils.o


clean:
	${RM} exec/${dest_quadratic} ${utils_dir}/*.o ${maxtree_node_dir}/*.o

tests: all
	for input_test in ${test_files} ; do \
		./${dest_quadratic} $$input_test ; \
	done
