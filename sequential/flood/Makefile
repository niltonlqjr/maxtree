non_rec_name=non_recursive
rec_name=recursive

source_non_rec = ${non_rec_name}.cpp
source_rec= ${rec_name}.cpp

args = `pkg-config vips-cpp --cflags --libs`

dest_nonrec = exec/${non_rec_name}
dest_rec = exec/${rec_name}

all: non_rec rec

non_rec:
	g++ -g -Wall $(source_non_rec) $(args) -o $(dest_nonrec)

rec:
	g++ -g -Wall $(source_rec) $(args) -o $(dest_rec)	
