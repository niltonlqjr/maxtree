sources = quadratic.c
args = `pkg-config vips --cflags --libs`

dest = exec/quadratic

all:
	mkdir -p exec
	gcc $(sources) $(args) -o $(dest)