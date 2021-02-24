#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <bits/stdc++.h>

using namespace std;


enum TileStatus {
    Rook, Knight, Pawn, Empty
};

bool compareMoves(const pair<pair<short, short>, short> &a, const pair<pair<short, short>, short> &b) {
    return a.second > b.second;
}

class ChessBoard {

private:
    short k;
    // Array of size k^2 mapped to 2D array
    vector<short> board;
    pair<short, short> rookPosition;
    pair<short, short> knightPosition;

    short mapToTileStatus(char input) {
        switch (input) {
            case '-':
                return 3;
            case 'P':
                return 2;
            case 'J':
                return 1;
            case 'V':
                return 0;
            default:
                return '-';
        }
    }

    char tileStatusToStr(short status) {
        switch (status) {
            case 3:
                return '-';
            case 2:
                return 'P';
            case 1:
                return 'J';
            case 0:
                return 'V';
            default:
                return '-';
        }
    }

public:
    short turn;

    ChessBoard(void) {}

    ChessBoard(short k, string &boardStr) {
        cout << "ChessBoard" << endl;
        this->k = k;
        this->turn = TileStatus::Rook;
        for (short i = 0; i < k; i++) {
            for (short j = 0; j < k; j++) {
                short tileStatus = this->mapToTileStatus(boardStr[this->mapPosition(i, j)]);
                if (tileStatus == TileStatus::Rook) {
                    this->rookPosition = make_pair(i, j);
                } else if (tileStatus == TileStatus::Knight) {
                    this->knightPosition = make_pair(i, j);
                }
                this->board.emplace_back(tileStatus);
            }
        }
        this->print();
    };

    short getDimension() {
        return this->k;
    }

    bool fitsDimensions(short x, short y) {
        return x >= 0 && x < this->k && y >= 0 && y < this->k;
    }

    bool isTileEmpty(short x, short y) {
        return this->getTile(x, y) == TileStatus::Empty;
    }

    bool canMoveRookTo(short x, short y) {
        short xDiff = x - this->rookPosition.first;
        short yDiff = y - this->rookPosition.second;

        if (xDiff > 0) {
            for (short i = 1; i < xDiff; ++i) {
                if (!this->isTileEmpty(this->rookPosition.first + i, this->rookPosition.second)) {
                    return false;
                }
            }
        } else if (xDiff < 0) {
            for (short i = xDiff; i < -1; ++i) {
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
            for (short i = yDiff; i < -1; ++i) {
                if (!this->isTileEmpty(this->rookPosition.first + i, this->rookPosition.second + i)) {
                    return false;
                }
            }
        }

        return (this->getTile(x, y) == TileStatus::Empty) || (this->getTile(x, y) == TileStatus::Pawn);
    }

    bool canMoveKnightTo(short x, short y) {
        if (!this->fitsDimensions(x, y)) {
            return false;
        }
        return (this->getTile(x, y) == TileStatus::Empty) || (this->getTile(x, y) == TileStatus::Pawn);
    }

    short getSize() {
        return this->k * this->k;
    }

    short mapPosition(short x, short y) {
        return x * this->k + y;
    }

    short getTile(short x, short y, bool print = false) {
        if (print) {
            cout << "[" + to_string(x) + "," + to_string(y) + "]" << endl;
        }
        return this->board[this->mapPosition(x, y)];
    }

    pair<short, short> getRookPosition() {
        return this->rookPosition;
    }

    pair<short, short> getKnightPosition() {
        return this->knightPosition;
    }

    void printTile(short x, short y) {
        cout << "[" << x << "," << y << "]: " << this->getTile(x, y) << endl;
    }

    void printRookPosition() {
        cout << "Rook position: [" + to_string(this->getRookPosition().first) + "," +
                to_string(this->getRookPosition().second) + "]" << endl;
    }

    void printKnightPosition() {
        cout << "Knight position: [" + to_string(this->getKnightPosition().first) + "," +
                to_string(this->getKnightPosition().second) + "]" << endl;
    }

    void print() {
        for (short i = 0; i < k; ++i) {
            for (short j = 0; j < k; ++j) {
                cout << this->tileStatusToStr(this->board[this->mapPosition(i, j)]);
            }
            cout << endl;
        }
    }
};


class Game {

public:
    // ChessBoard represents configuration
    ChessBoard chessBoard;

