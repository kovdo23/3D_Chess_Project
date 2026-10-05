//
// Created by Kovács Domonkos on 2026. 09. 21..
//

#ifndef ONLAB_FULLERENE_CHESS_H
#define ONLAB_FULLERENE_CHESS_H

#include "fullerene_engine.h" //jatekter
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <string>

// ----- SAKK SZABALYOK -----
//babuk szine es tipusa
enum class Team { White, Black };
enum class PieceType { Flag, Rook, bishop };

class Piece {
public:
    int currentTileId; //az adott korben a babu pozicioja (valtozik)
    Team team;
    PieceType type;
    bool isCaptured;
    // 0: RookMode, 1: BishopMode (csak Flag eseten szamit)
    int flagState = 0;

    Piece(int startTile, Team t, PieceType type)
        : currentTileId(startTile), team(t), type(type), isCaptured(false) {}

    //virtualis fuggveny a lehetseges lepesekhez
    std::vector<int> getValidMoves(const FullereneGrid& grid, const std::map<int, Piece*>& boardState) {
        if (this->type == PieceType::Flag) {
            return getFlagSwaps(grid, boardState);
        }
        if (this->type == PieceType::Rook) {
            return getRookMoves(grid, boardState);
        }
        if (this->type == PieceType::bishop) {
            return getBishopMoves(grid, boardState);
        }
        return {};
    }

    std::vector<int> getFlagSwaps(const FullereneGrid& grid, const std::map<int, Piece*>& boardState) {
        std::vector<int> swaps;
        const auto& neighbors = grid.getTile(currentTileId).edgeNeighbors;

        for (int neighborId : neighbors) {
            if (neighborId != -1 && boardState.count(neighborId)) {
                Piece* other = boardState.at(neighborId);
                if (other->team == this->team) {
                    //ha a flag bastya modban (0) van -> futot keres
                    if (this->flagState == 0 && other->type == PieceType::bishop) {
                        swaps.push_back(neighborId);
                    }
                    //ha a flag futo modban (1) van -> bastyat keres
                    else if (this->flagState == 1 && other->type == PieceType::Rook) {
                        swaps.push_back(neighborId);
                    }
                }
            }
        }
        return swaps;
    }

    //ezt a fuggvenyt hivom meg a GameLogic-ban, ha a Flag lep
    void executeFlagSwap(Piece* other) {
        //kapcsolo logika (flagstate 0, ha a zaszlo bastya, 1, ha futo)
        if (this->flagState == 0) {
            this->type = PieceType::Rook;
            other->type = PieceType::Flag;
            other->flagState = 1;
        }
        else {
            this->type = PieceType::bishop;
            other->type = PieceType::Flag;
            other->flagState = 0;
        }
        std::swap(this->currentTileId, other->currentTileId);
    }

    //Segedfuggveny az ismetlodesek eltavolitasara
    void removeDuplicates(std::vector<int>& moves) {
        std::sort(moves.begin(), moves.end());
        moves.erase(std::unique(moves.begin(), moves.end()), moves.end());
    }

    //bastya mozgasanak szabalyai
    std::vector<int> getRookMoves(const FullereneGrid& grid, const std::map<int, Piece*>& boardState){
        std::vector<int> moves;
        const auto& startNeighbors = grid.getTile(currentTileId).edgeNeighbors;

        for (int startDirId : startNeighbors) {
            int prevId = currentTileId;
            int currId = startDirId;

            //Ciklus a "vonalon" valo haladashoz
            while (currId != -1) {
                //utkozesvizsgalat
                if (boardState.count(currId)) {
                    if (boardState.at(currId)->team != this->team) {
                        moves.push_back(currId); // Az ellenseges babu leutheto
                    }
                    break;
                }

                moves.push_back(currId);

                //otszog szabaly: Ha otszegbe lepunk, megall a bastya
                if (grid.isPentagon(currId)) {
                    break;
                }

                //haladas a kovetkezo hatszogre
                int nextId = grid.getOppositeNeighbor(currId, prevId);
                prevId = currId;
                currId = nextId;
            }
        }
        removeDuplicates(moves);
        return moves;
    }

