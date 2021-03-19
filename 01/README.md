# Sekvenční algoritmus (VAJ)

## Dostupné příkazy

### Kompilace

    make

### Vyčištění výstupního adresáře

    make clean

### Spuštění programu

    ./output/a.out < ./../data/vaj1.txt    

## Měření

    { time ./output/a.out < ./../data/vaj1.txt ; } >> ./results/measurement.txt 2>> ./results/measurement.txt
