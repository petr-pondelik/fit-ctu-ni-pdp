#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <bits/stdc++.h>
#include <limits>
#include <omp.h>
#include <chrono>

using namespace std;

unsigned long long ITERATION = 0;

void solve() {
    ITERATION++;
    /** Pokud řešení nemůže být lepší než stávající nejlepší, nebo než lepší či rovno horní mezi, ukončí větev */
    if ( ITERATION > 10000 ) {
        return;
    }
    # pragma omp task
    solve();
    # pragma omp task
    solve();
}

int main(int argc, char *argv[]) {

    chrono::steady_clock::time_point _start(chrono::steady_clock::now());
    # pragma omp parallel
    {
        /** Rekurentní nalezení nejlepší posloupnosti tahů */
        # pragma omp single
        solve();
    }
    chrono::steady_clock::time_point _end(chrono::steady_clock::now());

    cout << "Iterations: " << ITERATION << endl;
    cout << "Time: " << chrono::duration_cast<chrono::duration<double>>(_end - _start).count() << endl;
}