    std::vector<int> getBishopMoves(const FullereneGrid& grid, const std::map<int, Piece*>& boardState) {
        std::vector<int> moves;
        const Tile& currentTile = grid.getTile(currentTileId);

        for (int i = 0; i < currentTile.vertexNeighbors.size(); ++i) {
            int prevId = currentTileId;
            int currId = currentTile.vertexNeighbors[i];

            //sorompo ellenorzese az elso lepesre
            int side1Id = currentTile.edgeNeighbors[i];
            int side2Id = currentTile.edgeNeighbors[(i + 1) % currentTile.edgeNeighbors.size()];

            bool blocked = false;
            if (side1Id != -1 && side2Id != -1 && boardState.count(side1Id) && boardState.count(side2Id)) {
                Piece* p1 = boardState.at(side1Id);
                Piece* p2 = boardState.at(side2Id);
                // CSAK akkor blokkol, ha MINDKETTŐ ellenséges
                if (p1->team != this->team && p2->team != this->team) {
                    blocked = true;
                }
            }

            if (blocked) continue;

            while (currId != -1) {
                //Utkozesvizsgalat a celmezon
                if (boardState.count(currId)) {
                    if (boardState.at(currId)->team != this->team) {
                        moves.push_back(currId);
                    }
                    break;
                }
                moves.push_back(currId);

                //ha otszogre lep megall
                if (grid.isPentagon(currId)) break;

                //haladas tovabb a szemkozti csucson
                const Tile& nextTile = grid.getTile(currId);
                int arrivalIndex = -1;
                for (int k = 0; k < nextTile.vertexNeighbors.size(); ++k) {
                    if (nextTile.vertexNeighbors[k] == prevId) {
                        arrivalIndex = k;
                        break;
                    }
                }

                if (arrivalIndex != -1) {
                    int nextDir = (arrivalIndex + 3) % 6;
                    prevId = currId;

                    //itt is ellenorizni kell a "sorompot" minden lepesnel
                    int s1 = nextTile.edgeNeighbors[nextDir];
                    int s2 = nextTile.edgeNeighbors[(nextDir + 1) % 6];

                    if (s1 != -1 && s2 != -1 && boardState.count(s1) && boardState.count(s2)) {
                        Piece* ps1 = boardState.at(s1);
                        Piece* ps2 = boardState.at(s2);
                        if (ps1->team != this->team && ps2->team != this->team) {
                            break;
                        }
                    }

                    currId = nextTile.vertexNeighbors[nextDir];
                } else {
                    currId = -1;
                }
            }
        }
        removeDuplicates(moves);
        return moves;
    }

    bool canMoveTo(int targetId, const FullereneGrid& grid, const std::map<int, Piece*>& boardState) {
        if (targetId == currentTileId) return false;

        std::vector<int> validMoves = getValidMoves(grid, boardState);

        if (std::find(validMoves.begin(), validMoves.end(), targetId) != validMoves.end()) {
            if (type == PieceType::Flag) {
                std::cout << "Zaszlo csere: " << currentTileId << " <-> " << targetId << "\n";
            } else {
                std::cout << "Sikeres lepes: " << currentTileId << " -> " << targetId << "\n";
            }
            return true;
        }
        return false;
    }
};

//ATFOGO MOZGASTER TESZT - UNIT TESZT
void UnitTest(const FullereneGrid& grid, const std::string& filename) {
    std::ofstream outFile(filename);
    for (int i = 1; i <= 72; ++i) {
        outFile << "Mezo " << i << ":\n";

        Piece rook(i, Team::White, PieceType::Rook);
        std::map<int, Piece*> emptyBoard;
        emptyBoard[i] = &rook;
        auto rookMoves = rook.getRookMoves(grid, emptyBoard);
        outFile << "  Bastya (" << rookMoves.size() << " lepes): ";
        for (int m : rookMoves) outFile << m << " ";

        Piece bishop(i, Team::White, PieceType::bishop);
        emptyBoard.clear();
        emptyBoard[i] = &bishop;
        auto bishopMoves = bishop.getBishopMoves(grid, emptyBoard);
        outFile << "\n  Futo (" << bishopMoves.size() << " lepes): ";
        for (int m : bishopMoves) outFile << m << " ";
        outFile << "\n\n";
    }
    outFile.close();
}

//KEZDOALLAS SZABALYOS LEPESEINEK GENERALASA
void InitialStateTest(const FullereneGrid& grid, std::map<int, Piece*>& board, Team currentTurn, const std::string& filename) {
    std::ofstream outFile(filename);
    outFile << "Szabalyos lepesek a kezdopoziciobol (" << (currentTurn == Team::White ? "FEHER" : "FEKETE") << "):\n";

    for (auto const& [pos, piece] : board) {
        if (piece->team == currentTurn) {
            auto moves = piece->getValidMoves(grid, board);
            if (!moves.empty()) {
                if ((int)piece->type == 0) {
                    outFile << "Babu: FLAG" << " Mezon: " << pos << " -> Lehetseges celpontok: ";
                }else if ((int)piece->type == 1) {
                    outFile << "Babu: ROOK"<< " Mezon: " << pos << " -> Lehetseges celpontok: ";
                }else {
                    outFile << "Babu: BISHOP" << " Mezon: " << pos << " -> Lehetseges celpontok: ";
                }
                for (int m : moves) outFile << m << " ";
                outFile << "\n";
            }
        }
    }
    outFile.close();
}

