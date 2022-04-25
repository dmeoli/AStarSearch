# A* Search [![Build Status](https://app.travis-ci.com/dmeoli/AStarSearch.svg?branch=master)](https://app.travis-ci.com/dmeoli/AStarSearch)

This code is an implementation of the [A* search algorithm](https://en.wikipedia.com/wiki/A*_search_algorithm) developed during the Knowledge 
Engineering course @ [Department of Computer Science](http://www.uniba.it/ricerca/dipartimenti/informatica) @ [University of Bari "Aldo Moro"](http://www.uniba.it/) under the supervision of prof. [Floriana Esposito](http://lacam.di.uniba.it/people/FlorianaEsposito.html).

After three years from its sequential development, I provide different 
parallelized versions of the algorithm described in [1] using different 
concurrent libraries, i.e.:

  - [x] [C++ Standard Library](https://en.cppreference.com/w/cpp/thread)
  - [x] [FastFlow](http://calvados.di.unipi.it/)
  - [ ] [OpenMP](https://www.openmp.org/)
  - [ ] [Intel TBB (Threading Building Blocks)](https://www.intel.com/content/www/us/en/developer/tools/oneapi/onetbb.html)

in the context of Parallel
and Distributed Systems course @ [Department of Computer Science](https://www.di.unipi.it/en/)
@ [University of Pisa](https://www.unipi.it/index.php/english) under the
supervision of prof. [Marco Danelutto](http://calvados.di.unipi.it/paragroup/danelutto/)
and [Massimo Torquati](http://calvados.di.unipi.it/paragroup/torquati/).

## Running the software

To compile the source code and run the software you just type into the terminal:

```
$ make
```

or:

```
$ mkdir build
$ cd build
$ cmake ..
$ make
```

and then:

```
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