# MPI

TODO

## Lokální práce

### Kompilace a spuštění MPI programu

    mpic++ -o ./output/mpi.out mpi.cpp -Wall -pedantic -std=c++11 -fopenmp -O3
    mpirun -np 3 ./output/mpi.out 2 1 3 < ./../data/vaj11.txt >> results/measurement.txt

## Klastr star.fit.cvut.cz

### Připojení pomocí SSH

    ssh pondepe1@fit.cvut.cz

### Adresářová struktura

    /
        bin/
            TODO
        data/
            {input data .txt}
        output/
            TODO
        sge_scripts/
            serial_job.sh
            parallel_job.sh
        solution/
            01/
                sequentional.cpp
                Makefile
            02/
                taskparallelism.cpp
                Makefile
            03/
                dataparallelism.cpp
                Makefile
            04/
                openmpi.cpp
                Makefile

### Upload souborů na star

Na klastr star lze uploadovat například pomocí `scp`.

    # Upload data directory
    scp data/* pondepe1@star.fit.cvut.cz:~/data

    # Upload solution source
    scp mpi.cpp pondepe1@star.fit.cvut.cz:~/solution/04

### Naplánování úlohy na klastru star

    # Úloha na 1 nodu
    qrun 20c 1 pdp_serial ./sge_scripts/{script}.sh

    # Distribuovaná úloha na více nodech
    qrun 20c 3 pdp_fast ./sge_scripts/{script}.sh

### Kompilace MPI programu

    mpic++ -o ~/output/mpi.out ~/solution/04/mpi.cpp -Wall -pedantic -std=c++11 -fopenmp -O3
