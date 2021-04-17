#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <bits/stdc++.h>
#include <limits>
#include <chrono>

using namespace std;

/** Globální proměnné */
unsigned int OPT_COST = numeric_limits<unsigned int>::max();
vector<short> OPT_CONFIGURATION;
unsigned long long ITERATION = 0;

/** Komparační funkce pro setřídění pohybů dle jejich ceny */
bool compareMoves(const pair<pair<short, short>, short> &a, const pair<pair<short, short>, short> &b) {
    return a.second > b.second;
}

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

/** Reprezentace problému */
class Game {

public:
    ChessBoard chessBoard;
    char turn;

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
        for (short i = 0; i < this->chessBoard.k; ++i) {
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

void solve(Game game, pair<pair<short, short>, short> dest, unsigned int cost, vector<short> conf) {
    /** Pokud se nejezdná o počáteční krok, proveď tah */
    if (dest.second != -1) {
        game.move(dest);
//        cout << "Cost: " << cost << endl;
//        game.chessBoard.print();
        conf.emplace_back((dest.first.first * 1000) + dest.first.second);
    }
    /** V případě nalezení lepšího řešení, aktualizuj stávající nejlepší */
    if (game.chessBoard.pawnsCnt == 0 && cost < OPT_COST) {
        OPT_COST = cost;
        OPT_CONFIGURATION = conf;
    }

//    ITERATION++;

    /** Pokud řešení nemůže být lepší než stávající nejlepší, nebo lepší či rovno horní mezi, ukončí větev */
    if ( ((cost + game.chessBoard.pawnsCnt) >= OPT_COST) || ((cost + game.chessBoard.pawnsCnt) > game.chessBoard.upperBound) ) {
        return;
    }
    /** Získej vektor dalších tahů seřazených dle ceny a rekurentně se zanoř */
    vector < pair < pair < short, short >, short >> moves = game.next();
    for (unsigned short i = 0; i < moves.size(); i++) {
        solve(game, moves[i], cost + 1, conf);
    }
}

int main(int argc, char *argv[]) {
    unsigned short k, upperbound;
    cin >> k >> upperbound;

    /** Načtení řetězce čachovnice */
    string tmp, boardStr;
    for (short i = 0; i < k; ++i) {
        cin >> tmp;
        boardStr += tmp;
    }

    /** Inicializace problému */
    Game game = Game();
    game.initGame(k, upperbound, boardStr);
//    game.chessBoard.print();

    chrono::steady_clock::time_point _start(chrono::steady_clock::now());

    /** Rekurentní nalezení nejlepší posloupnosti tahů */
    solve(game, make_pair(make_pair(-1, -1), -1), 0, OPT_CONFIGURATION);

    chrono::steady_clock::time_point _end(chrono::steady_clock::now());

    short x, y;
    vector<pair<short, short>> taken;
    pair<short, short> coordinates;

    /** Výpis nejlepší nalezené konfigurace */
    for (unsigned int i = 0; i < OPT_CONFIGURATION.size(); ++i) {
        cout << (i % 2 ? "J" : "V");
        x = (OPT_CONFIGURATION[i] / 1000);
        y = OPT_CONFIGURATION[i] % 1000;
        cout << "[" << x << " " << y << "]";
        coordinates = make_pair(x, y);
        if (find(taken.begin(), taken.end(), coordinates) == taken.end()) {
            if (game.chessBoard.getTile(x, y) == 'P') {
                cout << "*";
            }
        }
        if (i < OPT_CONFIGURATION.size() - 1) {
            cout << ", ";
        }
        taken.emplace_back(make_pair(x, y));
    }
    cout << endl;

    cout << "=======================" << endl;
    cout << "Best cost: " << OPT_COST << endl;
//    cout << "Iterations: " << ITERATION << endl;
    cout << "Time: " << chrono::duration_cast<chrono::duration<double>>(_end - _start).count() << endl;
}