//RANDOM JATSZMA SZIMULACIO - END-TO-END TESZT
void RandomGameSimulation(const FullereneGrid& grid, std::map<int, Piece*> board, const std::string& filename) {
    std::ofstream outFile(filename);
    Team turn = Team::White;
    srand(time(0));

    bool gameOver = false;
    int step = 1;

    outFile << "TELJES RANDOM JATSZMA SZIMULACIO\n\n";

    while (!gameOver) {
        struct Move { int from; int to; Piece* p; };
        std::vector<Move> allValidMoves;

        for (auto const& [pos, piece] : board) {
            if (piece->team == turn) {
                auto targets = piece->getValidMoves(grid, board);
                for (int target : targets) allValidMoves.push_back({pos, target, piece});
            }
        }

        if (allValidMoves.empty()) {
            if (turn == Team::White) outFile << "GYOZELEM - a fekete csapat nyert\n";
            else outFile << "GYOZELEM - a feher csapat nyert\n";
            outFile << "GAME OVER a(z) " << step << ". korben.\n";
            break;
        }

        Move m = allValidMoves[rand() % allValidMoves.size()];

        outFile << step << ". lepes: " << (turn == Team::White ? "FEHER" : "FEKETE");
        if (m.p->type == PieceType::Flag) outFile << " [FLAG]";
        else if (m.p->type == PieceType::Rook) outFile << " [ROOK]";
        else outFile << " [BISHOP]";
        outFile << " " << m.from << " -> " << m.to;

        //gyozelem feltetel ellenorzese
        if (board.count(m.to)) {
            Piece* targetPiece = board[m.to];
            if (targetPiece->team != turn && targetPiece->type == PieceType::Flag) {
                outFile << " (!!! ZASZLO UTES - GYOZELEM !!!)\n";
                gameOver = true;
            }
        }

        //Csak akkor sáncol, ha maga a lépő bábu a Zászló
        if (!gameOver && m.p->type == PieceType::Flag) {
            m.p->executeFlagSwap(board[m.to]);
            outFile << " (ZASZLO CSERE)\n";
        }
        else if (!gameOver) {
            if (board.count(m.to)) outFile << " (UTES!)";
            board.erase(m.from);
            m.p->currentTileId = m.to;
            board[m.to] = m.p;
            outFile << "\n";
        }

        turn = (turn == Team::White) ? Team::Black : Team::White;
        step++;

        if (step > 300) {
            outFile << "Dontetlen\n";
            break;
        }
    }

    outFile.close();
    std::cout << "Szimulacio befejezodott " << step-1 << " lepes utan." << std::endl;
}

class GameEngine {
public:
    FullereneGrid board;
    std::map<int, Piece*> pieces;
    Team currentTurn;
    bool gameOver;

    GameEngine() : currentTurn(Team::White), gameOver(false) {
        initializePieces();
    }

    ~GameEngine() {
        for (auto const& [id, piece] : pieces) delete piece;
    }

    void initializePieces() {
        //feher csapat
        pieces[1] = new Piece(1, Team::White, PieceType::Flag);
        int wRooks[] = {8, 14, 20, 26, 32};
        for(int pos : wRooks) pieces[pos] = new Piece(pos, Team::White, PieceType::Rook);
        int wBishops[] = {2, 3, 4, 5, 6};
        for(int pos : wBishops) pieces[pos] = new Piece(pos, Team::White, PieceType::bishop);

        //fekete csapat
        pieces[67] = new Piece(67, Team::Black, PieceType::Flag);
        int bRooks[] = {38, 44, 50, 56, 62};
        for(int pos : bRooks) pieces[pos] = new Piece(pos, Team::Black, PieceType::Rook);
        int bBishops[] = {68, 69, 70, 71, 72};
        for(int pos : bBishops) pieces[pos] = new Piece(pos, Team::Black, PieceType::bishop);
    }

    bool processMove(int onnan, int hova) {
        if (gameOver) return false;

        if (pieces.find(onnan) == pieces.end() || pieces[onnan]->team != currentTurn) {
            return false;
        }

        Piece* selectedPiece = pieces[onnan];

        if (selectedPiece->canMoveTo(hova, board, pieces)) {
            executeMoveLogic(onnan, hova);
            currentTurn = (currentTurn == Team::White) ? Team::Black : Team::White;
            return true;
        }
        return false;
    }

private:
    void executeMoveLogic(int onnan, int hova) {
        Piece* selectedPiece = pieces[onnan];

        if (pieces.count(hova) && pieces[hova]->team != currentTurn && pieces[hova]->type == PieceType::Flag) {
            std::cout << "!!! ZASZLO UTES - GYOZELEM !!!" << std::endl;
            gameOver = true;
        }

        if (!gameOver && selectedPiece->type == PieceType::Flag) {
            //Normal zaszlo csere
            Piece* otherPiece = pieces[hova];
            pieces.erase(onnan);
            pieces.erase(hova);
            selectedPiece->executeFlagSwap(otherPiece);
            pieces[selectedPiece->currentTileId] = selectedPiece;
            pieces[otherPiece->currentTileId] = otherPiece;
        }
        else if (!gameOver) {
            //Normal lepes vagy utes
            pieces.erase(onnan);
            selectedPiece->currentTileId = hova;
            pieces[hova] = selectedPiece;
        }
    }
};

#endif //ONLAB_FULLERENE_CHESS_H