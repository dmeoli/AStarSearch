# A* Search [![Build Status](https://app.travis-ci.com/dmeoli/AStarSearch.svg?branch=master)](https://app.travis-ci.com/dmeoli/AStarSearch)

This code is an implementation of the [A* search algorithm](https://en.wikipedia.com/wiki/A*_search_algorithm) developed during the Knowledge 
Engineering course @ [Department of Computer Science](http://www.uniba.it/ricerca/dipartimenti/informatica) @ [University of Bari "Aldo Moro"](http://www.uniba.it/) under the supervision of prof. [Floriana Esposito](http://lacam.di.uniba.it/people/FlorianaEsposito.html).

##  TODO

  - [ ] NQueens

After three years from its sequential development, I provide different 
parallelized versions of the algorithm described in [1] using different 
concurrent libraries, i.e.:

  - [ ] [C++ Standard Library](https://en.cppreference.com/w/cpp/thread)
  - [ ] [FastFlow](http://calvados.di.unipi.it/)
  - [ ] [OpenMP](https://www.openmp.org/)

in the context of Parallel
and Distributed Systems course @ [Department of Computer Science](https://www.di.unipi.it/en/)
@ [University of Pisa](https://www.unipi.it/index.php/english) under the
supervision of prof. [Marco Danelutto](http://calvados.di.unipi.it/paragroup/danelutto/)
and prof. [Massimo Torquati](http://calvados.di.unipi.it/paragroup/torquati/).

## Running the software

Getting the whole project and all the subprojects updated at the latest 
version can be done with:

```sh
git clone --recurse-submodules https://github.com/dmeoli/AStarSearch.git
git submodule foreach --recursive "git checkout develop || git checkout master"
git submodule foreach --recursive "git pull"
```

To build the source code and run the code you just type into the terminal:

```sh
$ make
```

or, using CMake:

```sh
$ mkdir build
$ cd build
$ cmake ..
$ make
```

and then:

```sh
$ ./FindPath
$ ./8Puzzle {134862705|281043765|281463075|567408321|etc.}
$ ./15Puzzle {10 2 7 1 14 0 8 4 13 5 15 12 9 11 3 6|etc.}
$ ./MinPathToBucharest {Arad|Bucharest|Craiova|Drobeta|Eforie|Fagaras|Giurgiu|Hirsova|Iasi|Lugoj|Mehadia|Neamt|Oradea|Pitesti|RimnicuVilcea|Sibiu|Timisoara|Urziceni|Vaslui|Zerind}
```

## License [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

This software is released under the MIT License. See the [LICENSE](LICENSE) file for details.

## References

[1] Alex Fukunaga, Adi Botea, Yuu Jinnai, and Akihiro Kishimoto, [*A Survey 
of Parallel A**](https://arxiv.org/abs/1708.05296).