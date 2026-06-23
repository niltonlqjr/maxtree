maxtree_node_dir=../include
maxtree_dir=../include
boundary_tree_dir=../include
utils_dir=../include
heap_dir=../include
custom_dir=../include

tasks_dir=../parallel
message_dir=../parallel
workers_dir=../parallel

info_name = info

hps_dir=../hps
vips_args = `pkg-config vips-cpp --cflags --libs`

CPP_libs = -pthread -lryml

ifeq (${TYPE},R)
	OBJ_CPP_FLAGS=-c 
	EXE_CPP_FLAGS=
	OPT ?= -O2
else 
	OBJ_CPP_FLAGS=-g -c
	EXE_CPP_FLAGS=-g
endif

all: ${info_name}

${info_name}: base ${info_name}.o 
	g++ ${OPT} ${EXE_CPP_FLAGS} \
		-I${maxtree_node_dir} \
		-I${maxtree_dir} \
		-I${boundary_tree_dir} \
		-I${utils_dir} \
		${maxtree_node_dir}/maxtree_node.o \
		${maxtree_dir}/maxtree.o \
		${boundary_tree_dir}/boundary_tree.o \
		${utils_dir}/utils.o \
		${custom_dir}/custom.o \
		${CPP_libs} \
		${vips_args} \
		${info_name}.o -o ${info_name}

${info_name}.o: ${info_name}.cpp
	g++ ${OPT} ${OBJ_CPP_FLAGS} \
		-I${maxtree_node_dir} \
		-I${maxtree_dir} \
		-I${boundary_tree_dir} \
		-I${hps_dir} \
		${vips_args} \
		${info_name}.cpp -o ${info_name}.o

base: 
	export TYPE=${TYPE}
	export OPT=${OPT}
	@make -C ${maxtree_node_dir} 
	@make -C ${maxtree_dir} 
	@make -C ${boundary_tree_dir} 
	@make -C ${utils_dir} 
	@make -C ${heap_dir}
	@make -C ${custom_dir}
	@make -C ${tasks_dir} tasks.o
	@make -C ${message_dir} message.o
	@make -C ${workers_dir} workers.o

clean:
	@rm ${info_name}.o ${info_name}