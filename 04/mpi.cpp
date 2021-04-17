#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <bits/stdc++.h>
#include <limits>
#include <omp.h>
#include <mpi.h>
#include <chrono>

using namespace std;

// ======================================= CONSTANTS ===================================================================

/** Size constants */
#define MAX_K 400
#define MAX_PATH 50

/** Custom MPI tags */
#define TAG_WORK 1
#define TAG_UPDATE 2
#define TAG_DONE 3
#define TAG_FINISHED 4

#define MASTER_PROCESS 0

// ======================================= GLOBAL VARIABLES ============================================================

unsigned short OPT_COST = numeric_limits<unsigned short>::max();
unsigned short THREAD_CNT;
unsigned long long ITERATION = 0;

// ======================================= MPI STATE STRUCTURE =========================================================

struct state_structure {
    char board[MAX_K];
    short rookPosition[2];
    short knightPosition[2];
    unsigned short k;
    unsigned short pawnsCnt;
    unsigned short lowerBound;
    unsigned short upperBound;
    char turn;
    unsigned short cost;
    short path[MAX_PATH];
    short nextMove[3];
};

//MPI_Datatype state_structure_type;
//
//int state_structure_lengths[4] = { MAX_K, 2, 2, 1, };
//
//const MPI_Aint mss_displacements[4] = { 0,
//                                        MAX_N * sizeof(bool),
//                                        MAX_N * sizeof(bool) + sizeof(int),
//                                        MAX_N * sizeof(bool) + sizeof(int) + sizeof(uint) };
//
//MPI_Datatype mss_types[4] = { MPI_C_BOOL, MPI_INT, MPI_UNSIGNED, MPI_DOUBLE };


// ======================================= APPLICATION LOGIC ===========================================================

/** Reprezentace šachovnice */
class ChessBoard {

public:
    /** Vector o velikosti k^2 mapovaný na 2D pole */
    vector<char> board;

    /** Aktuální pozice věže */
    pair<short, short> rookPosition;

    /** Aktuální pozice jezdce */
    pair<short, short> knightPosition;

    unsigned short k, pawnsCnt, lowerBound, upperBound;

    ChessBoard(void) {}

    /** Konstruktor zajistí konstrukci šachovice */
    ChessBoard(unsigned short &k, unsigned short &upperBound, string &boardStr) {
        this->k = k;
        this->pawnsCnt = 0;
        for (short i = 0; i < k; i++) {
            for (short j = 0; j < k; j++) {
                short content = boardStr[this->mapPosition(i, j)];
                if (content == 'V') {
                    this->rookPosition = make_pair(i, j);
                } else if (content == 'J') {
                    this->knightPosition = make_pair(i, j);
                } else if (content == 'P') {
                    this->pawnsCnt++;
                }
                this->board.emplace_back(content);
            }
        }
        this->lowerBound = this->pawnsCnt;
        this->upperBound = upperBound;
    };

    /** Construct state object from structure */
    ChessBoard(const state_structure s): k(s.k), pawnsCnt(s.pawnsCnt), lowerBound(s.lowerBound), upperBound(s.upperBound) {
        short kSq = s.k * s.k;
        vector<char> b(kSq);
        for (int i = 0; i < kSq; ++i) {
            b[i] = s.board[i];
        }
        this->board = b;
        this->rookPosition = make_pair(s.rookPosition[0], s.rookPosition[1]);
        this->knightPosition = make_pair(s.knightPosition[0], s.knightPosition[1]);
    }

    /** Vrací, zda souřadnice patří do šachovnice */
    bool fitsDimensions(short &x, short &y) {
        return x >= 0 && x < this->k && y >= 0 && y < this->k;
    }

    /** Vrací, zda je pole prázdné */
    bool isTileEmpty(short x, short y) {
        return this->getTile(x, y) == '-';
    }

