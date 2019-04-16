# MMAS for the U-TPP

This repository contains a source code of the MMAS for solving the
_uncapacitated_ Traveling Purchaser Problem (U-TPP) as described in:

Rafał Skinderowicz, Solving the Uncapacitated Traveling Purchaser Problem with
the MAX-MIN Ant System. ICCCI (2) 2018: 257-267

BibTeX entry:

    @inproceedings{DBLP:conf/iccci/Skinderowicz18,
    author    = {Rafal Skinderowicz},
    editor    = {Ngoc Thanh Nguyen and
                Elias Pimenidis and
                Zaheer Khan and
                Bogdan Trawinski},
    title     = {Solving the Uncapacitated Traveling Purchaser Problem with the {MAX-MIN}
                Ant System},
    booktitle = {Computational Collective Intelligence - 10th International Conference,
                {ICCCI} 2018, Bristol, UK, September 5-7, 2018. Proceedings, Part
                {II}},
    series    = {Lecture Notes in Computer Science},
    volume    = {11056},
    pages     = {257--267},
    publisher = {Springer},
    year      = {2018},
    url       = {https://doi.org/10.1007/978-3-319-98446-9\_24},
    doi       = {10.1007/978-3-319-98446-9\_24},
    timestamp = {Mon, 27 Aug 2018 12:22:13 +0200},
    biburl    = {https://dblp.org/rec/bib/conf/iccci/Skinderowicz18},
    bibsource = {dblp computer science bibliography, https://dblp.org}
    }

## How to compile the program

The source is written in C++ and should work with the GNU g++ supporting
`--std=c++14`

Build instructions are provided in `Makefile` file for the GNU make.
To compile the program simply change the current working directory
to the folder with the source code and run make:

    make

If everything goes OK, a single executable should be created:
`ants-tpp`

## Running

The program supports a number of parameters but to run the most basic version
you can enter:

    ./ants-tpp --instance=EEuclideo.350.150.1.tpp --timeout=5

where `--instance=EEuclideo.350.150.1.tpp` is an example path to the U-TPP
instance to solve and `--timeout=5` sets the max computation time.
Results are saved automatically to a file in JSON format.

The program also prints some logging information to the console. Example
output:

    2019-04-16 10:01:56.436 (   0.419s) [main thread     ]               main.cpp:86    WARN| New global best: 2272 (55.94%, 1457), iter: 0
    2019-04-16 10:01:56.455 (   0.438s) [main thread     ]               main.cpp:86    WARN| New global best: 2103 (44.34%, 1457), iter: 1
    2019-04-16 10:01:56.470 (   0.453s) [main thread     ]               main.cpp:86    WARN| New global best: 1954 (34.11%, 1457), iter: 2
    2019-04-16 10:01:56.501 (   0.484s) [main thread     ]               main.cpp:86    WARN| New global best: 1894 (29.99%, 1457), iter: 4
    2019-04-16 10:01:56.754 (   0.737s) [main thread     ]               main.cpp:86    WARN| New global best: 1872 (28.48%, 1457), iter: 21
    2019-04-16 10:01:57.007 (   0.990s) [main thread     ]               main.cpp:86    WARN| New global best: 1865 (28.00%, 1457), iter: 38
    2019-04-16 10:01:57.073 (   1.056s) [main thread     ]               main.cpp:86    WARN| New global best: 1846 (26.70%, 1457), iter: 43
    2019-04-16 10:01:57.142 (   1.125s) [main thread     ]               main.cpp:86    WARN| New global best: 1703 (16.88%, 1457), iter: 48
    2019-04-16 10:01:57.750 (   1.733s) [main thread     ]                aco.cpp:194   WARN| Branching factor: 4.201429
    2019-04-16 10:01:57.878 (   1.861s) [main thread     ]               main.cpp:86    WARN| New global best: 1648 (13.11%, 1457), iter: 110
    2019-04-16 10:01:58.105 (   2.088s) [main thread     ]               main.cpp:86    WARN| New global best: 1592 (9.27%, 1457), iter: 130
    2019-04-16 10:01:58.833 (   2.816s) [main thread     ]                aco.cpp:194   WARN| Branching factor: 3.482857
    2019-04-16 10:01:58.883 (   2.866s) [main thread     ]               main.cpp:86    WARN| New global best: 1461 (0.27%, 1457), iter: 200
    2019-04-16 10:01:59.508 (   3.491s) [main thread     ]               main.cpp:86    WARN| New global best: 1458 (0.07%, 1457), iter: 234
    2019-04-16 10:01:59.577 (   3.559s) [main thread     ]               main.cpp:86    WARN| New global best: 1455 (-0.14%, 1457), iter: 235
    2019-04-16 10:02:00.846 (   4.829s) [main thread     ]                aco.cpp:194   WARN| Branching factor: 1.064286
    2019-04-16 10:02:01.060 (   5.042s) [main thread     ]               main.cpp:100   WARN| Best route: 0 156 155 59 293 189 341 242 93 236 157 183 224 181 321 325 271 104 154
    2019-04-16 10:02:01.060 (   5.043s) [main thread     ]               main.cpp:321   WARN| Saving results to a file: ./results_EEuclideo.350.150.1_2019-4-16__10:2:1_23079.js
