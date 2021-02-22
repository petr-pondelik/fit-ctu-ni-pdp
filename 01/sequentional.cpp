#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;


enum TileStatus {
    Rook, Knight, Pawn, Empty
};


class ChessBoard {

private:
    // Array of size k^2 mapped to 2D array
    int k;
    vector<int> board;
    pair<int,int> rookPosition;
    pair<int,int> knightPosition;

    int mapToTileStatus(char input) {
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

    char tileStatusToStr(int status) {
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
    ChessBoard(void) {}

    ChessBoard(int k, string &boardStr) {
        cout << "ChessBoard" << endl;
        this->k = k;
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < k; j++) {
                int tileStatus = this->mapToTileStatus(boardStr[this->mapPosition(i, j)]);
                if (tileStatus == TileStatus::Rook) {
                    this->rookPosition = make_pair(i,j);
                } else if (tileStatus == TileStatus::Knight) {
                    this->knightPosition = make_pair(i,j);
                }
                this->board.emplace_back(tileStatus);
            }
        }
        this->print();
    };

    int getDimension() {
        return this->k;
    }

    bool fitsDimensions(int x, int y) {
        return x >= 0 && x < this->k && y >= 0 && y < this->k;
    }

    bool canBeTaken(int x, int y) {
        return (this->getTile(x,y) == TileStatus::Empty) || (this->getTile(x,y) == TileStatus::Pawn);
    }

    int getSize() {
        return this->k * this->k;
    }

    int mapPosition(int x, int y) {
        return x * this->k + y;
    }

    int getTile(int x, int y, bool print = false) {
        if (print) {
            cout << "[" + to_string(x) + "," + to_string(y)  + "]" << endl;
        }
        return this->board[this->mapPosition(x,y)];
    }

    pair<int,int> getRookPosition() {
        return this->rookPosition;
    }

    void printRookPosition() {
        cout << "Rook position: [" + to_string(this->getRookPosition().first) + "," + to_string(this->getRookPosition().second) + "]" << endl;
    }

    pair<int,int> getKnightPosition() {
        return this->knightPosition;
    }

    void printKnightPosition() {
        cout << "Knight position: [" + to_string(this->getKnightPosition().first) + "," + to_string(this->getKnightPosition().second) + "]" << endl;
    }

    void print() {
        for (int i = 0; i < k; ++i) {
            for (int j = 0; j < k; ++j) {
                cout << this->tileStatusToStr(this->board[this->mapPosition(i,j)]);
            }
            cout << endl;
        }
    }
};


class Game {

private:
    int maxDepth;

    pair<int,int> knightMoves[8] = {
            make_pair(-2,-1), make_pair(-2,1),
            make_pair(-1,2), make_pair(1,2),
            make_pair(2,-1), make_pair(2,1),
            make_pair(-1,-2), make_pair(1,-2),
    };

    // ChessBoard represents configuration
    ChessBoard *chessBoard;

    int turn;

    int solution;
    int bestSolution;

    vector<int> nextRook() {
//        TODO
        cout << "NextRook" << endl;
        this->chessBoard->printRookPosition();
        pair<int,int> position = this->chessBoard->getRookPosition();
        for (int i = 0; i < this->chessBoard->getDimension(); ++i) {
            if (this->chessBoard->canBeTaken(position.first, i)) {
                int tileX = this->chessBoard->getTile(position.first, i, true);
                cout << "TileX: " + to_string(tileX) << endl;
            }
            if (this->chessBoard->canBeTaken(i, position.second)) {
                int tileY = this->chessBoard->getTile(i, position.second, true);
                cout << "TileY: " + to_string(tileY) << endl;
            }
        }
        vector<int> next;
        return next;
    }

    vector<int> nextKnight() {
//        TODO
        cout << "NextKnight" << endl;
        this->chessBoard->printKnightPosition();
        pair<int,int> position = this->chessBoard->getKnightPosition();
        for (int i = 0; i < 8; ++i) {
            int xRes = position.first - this->knightMoves[i].first;
            int yRes = position.second - this->knightMoves[i].second;
            if (this->chessBoard->fitsDimensions(xRes, yRes)) {
                cout << this->chessBoard->getTile(xRes, yRes) << endl;
            }
        }
        vector<int> next;
        return next;
    }

    vector<int> next() {
//        if (this->turn == TileStatus::Rook) {
//            return this->nextRook();
//        }
//        return this->nextKnight();
        this->nextRook();
        return this->nextKnight();
    }

public:
    Game(int maxDepth) {
        this->maxDepth = maxDepth;
    }

    void initGame(int k, string &boardStr) {
        cout << "Init game" << endl;
        this->turn = TileStatus::Rook;
        this->chessBoard = new ChessBoard(k, boardStr);
    }

    bool isFinal() {
//        TODO
        return false;
    }

    void move() {
        vector<int> next = this->next();
    }

    void solve() {
        this->move();
    };
};


int main(int argc, char *argv[]) {
    int k, maxDepth;

    cin >> k;
    cin >> maxDepth;

    string tmp;
    string boardStr;
    for (int i = 0; i < k; ++i) {
        cin >> tmp;
        boardStr += tmp;
    }

    Game *game = new Game(maxDepth);
    game->initGame(k, boardStr);
    game->solve();
}