    /** Vrací, zda se může věž posunout na zadanou souřadnici */
    bool canMoveRookTo(short x, short y) {
        if (!this->fitsDimensions(x, y)) {
            return false;
        }

        /** Zjištění velikosti posunu po osách x a y */
        short xDiff = x - this->rookPosition.first;
        short yDiff = y - this->rookPosition.second;

        /** Ověření, zda se na cestě k zadané souřadnici nenachází kámen -> pokud ano, věž ho nemůže přeskočit */
        if (xDiff > 0) {
            for (short i = 1; i < xDiff; ++i) {
                if (!this->isTileEmpty(this->rookPosition.first + i, this->rookPosition.second)) {
                    return false;
                }
            }
        } else if (xDiff < 0) {
            for (short i = xDiff + 1; i < 0; ++i) {
                if (!this->isTileEmpty(this->rookPosition.first + i, this->rookPosition.second)) {
                    return false;
                }
            }
        } else if (yDiff > 0) {
            for (short i = 1; i < yDiff; ++i) {
                if (!this->isTileEmpty(this->rookPosition.first, this->rookPosition.second + i)) {
                    return false;
                }
            }
        } else if (yDiff < 0) {
            for (short i = yDiff + 1; i < 0; ++i) {
                if (!this->isTileEmpty(this->rookPosition.first, this->rookPosition.second + i)) {
                    return false;
                }
            }
        }

        /** Věž může na prázdné pole, či na pole s pěšákem a sebrat ho */
        return (this->getTile(x, y) == '-') || (this->getTile(x, y) == 'P');
    }

    /** Vrací, zda se může jezdec posunout na zadanou souřadnici */
    bool canMoveKnightTo(short &x, short &y) {
        if (!this->fitsDimensions(x, y)) {
            return false;
        }
        /** Jezdec může na prázdné pole, či na pole s pěšákem a sebrat ho */
        return (this->getTile(x, y) == '-') || (this->getTile(x, y) == 'P');
    }

    short getSize() {
        return this->k * this->k;
    }

    /** Mapování souřadnic ve 2D poli do 1D pole */
    short mapPosition(short &x, short &y) {
        return x * this->k + y;
    }

    /** Vrací pole šachovnice o souřadnicích [x,y] */
    short getTile(short &x, short &y) {
        return this->board[this->mapPosition(x, y)];
    }

    /** Proveď pohyb se souřadnic from na souřadnice to */
    void move(pair<short, short> &from, pair<short, short> &to) {
        short stone = this->getTile(from.first, from.second);
        this->board[this->mapPosition(from.first, from.second)] = '-';
        /** Pokud je proveden přesun na pěšáka -> pěšák sebrán */
        if (this->getTile(to.first, to.second) == 'P') {
            this->pawnsCnt--;
        }
        this->board[this->mapPosition(to.first, to.second)] = stone;
    }

    /** Funkce pro výpis */

    void printTile(short &x, short &y) {
        cout << "[" << x << "," << y << "]: " << this->getTile(x, y) << endl;
    }

    void printRookPosition() {
        cout << "Rook position: [" + to_string(this->rookPosition.first) + "," +
                to_string(this->rookPosition.second) + "]" << endl;
    }

    void printKnightPosition() {
        cout << "Knight position: [" + to_string(this->knightPosition.first) + "," +
                to_string(this->knightPosition.second) + "]" << endl;
    }

    string serialize() {
        string board = "";
        for (short i = 0; i < k; ++i) {
            for (short j = 0; j < k; ++j) {
                board += this->board[this->mapPosition(i, j)];
            }
            board += '\n';
        }
        return "Pawns: " + to_string(this->pawnsCnt) + "\nLowerBound: " + to_string(this->lowerBound) + "\nUpperBound: " + to_string(this->upperBound) + "\n" + board;
    }
};

/** Komparační funkce pro setřídění pohybů dle jejich ceny */
bool compareMoves(const pair<pair<short, short>, short> &a, const pair<pair<short, short>, short> &b) {
    return a.second > b.second;
}

