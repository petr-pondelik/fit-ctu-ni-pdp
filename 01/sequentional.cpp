#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <bits/stdc++.h>
#include <chrono>
#include <thread>
#include <limits>

using namespace std;

unsigned int OPT_COST = numeric_limits<unsigned int>::max();
vector<short> OPT_CONFIGURATION;
unsigned long long ITERATION = 0;


bool compareMoves(const pair<pair<short, short>, short> &a, const pair<pair<short, short>, short> &b) {
    return a.second > b.second;
}

class ChessBoard {

public:
    // Array of size k^2 mapped to 2D array
    vector<char> board;
    pair<short, short> rookPosition;
    pair<short, short> knightPosition;
    unsigned short k, pawnsCnt, lowerBound, upperBound;

    ChessBoard(void) {}

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

    bool fitsDimensions(short &x, short &y) {
        return x >= 0 && x < this->k && y >= 0 && y < this->k;
    }

    bool isTileEmpty(short x, short y) {
        return this->getTile(x, y) == '-';
    }

    bool canMoveRookTo(short x, short y) {
        if (!this->fitsDimensions(x, y)) {
            return false;
        }

        short xDiff = x - this->rookPosition.first;
        short yDiff = y - this->rookPosition.second;

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

        return (this->getTile(x, y) == '-') || (this->getTile(x, y) == 'P');
    }

    bool canMoveKnightTo(short &x, short &y) {
        if (!this->fitsDimensions(x, y)) {
            return false;
        }
        return (this->getTile(x, y) == '-') || (this->getTile(x, y) == 'P');
    }

    short getSize() {
        return this->k * this->k;
    }

    short mapPosition(short &x, short &y) {
        return x * this->k + y;
    }

    short getTile(short &x, short &y) {
        return this->board[this->mapPosition(x, y)];
    }

    void move(pair<short, short> &from, pair<short, short> &to) {
        short stone = this->getTile(from.first, from.second);
        this->board[this->mapPosition(from.first, from.second)] = '-';
        if (this->getTile(to.first, to.second) == 'P') {
            this->pawnsCnt--;
        }
        this->board[this->mapPosition(to.first, to.second)] = stone;
    }

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


class Game {

public:
    // ChessBoard represents configuration
    ChessBoard chessBoard;

    short turn;

    pair<short, short> knightMoves[8] = {
            make_pair(-2, -1), make_pair(-2, 1),
            make_pair(-1, 2), make_pair(1, 2),
            make_pair(2, -1), make_pair(2, 1),
            make_pair(-1, -2), make_pair(1, -2),
    };

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

    short valRook(short x, short y, short tile) {
        if (tile == 'P') {
            return 2;
        }
        if (this->isPawnOnAxes(x, y)) {
            return 1;
        }
        return 0;
    }

    vector<pair<pair < short, short>, short>> nextRook() {
        vector < pair < pair < short, short >, short >> nextMoves;
        pair<short, short> position = this->chessBoard.rookPosition;
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

    short valKnight(short tile) {
        if (tile == 'P') {
            return 2;
        }
        return 0;
    }

    vector<pair<pair < short, short>, short>> nextKnight() {
        vector < pair < pair < short, short >, short >> nextMoves;
        pair<short, short> position = this->chessBoard.knightPosition;
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

    vector<pair<pair < short, short>, short>> next() {
        if (this->turn == 'V') {
            return this->nextRook();
        }
        return this->nextKnight();
    }

    void initGame(unsigned short &k, unsigned short &upperBound, string &boardStr) {
        this->turn = 'V';
        this->chessBoard = ChessBoard(k, upperBound, boardStr);
    }

    bool terminate(unsigned int &cost) {
        if ((cost + this->chessBoard.pawnsCnt) >= OPT_COST) {
            return true;
        }
        if ((cost + this->chessBoard.pawnsCnt) > this->chessBoard.upperBound) {
            return true;
        }
        return false;
    }

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
//    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (dest.second != -1) {
        game.move(dest);
//        cout << "Cost: " << cost << endl;
//        game.chessBoard.print();
        conf.emplace_back((dest.first.first * 1000) + dest.first.second);
    }
    if (game.chessBoard.pawnsCnt == 0 && cost < OPT_COST) {
        OPT_COST = cost;
        OPT_CONFIGURATION = conf;
    }
    ITERATION++;
    if (ITERATION == 1000000000 || ITERATION == 5000000000 || ITERATION == 10000000000 || ITERATION == 30000000000 ||
            ITERATION == 100000000000 || ITERATION == 200000000000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        cout << "Iteration: " << ITERATION << endl;
        cout << "Best: " << OPT_COST << endl;
        game.chessBoard.print();
        cout << "Cost: " << cost << endl;
    }
    if (game.terminate(cost)) {
        return;
    }
    vector < pair < pair < short, short >, short >> moves = game.next();
    for (unsigned short i = 0; i < moves.size(); i++) {
        solve(game, moves[i], cost + 1, conf);
    }
}

int main(int argc, char *argv[]) {
    unsigned short k, upperbound;
    cin >> k >> upperbound;

    string tmp, boardStr;
    for (short i = 0; i < k; ++i) {
        cin >> tmp;
        boardStr += tmp;
    }

    Game game = Game();
    game.initGame(k, upperbound, boardStr);
    game.chessBoard.print();

    solve(game, make_pair(make_pair(-1, -1), -1), 0, OPT_CONFIGURATION);

    cout << "=======================" << endl;
    cout << "Best cost: " << OPT_COST << endl;
    cout << "Iterations: " << ITERATION << endl;
    cout << "Best configuration: " << endl;

    short x, y;
    vector<pair<short, short>> taken;
    pair<short, short> coordinates;

    for (unsigned int i = 0; i < OPT_CONFIGURATION.size(); ++i) {
        cout << (i % 2 ? "J" : "V");
        x = (OPT_CONFIGURATION[i] / 1000);
        y = OPT_CONFIGURATION[i] % 1000;
        cout << "[" << x << "," << y << "]";

        coordinates = make_pair(x, y);

        if (find(taken.begin(), taken.end(), coordinates) == taken.end()) {
            if (game.chessBoard.getTile(x, y) == 'P') {
                cout << "*";
            }
        }

        cout << " ";
        taken.emplace_back(make_pair(x, y));
    }

    cout << endl;
}