    pair<short, short> knightMoves[8] = {
            make_pair(-2, -1), make_pair(-2, 1),
            make_pair(-1, 2), make_pair(1, 2),
            make_pair(2, -1), make_pair(2, 1),
            make_pair(-1, -2), make_pair(1, -2),
    };

    bool isPawnOnAxes(short x, short y) {
        for (short i = 0; i < this->chessBoard.getDimension(); ++i) {
            if (this->chessBoard.getTile(x, i) == TileStatus::Pawn) {
                return true;
            }
            if (this->chessBoard.getTile(i, y) == TileStatus::Pawn) {
                return true;
            }
        }
        return false;
    }

    short valRook(short x, short y, short tile) {
        if (tile == TileStatus::Pawn) {
            return 2;
        }
        if (this->isPawnOnAxes(x, y)) {
            return 1;
        }
        return 0;
    }

    vector<pair<pair < short, short>, short>> nextRook() {
        cout << endl << "NextRook" << endl;
        vector < pair < pair < short, short >, short >> nextMoves;
        this->chessBoard.printRookPosition();
        pair<short, short> position = this->chessBoard.getRookPosition();
        for (short i = 0; i < this->chessBoard.getDimension(); ++i) {
            if (this->chessBoard.canMoveRookTo(position.first, i)) {
                short tile = this->chessBoard.getTile(position.first, i);
                this->chessBoard.printTile(position.first, i);
                short val = this->valRook(position.first, i, tile);
                nextMoves.push_back(make_pair(make_pair(position.first, i), val));
            }
            if (this->chessBoard.canMoveRookTo(i, position.second)) {
                short tile = this->chessBoard.getTile(i, position.second);
                this->chessBoard.printTile(i, position.second);
                short val = this->valRook(i, position.second, tile);
                nextMoves.push_back(make_pair(make_pair(i, position.second), val));
            }
        }
        sort(nextMoves.begin(), nextMoves.end(), compareMoves);
        return nextMoves;
    }

    short valKnight(short tile) {
        if (tile == TileStatus::Pawn) {
            return 2;
        }
        return 0;
    }

    vector<pair<pair < short, short>, short>> nextKnight() {
        cout << endl << "NextKnight" << endl;
        vector < pair < pair < short, short >, short >> nextMoves;
        this->chessBoard.printKnightPosition();
        pair<short, short> position = this->chessBoard.getKnightPosition();
        for (short i = 0; i < 8; ++i) {
            short x = position.first - this->knightMoves[i].first;
            short y = position.second - this->knightMoves[i].second;
            if (this->chessBoard.canMoveKnightTo(x, y)) {
                this->chessBoard.printTile(x, y);
                short tile = this->chessBoard.getTile(x, y);
                short val = this->valKnight(tile);
                nextMoves.push_back(make_pair(make_pair(x, y), val));
            }
        }
        sort(nextMoves.begin(), nextMoves.end(), compareMoves);
        return nextMoves;
    }

    vector<pair<pair < short, short>, short>> next() {
        if (this->chessBoard.turn == TileStatus::Rook) {
            return this->nextRook();
        }
        return this->nextKnight();
    }

    void initGame(short k, string &boardStr) {
        cout << "Init game" << endl;
        this->chessBoard = ChessBoard(k, boardStr);
    }

    bool isFinal() {
//        TODO
        return false;
    }
};

void solve(Game game) {
    vector < pair < pair < short, short >, short >> moves = game.next();
    cout << endl;
    for (unsigned short i = 0; i < moves.size(); ++i) {
        game.chessBoard.printTile(moves[i].first.first, moves[i].first.second);
        cout << "Val: " << moves[i].second << endl;
    }
}

int main(int argc, char *argv[]) {
    short k, maxDepth;
    cin >> k >> maxDepth;

    string tmp;
    string boardStr;
    for (short i = 0; i < k; ++i) {
        cin >> tmp;
        boardStr += tmp;
    }

    Game game = Game();
    game.initGame(k, boardStr);
    solve(game);
}