/** Reprezentace problému */
class Game {

public:
    ChessBoard chessBoard;
    char turn;

    Game(void) {}

    /** Construct state object from structure */
    Game(const state_structure s): turn(s.turn) {
        this->chessBoard = ChessBoard(s);
    }

    /** Pole skoků jezdce */
    pair<short, short> knightMoves[8] = {
            make_pair(-2, -1), make_pair(-2, 1),
            make_pair(-1, 2), make_pair(1, 2),
            make_pair(2, -1), make_pair(2, 1),
            make_pair(-1, -2), make_pair(1, -2),
    };

    /** Zjištění, zda se na ose [x,j] nebo [i,y] nachází pěšák */
    bool isPawnOnAxes(short x, short y) {
        for (short i = 0; i < this->chessBoard.k; ++i) {
            if (this->chessBoard.getTile(x, i) == 'P') {
                return true;
            }
            if (this->chessBoard.getTile(i, y) == 'P') {
                return true;
            }
        }
        return false;
    }

    /** Ohodnocení tahu věže */
    short valRook(short x, short y, short tile) {
        if (tile == 'P') {
            return 2;
        }
        if (this->isPawnOnAxes(x, y)) {
            return 1;
        }
        return 0;
    }

    /** Získání možných tahů věže seřazených dle jejich ceny */
    vector<pair<pair < short, short>, short>> nextRook() {
        vector < pair < pair < short, short >, short >> nextMoves;
        pair<short, short> position = this->chessBoard.rookPosition;
        /** Vyhodnoť tahy po osách souřadnice věže */
        for (short i = 0; i < this->chessBoard.k; ++i) {
            if (this->chessBoard.canMoveRookTo(position.first, i)) {
                short tile = this->chessBoard.getTile(position.first, i);
                short val = this->valRook(position.first, i, tile);
                nextMoves.push_back(make_pair(make_pair(position.first, i), val));
            }
        }
        for (short i = 0; i < this->chessBoard.k; i++) {
            if (this->chessBoard.canMoveRookTo(i, position.second)) {
                short tile = this->chessBoard.getTile(i, position.second);
                short val = this->valRook(i, position.second, tile);
                nextMoves.push_back(make_pair(make_pair(i, position.second), val));
            }
        }
        sort(nextMoves.begin(), nextMoves.end(), compareMoves);
        return nextMoves;
    }

    /** Ohodnocení tahu jezdce */
    short valKnight(short tile) {
        if (tile == 'P') {
            return 2;
        }
        return 0;
    }

    /** Získání možných tahů jezdce seřazených dle jejich ceny */
    vector<pair<pair < short, short>, short>> nextKnight() {
        vector < pair < pair < short, short >, short >> nextMoves;
        pair<short, short> position = this->chessBoard.knightPosition;
        /** Vyhodnoť všech 8 možných skoků jezdce */
        for (short i = 0; i < 8; ++i) {
            short x = position.first - this->knightMoves[i].first;
            short y = position.second - this->knightMoves[i].second;
            if (this->chessBoard.canMoveKnightTo(x, y)) {
                short tile = this->chessBoard.getTile(x, y);
                short val = this->valKnight(tile);
                nextMoves.push_back(make_pair(make_pair(x, y), val));
            }
        }
        sort(nextMoves.begin(), nextMoves.end(), compareMoves);
        return nextMoves;
    }

    /** Získání možných tahů seřazených dle jejich ceny v daném kroku (pro V nebo J) */
    vector<pair<pair < short, short>, short>> next() {
        if (this->turn == 'V') {
            return this->nextRook();
        }
        return this->nextKnight();
    }

    /** Inicializace hry */
    void initGame(unsigned short &k, unsigned short &upperBound, string &boardStr) {
        this->turn = 'V';
        this->chessBoard = ChessBoard(k, upperBound, boardStr);
    }

