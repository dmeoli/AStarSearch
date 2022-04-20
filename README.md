# A* Search [![Build Status](https://travis-ci.org/dmeoli/AStarSearch.svg?branch=master)](https://travis-ci.org/dmeoli/AStarSearch)

This code is an implementation of the [A* search algorithm](https://en.wikipedia.org/wiki/A*_search_algorithm) developed during the Knowledge Engineering course @ [Department of Computer Science](http://www.uniba.it/ricerca/dipartimenti/informatica) @ [University of Bari "Aldo Moro"](http://www.uniba.it/) under the supervision of prof. [Floriana Esposito](http://lacam.di.uniba.it/people/FlorianaEsposito.html).

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
$ ./FindPath.o
$ ./8Puzzle.o {134862705|281043765|281463075|567408321|etc.}
$ ./MinPathToBucharest.o {Arad|Bucharest|Craiova|Drobeta|Eforie|Fagaras|Giurgiu|Hirsova|Iasi|Lugoj|Mehadia|Neamt|Oradea|Pitesti|RimnicuVilcea|Sibiu|Timisoara|Urziceni|Vaslui|Zerind}
```

## License [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

This software is released under the MIT License. See the [LICENSE](LICENSE) file for details.
