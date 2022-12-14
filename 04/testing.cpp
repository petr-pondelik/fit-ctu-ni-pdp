#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <bits/stdc++.h>
#include <limits>
#include <omp.h>
#include <chrono>

using namespace std;

// =====================================================================================================================
// CONSTANTS

#define MAX_K 400
#define MAX_PATH 50

// =====================================================================================================================
// GLOBAL VARIABLES

unsigned int OPT_COST = numeric_limits<unsigned int>::max();
unsigned short THREAD_CNT;

// =====================================================================================================================
// MPI state structure

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

// =====================================================================================================================

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

    void print() {
        cout << endl;
        cout << "Pawns: " << this->pawnsCnt << endl;
        cout << "Lower bound: " << this->lowerBound << endl;
        cout << "Upper bound: " << this->upperBound << endl;
        for (short i = 0; i < k; ++i) {
            for (short j = 0; j < k; ++j) {
                cout << this->board[this->mapPosition(i, j)];
            }
            cout << endl;
        }
        cout << endl;
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
        cout << "Turn: " << this->turn << endl;
        this->chessBoard.print();
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
        cout << "Construct state from structure" << endl;

        cout << "Cost:" << this->cost << endl;

        for (int i = 0; i < MAX_PATH; ++i) {
            if (s.path[i] == 0) { break; }
            this->path.push_back(s.path[i]);
        }
        cout << "Path: ";
        for (unsigned int i = 0; i < this->path.size(); ++i) {
            cout << this->path[i] << " ";
        }
        cout << endl;

        this->nextMove = make_pair(make_pair(s.nextMove[0], s.nextMove[1]), s.nextMove[2]);
        cout << "Next move: [ [" << this->nextMove.first.first << "," << this->nextMove.first.second << "], " << this->nextMove.second << "]" << endl;

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

        for (unsigned int i = 0; i < this->path.size(); i++) {
            res.path[i] = this->path[i];
        }

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
    if (argc != 3) {
        char errMsg[200];
        sprintf(errMsg, "Invalid number of arguments.\n%s <master_expansion_depth> <worker_expansion_depth>", argv[0]);
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

int main(int argc, char *argv[]) {

    short masterExpansionDepth = atoi(argv[1]);
    short slaveExpansionDepth = atoi(argv[2]);

    Game problem = init();
    problem.chessBoard.print();

    /** Create root of the state space */
    State initState = State(problem, make_pair(make_pair(-1, -1), -1), 0);

    /** Vector for storing the master expansion states */
    vector<State> masterExpStates;

    /** Expand states and sort them by perspective (number of removed pawns) */
    expandStates(initState, masterExpStates, masterExpansionDepth);
    sort(masterExpStates.begin(), masterExpStates.end(), compareStates);

    /** Odstranění duplicitních stavů */
    deque<State> masterExpStatesUnique;
    for (unsigned int i = 0; i < masterExpStates.size(); ++i) {
        bool unique = true;
        for (unsigned int j = i + 1; j < masterExpStates.size(); ++j) {
            if (masterExpStates[i].compare(masterExpStates[j])) {
                unique = false;
            }
        }
        if (unique) {
            masterExpStatesUnique.push_back(masterExpStates[i]);
        }
    }

    /** Testing state's state_structure conversion method */

    masterExpStatesUnique[0].game.chessBoard.print();

    state_structure s = masterExpStatesUnique[0].toStruct();

    cout << s.board << endl;
    cout << "[" << s.rookPosition[0] << ", " << s.rookPosition[1] << "]" << endl;
    cout << "[" << s.knightPosition[0] << ", " << s.knightPosition[1] << "]" << endl;
    cout << s.k << endl;
    cout << s.pawnsCnt << endl;
    cout << s.lowerBound << endl;
    cout << s.upperBound << endl;
    cout << s.turn << endl;
    cout << s.cost << endl;
    for (unsigned int i = 0; i < 50; i++) {
        cout << s.path[i] << " ";
    }
    cout << endl;

    cout << "[" << s.nextMove[0] << "," << s.nextMove[1] << "," << s.nextMove[2] << "]" << endl << endl;

//    char board[MAX_K];
//    short rookPosition[2];
//    short knightPosition[2];
//    unsigned short k;
//    unsigned short pawnsCnt;
//    unsigned short lowerBound;
//    unsigned short upperBound;
//    char turn;
//    unsigned short cost;
//    short path[MAX_PATH];
//    short nextMove[3];

    State constructed = State(s);

    cout << sizeof(state_structure) << endl;
    cout << sizeof(bool) << endl;
}