    /** Proveď pohyb na cílovou souřadnici dest */
    void move(pair<pair<short, short>, short> &dest) {
        if (this->turn == 'V') {
            this->chessBoard.move(this->chessBoard.rookPosition, dest.first);
            this->chessBoard.rookPosition = dest.first;
            this->turn = 'J';
        } else {
            this->chessBoard.move(this->chessBoard.knightPosition, dest.first);
            this->chessBoard.knightPosition = dest.first;
            this->turn = 'V';
        }
    }

    string serialize() {
        string res = this->chessBoard.serialize();
        res += "Turn: ";
        res += this->turn;
        return res;
    }
};

class State {

public:
    Game game;
    pair<pair<short, short>, short> nextMove;
    unsigned short cost;
    vector<short> path;

    State(void) {}

    State(Game game, pair<pair<short, short>, short> nextMove, unsigned short cost) {
        this->game = game;
        this->nextMove = nextMove;
        this->cost = cost;
    }

    /** Construct state object from structure */
    State(const state_structure s): cost(s.cost) {
        for (int i = 0; i < MAX_PATH; ++i) {
            if (s.path[i] == -1) { break; }
            this->path.push_back(s.path[i]);
        }
        this->nextMove = make_pair(make_pair(s.nextMove[0], s.nextMove[1]), s.nextMove[2]);
        this->game = Game(s);
    }

    void move () {
        this->game.move(this->nextMove);
        this->path.emplace_back((this->nextMove.first.first * 1000) + this->nextMove.first.second);
        this->cost++;
    }

    void setNextMove(pair<pair<short, short>, short> nextMove) {
        this->nextMove = nextMove;
    }

    /** Porovnání aktuální stavu s jiným stavem */
    bool compare(State &other) {
        /** Pokud je věž na jiné pozici, stavy se liší */
        if (!(this->game.chessBoard.rookPosition == other.game.chessBoard.rookPosition)) {
            return false;
        }
        /** Pokud je jezdec na jiné pozici, stavy se liší */
        if (!(this->game.chessBoard.knightPosition == other.game.chessBoard.knightPosition)) {
            return false;
        }
        /** Pokud se liší počet pěšáků, stavy se liší */
        if (this->game.chessBoard.pawnsCnt != other.game.chessBoard.pawnsCnt) {
            return false;
        }
        /** V případě, že nebyl zjištěn rozdíl v jiných stavových proměnných, porovnej řetězce šachovnic */
        return this->game.chessBoard.board == other.game.chessBoard.board;
    }

    state_structure toStruct() {
        state_structure res = {
                {},
                { this->game.chessBoard.rookPosition.first, this->game.chessBoard.rookPosition.second},
                { this->game.chessBoard.knightPosition.first, this->game.chessBoard.knightPosition.second },
                this->game.chessBoard.k,
                this->game.chessBoard.pawnsCnt,
                this->game.chessBoard.lowerBound,
                this->game.chessBoard.upperBound,
                this->game.turn,
                this->cost,
                {},
                { this->nextMove.first.first, this->nextMove.first.second, this->nextMove.second }
        };

        for (short int i = 0; i < this->game.chessBoard.getSize(); i++) {
            res.board[i] = this->game.chessBoard.board[i];
        }

        for (unsigned int i = 0; i < MAX_PATH; i++) {
            if (i < this->path.size()) {
                res.path[i] = this->path[i];
            } else {
                res.path[i] = -1;
            }
        }

        return res;
    }

    string serialize() {
        string res = this->game.serialize() + "\n";
        res += ("Cost: " + to_string(this->cost) + "\n");
        res += "Path: [";
        for (size_t i = 0; i < this->path.size(); ++i) {
            res += (to_string(this->path[i]) + ", ");
        }
        res += "]\n";
        return res;
    }

};

