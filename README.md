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
