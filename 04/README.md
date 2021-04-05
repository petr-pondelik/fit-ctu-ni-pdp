# MPI

TODO

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
    scp 01/sequentional.cpp pondepe1@star.fit.cvut.cz:~/solution

### Naplánování úlohy na klastru star

    # Úloha na 1 nodu
    qrun 20c 1 pdp_serial ./sge_scripts/{script}.sh

    # Distribuovaná úloha na více nodech
    qrun 20c 3 pdp_long ./sge_scripts/{script}.sh