/** Komparační funkce pro setřídění stavů dle jejich perspektivy */
bool compareStates(const State &a, const State &b) {
    return a.game.chessBoard.pawnsCnt < b.game.chessBoard.pawnsCnt;
}

/** Globální nejlepší stav */
State OPT_STATE;

/** Check validity of the input arguments */
void checkInputArgs(const int &argc, char** argv) {
    if (argc != 4) {
        char errMsg[200];
        sprintf(errMsg, "Invalid number of arguments.\n%s <slave_threads> <master_expansion_depth> <slave_expansion_depth>", argv[0]);
        throw std::runtime_error(errMsg);
    }
}

Game init() {

    /** Load problem metadata */
    unsigned short k, upperbound;
    cin >> k >> upperbound;

    /** Load chessboard string */
    string tmp, boardStr;
    for (short i = 0; i < k; ++i) {
        cin >> tmp;
        boardStr += tmp;
    }

    /** Initialize the problem */
    Game game = Game();
    game.initGame(k, upperbound, boardStr);

    return game;

}

void expandStates(State state, vector <State> &states, short &depth) {

    /** If it's not the initial step, make a move */
    if (state.nextMove.second != -1) {
        state.move();
    }

    /** If the solution can't be better than the current one,
     * or at least the same as the upper bound, return from the branch.
     */
    if (
            ((state.cost + state.game.chessBoard.pawnsCnt) >= OPT_COST) ||
            ((state.cost + state.game.chessBoard.pawnsCnt) > state.game.chessBoard.upperBound)
            || state.cost > 3
            ) {
        return;
    }

    /**
     * When the required depths is reached, store the state as a root of the subtree in the state space.
     * Return from the recursion.
     */
    if (state.cost >= depth) {
        state.setNextMove(make_pair(make_pair(-1, -1), -1));
        states.push_back(state);
        return;
    }

    /** Get vector of next moves sorted by their price and call the recursion. */
    vector < pair < pair < short, short >, short >> moves = state.game.next();
    for (unsigned short i = 0; i < moves.size(); i++) {
        state.setNextMove(moves[i]);
        expandStates(state, states, depth);
    }

}

void filterStates(vector<State> &input, deque<State> &output) {
    for (unsigned int i = 0; i < input.size(); ++i) {
        bool unique = true;
        for (unsigned int j = i + 1; j < input.size(); ++j) {
            if (input[i].compare(input[j])) {
                unique = false;
            }
        }
        if (unique) {
            output.push_back(input[i]);
        }
    }
}

