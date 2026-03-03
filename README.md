## Maxtree parallel implementations

### Prerequisites
- libvips: [libvips.org](https://www.libvips.org/)
- C++11 Compiler (tested on g++ 13.3.0)
- ZeroMQ: [zeromq.org](https://zeromq.org/)
    - [How to get](http://wiki.zeromq.org/intro:get-the-software)
- rapidyaml: [Git repository](https://github.com/biojppm/rapidyaml)

### Docker ready to use

- Docker on [github](https://github.com/niltonlqjr/docker-libvips)

### Build

- Build all implementations (parallel and sequential)
```console
$ make
```

You can also build with your optimization set and remove -g

#### Build Release version
- Build with O2 and no debug flag
```console
$ make TYPE=R
```

#### Build with optimization

- Build with custom flags (e.g. -O3 optimization sequence) and degug flag (-g)
```console
$ make OPT=-O3
```
- If used with TYPE variable, this variable replaces -O2 optimization level

- Build with custom flags and no debug flag
```console
$ make TYPE=R OPT=-O3
```

Ps: Just remember to clean if you want to rebuild all object files with new flag set

#### Build only parallel or sequential

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

### Run

- To run you must pass the the configuration file (optional arguments are input file and output prefix name):

    - Run parallel
        ```console
        $ cd parallel
        $ ./exec/merge example_config.txt
        ```
        ```console
        $ cd parallel
        $ ./exec/merge example_config.txt ../testes/dos_wp_bw.png 
        ```
        ```console
        $ cd parallel
        $ ././exec/merge example_config.txt ../testes/dos_wp_bw.png output_prefix

    - Run sequential 
        ```console
        $ cd sequential/flood
        $ ./exec/recursive ../../testes/dos_wp_bw.png  ../../configs/sequential_rec.txt
        ```

#### Configuration files

- Some configuration examples are available in [configs](configs/).
    - [worker configurations](configs/worker/)
    - [manager configurations](configs/manager/)