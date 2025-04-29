divide_dir = ./parallel
flood_dir = ./sequential/flood
union_find_dir = ./sequential/union-find

all: divide flood union


divide:
	$(MAKE) -C ${divide_dir}

flood:
	$(MAKE) -C ${flood_dir}

union:
	$(MAKE) -C ${union_find_dir}