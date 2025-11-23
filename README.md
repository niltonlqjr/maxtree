### Maxtree parallel implementations

#### Prerequisites
- libvips [libvips.org](https://www.libvips.org/)
- C++11 Compiler (tested on g++ 13.3.0)

#### Docker ready to use

- Docker on [github](https://github.com/niltonlqjr/docker-libvips)

#### Build

- Build all implementations (parallel and sequential)
```console
$ make
```

You can also build with your optimization set and remove -g

##### Build Release version
- Build with O2 and no debug flag
```console
$ make TYPE=R
```

##### Build with optimization

- Build with custom flags (e.g. -O3 optimization sequence)
```console
$ make OPT=-O3
```
If used with TYPE variable, this variable replaces -O2 optimization level

##### Build only parallel or sequential

- Build only parallel
```console
$ cd parallel
$ make
```

- Build only sequential
```console
$ cd sequential/flood
$ make
```

The variables TYPE and OPT can be used in those buildings.

#### Run

- To run you must pass the input image and the configuration file (in this order):

    - Run sequential 
        ```console
        $ cd sequential/flood
        $ ./exec/recursive ../../testes/dos_wp_bw.png ../../configs/sequential_rec.txt
        ```

    - Run sequential
        ```console
        $ cd parallel
        $ ./exec/merge ../testes/dos_wp_bw.png ../configs/parallel_8l8c.txt
        ```