/** Probe the global optimum from MASTER process in order to update local optimum */
void slaveProbeGlobalOptimum() {
    int flag;
    #pragma omp critical
    {
        MPI_Iprobe(MASTER_PROCESS, TAG_UPDATE, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
        if (flag) {
            unsigned short globalOptimum;
            MPI_Recv(&globalOptimum, 1, MPI_UNSIGNED_SHORT, MASTER_PROCESS, TAG_UPDATE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            int procNum;
            MPI_Comm_rank(MPI_COMM_WORLD, &procNum);
            if(globalOptimum <= OPT_COST) {
                OPT_COST = globalOptimum;
            }
        }
    }
}

/** Notify MASTER process about new local (within specific SLAVE process) optimal price. */
void slaveNotifyLocalOptimum(const unsigned short &c) {
    MPI_Request request = MPI_REQUEST_NULL;
    MPI_Isend(&c, 1, MPI_UNSIGNED_SHORT, MASTER_PROCESS, TAG_UPDATE, MPI_COMM_WORLD, &request);
}


void solve(State state, int &procNumber) {
    /** Pokud se nejezdná o počáteční krok, proveď tah */
    if (state.nextMove.second != -1) {
        state.move();
//        if (state.cost % 3 == 2) {
//            slaveProbeGlobalOptimum();
//        }
        if (ITERATION % 1000 == 0) {
            slaveProbeGlobalOptimum();
        }
    }

    /** V případě nalezení lepšího řešení, aktualizuj stávající nejlepší */
    if (state.game.chessBoard.pawnsCnt == 0 && state.cost < OPT_COST) {
    #pragma omp critical
        {
            if (state.game.chessBoard.pawnsCnt == 0 && state.cost < OPT_COST) {
//                printf("[SLAVE-%d] Found better solution [%d]\n", procNumber, state.cost);
                OPT_COST = state.cost;
                OPT_STATE = state;
                slaveNotifyLocalOptimum(state.cost);
            }
        }
    }

    /** Pokud řešení nemůže být lepší než stávající nejlepší, nebo než lepší či rovno horní mezi, ukončí větev */
    if (
            ((state.cost + state.game.chessBoard.pawnsCnt) >= OPT_COST) ||
            ((state.cost + state.game.chessBoard.pawnsCnt) > state.game.chessBoard.upperBound)
            ) {
        return;
    }

    ITERATION++;

    /** Získej vektor dalších tahů seřazených dle ceny a rekurentně se zanoř */
    vector < pair < pair < short, short >, short >> moves = state.game.next();
    for (unsigned short i = 0; i < moves.size(); i++) {
        state.setNextMove(moves[i]);
        solve(state, procNumber);
    }
}

int main(int argc, char *argv[]) {

    checkInputArgs(argc, argv);

    /** Check if the MPI satisfies requirements */
    int required = MPI_THREAD_FUNNELED;
    int provided;
    MPI_Init_thread(&argc, &argv, required, &provided);
    if (provided < required) {
        throw std::runtime_error("MPI library does not provide required threading support.");
    }

    /** Get the number of process and total number of processes */
    int procNumber, procTotal;
    MPI_Comm_rank(MPI_COMM_WORLD, &procNumber);
    MPI_Comm_size(MPI_COMM_WORLD, &procTotal);

    short slaveThreads = atoi(argv[1]);
    short masterExpansionDepth = atoi(argv[2]);
    short slaveExpansionDepth = atoi(argv[3]);

//    printf("[slaveThreads, masterExpansionDepth, slaveExpansionDepth] = [%d, %d, %d]\n\n", slaveThreads, masterExpansionDepth, slaveExpansionDepth);

    if (procNumber == MASTER_PROCESS) {

        /** Master process */

        chrono::steady_clock::time_point _start(chrono::steady_clock::now());

//        printf("[MASTER]");
//
//        printf("[INIT] Required: %d\n\n", required);
//        printf("[INIT] Provided: %d\n\n", provided);
//
//        printf("[INIT] Processes total: %d\n\n", procTotal);
//        printf("[INIT] Process number: %d\n\n", procNumber);
//
//        printf("[INIT] Master expansion depth: %d\n\n", masterExpansionDepth);
//        printf("[INIT] Slave expansion depth: %d\n\n", slaveExpansionDepth);

        Game problem = init();

//        printf("[MASTER] Problem initialized\n\n");
//        printf("%s\n\n", problem.serialize().c_str());

        /** Create root of the state space */
        State initState = State(problem, make_pair(make_pair(-1, -1), -1), 0);

        /** Vector for storing the master expansion states */
        vector<State> masterExpStates;

        /** Expand states and sort them by perspective (number of removed pawns) */
        expandStates(initState, masterExpStates, masterExpansionDepth);
        sort(masterExpStates.begin(), masterExpStates.end(), compareStates);

        /** Remove duplicate states */
        deque<State> masterTasks;
        filterStates(masterExpStates, masterTasks);

//        cout << "[MASTER] Tasks" << endl;
//        for (unsigned int i = 0; i < masterTasks.size(); ++i) {
//            printf("%s\n\n", masterTasks[i].serialize().c_str());
//        }

        queue<short> freeSlaves = {};
        for (int i = 1; i < procTotal; i++) {
            freeSlaves.push(i);
        }

        /** Distribute expanded tasks to the slaves */
        while (!masterTasks.empty()) {
            state_structure taskStruct = masterTasks.front().toStruct();
            masterTasks.pop_front();
            MPI_Send(&taskStruct, sizeof(state_structure), MPI_CHAR, freeSlaves.front(), TAG_WORK, MPI_COMM_WORLD);
            freeSlaves.pop();

            /** If all the slaves are working, wait for a message from any slave */
            while(freeSlaves.empty()) {
                MPI_Status status;
                MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                /**
                 * If master received message with update tag:
                 *      1) Try to update optimal price
                 */
                if (status.MPI_TAG == TAG_UPDATE) {
                    unsigned short cost;
                    MPI_Recv(&cost, 1, MPI_UNSIGNED, MPI_ANY_SOURCE, TAG_UPDATE, MPI_COMM_WORLD, &status);
//                    printf("[MASTER] Received cost [%d] from [SLAVE-%d]\n\n", cost, status.MPI_SOURCE);
                    if (cost > 0 && cost <= OPT_COST) {
//                        printf("[MASTER] New optimal cost [%d]\n\n", cost);
                        OPT_COST = cost;
                        for (int dest = 1; dest < procTotal; dest++)
                        {
                            if (dest != status.MPI_SOURCE) {
                                /// cout << "[MASTER]: Sending cost : " << cost << " to WORKER-" << dest << endl;
                                MPI_Send(&cost, 1, MPI_UNSIGNED_SHORT, dest, TAG_UPDATE, MPI_COMM_WORLD);
                            }
                        }
                    }
                    else {
                        MPI_Send(&OPT_COST, 1, MPI_UNSIGNED_SHORT, status.MPI_SOURCE, TAG_UPDATE, MPI_COMM_WORLD);
                    }
                } else if (status.MPI_TAG == TAG_DONE) {
                    MPI_Recv(&taskStruct, sizeof(state_structure), MPI_CHAR, MPI_ANY_SOURCE, TAG_DONE, MPI_COMM_WORLD, &status);
                    State deserialized = State(taskStruct);
//                    printf("[MASTER] Received DONE from SLAVE-%d\n\n", status.MPI_SOURCE);
//                    printf("[MASTER] Received state:\n%s\n\n", deserialized.serialize().c_str());
//                    printf("[MASTER] Optimal cost: %d\n\n", OPT_COST);
                    if (deserialized.cost > 0 && deserialized.cost <= OPT_COST) {
//                        printf("[MASTER] Update optimum [%d] -> [%d]\n\n", OPT_COST, deserialized.cost);
                        OPT_COST = deserialized.cost;
                        OPT_STATE = deserialized;
                    }
                    freeSlaves.push(status.MPI_SOURCE);
                    break;
                } else {
                    throw std::runtime_error("[MASTER] Unknown message received.");
                    break;
                }
            }
        }

        /** All slaves finished given tasks
         *  (All slaves sent TAG_DONE and master didn't have any tasks left)
         */
//        printf("[MASTER] No tasks left.\n\n");
//        printf("[MASTER] Running slaves number: %lu\n\n", (procTotal - 1) - freeSlaves.size());

        for (int i = freeSlaves.size(); i < procTotal - 1; i++) {
            state_structure stateStruct;
            MPI_Status status;
            MPI_Recv(&stateStruct, sizeof(state_structure), MPI_CHAR, MPI_ANY_SOURCE, TAG_DONE, MPI_COMM_WORLD, &status);
            State deserialized = State(stateStruct);
//            printf("[MASTER] Received DONE from SLAVE-%d\n\n", status.MPI_SOURCE);
//            printf("[MASTER] Received state:\n%s\n\n", deserialized.serialize().c_str());
//            printf("[MASTER] Optimal cost: %d\n\n", OPT_COST);
            if (deserialized.cost > 0 && deserialized.cost <= OPT_COST) {
//                printf("[MASTER] Update optimum [%d] -> [%d]\n\n", OPT_COST, deserialized.cost);
                OPT_COST = deserialized.cost;
                OPT_STATE = deserialized;
            }
        }

//        printf("[MASTER] Began terminating slaves.\n\n");

        for (int slaveId = 1; slaveId < procTotal; slaveId++) {
//            printf("[MASTER] Terminating SLAVE-%d\n\n", slaveId);
            int msg = 1;
            MPI_Send(&msg, 1, MPI_INT, slaveId, TAG_FINISHED, MPI_COMM_WORLD);
        }

        chrono::steady_clock::time_point _end(chrono::steady_clock::now());

//        printf("[MASTER] All slaves terminated.\n\n");

//        printf("[MASTER] OPTIMAL SOLUTION:\n");
        printf("Cost: %d\n", OPT_COST);

        short x, y;
        vector<pair<short, short>> taken;
        pair<short, short> coordinates;

        /** Print the best found configuration */
        string conf = "";
        for (unsigned int i = 0; i < OPT_STATE.path.size(); ++i) {
            conf += (i % 2 ? "J" : "V");
            x = OPT_STATE.path[i] / 1000;
            y = OPT_STATE.path[i] % 1000;
            conf += ("[" + to_string(x) + " " + to_string(y) + "]");
            coordinates = make_pair(x, y);
            if (find(taken.begin(), taken.end(), coordinates) == taken.end()) {
                if (problem.chessBoard.getTile(x, y) == 'P') {
                    conf += "*";
                }
            }
            if (i < OPT_STATE.path.size() - 1) {
                conf += ", ";
            }
            taken.emplace_back(make_pair(x, y));
        }

        printf("%s\n", conf.c_str());

        printf("Time: %f\n", chrono::duration_cast<chrono::duration<double>>(_end - _start).count());

    } else {

        /** Slave process */
//        printf("[SLAVE-%d]\n\n", procNumber);

        while (true) {
            MPI_Status status;
            state_structure stateStruct;
            MPI_Recv(&stateStruct, sizeof(state_structure), MPI_CHAR, MASTER_PROCESS, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

//            printf("[SLAVE-%d] Received tag [%d]\n\n", procNumber, status.MPI_TAG);

            if (status.MPI_TAG == TAG_FINISHED) {
//                printf("[SLAVE-%d] Finished\n\n", procNumber);
                break;
            }

            State task = State(stateStruct);
//            printf("[SLAVE-%d] Received task:\n%s\n\n", procNumber, task.serialize().c_str());

            vector<State> slaveExpStates;

            /** Expand states and sort them by perspective (number of removed pawns) */
            expandStates(task, slaveExpStates, slaveExpansionDepth);
            sort(slaveExpStates.begin(), slaveExpStates.end(), compareStates);

//            printf("[SLAVE-%d] Tasks cnt [%lu]\n\n", procNumber, slaveExpStates.size());

            /** Remove duplicate states */
            deque<State> slaveTasks;
            filterStates(slaveExpStates, slaveTasks);

            /** Process task using parallel cycle */
            #pragma omp parallel for schedule(dynamic, 1) num_threads(slaveThreads)
            for (unsigned int i = 0; i < slaveTasks.size(); ++i) {
                /** Solve the given state space subtree */
                solve(slaveTasks[i], procNumber);
            }

            stateStruct = OPT_STATE.toStruct();
//            printf("[SLAVE-%d] Task done.\n\n", procNumber);
//            printf("[SLAVE-%d] OPT_COST [%d]\n\n", procNumber, OPT_COST);
//            printf("[SLAVE-%d] Sending state:\n", procNumber);
//            printf("%s\n", OPT_STATE.serialize().c_str());
            MPI_Send(&stateStruct, sizeof(state_structure), MPI_CHAR, MASTER_PROCESS, TAG_DONE, MPI_COMM_WORLD);
        }

    }

    MPI_Finalize();
}