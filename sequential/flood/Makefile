main_name=non_recursive
sources = ${main_name}.cpp
args = `pkg-config vips-cpp --cflags --libs`

dest = exec/${main_name}

all:
	g++ -g -Wall $(sources) $(args) -o $(dest)
