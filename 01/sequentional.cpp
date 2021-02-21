#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

enum field {Rook, Knight, Pawn, Empty};

class ChessBoard {

private:
    vector<int> board;

public:
    ChessBoard(void) {}
    ChessBoard(int k, vector<string> & boardStr)
    {
        cout << "ChessBoard" << endl;
        cout << k << endl;
    };
};

class Game {

private:
    int maxDepth;
    ChessBoard * chessBoard;
//    moves
//    active - Rook or Knight

public:
    Game(int maxDepth)
    {
        this->maxDepth = maxDepth;
    }

    void initGame(int k, vector<string> & boardStr)
    {
        cout << "Init game" << endl;
        cout << this->maxDepth << endl;
        this->chessBoard = new ChessBoard(k, boardStr);
    }
};

int main(int argc, char *argv[]) {
    cout << "START" << endl;

    int k, maxDepth;

    cin >> k;
    cout << k << endl;

    cin >> maxDepth;
    cout << maxDepth << endl;

    string tmp;
    vector<string> boardStr;
    for (int i = 0; i < k; ++i) {
        cin >> tmp;
        boardStr.emplace_back(tmp);
    }

    Game * game = new Game(maxDepth);
    game->initGame(k, boardStr);
}