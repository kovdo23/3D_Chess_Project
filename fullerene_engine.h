#ifndef ONLAB_FULLERENE_ENGINE_H
#define ONLAB_FULLERENE_ENGINE_H

#include <iostream>
#include <vector>
#include <map>
#include <algorithm>

enum class TileType { Pentagon, Hexagon };

struct Tile {
    int id;
    TileType type;
    std::vector<int> edgeNeighbors;
    std::vector<int> vertexNeighbors;
};

class FullereneGrid {
private:
    std::map<int, Tile> tiles;

public:
    FullereneGrid() { initializeGrid(); }

    bool isPentagon(int id) const {
        return (id - 1) % 6 == 0 && id <= 67;
    }

    const Tile& getTile(int id) const {
        return tiles.at(id);
    }

    // Megadja, hogy egy hatszogben melyik szomszed van szemben a belepesi irannyal
    int getOppositeNeighbor(int currentId, int arrivalId) const {
        const Tile& t = tiles.at(currentId);

        //Otszognek nincs szemkozti ele
        if (t.type == TileType::Pentagon) return -1;

        const auto& n = t.edgeNeighbors;
        int arrivalIndex = -1;

        //Megadja melyik iranybol lep be a mezore
        for (int i = 0; i < 6; ++i) {
            if (n[i] == arrivalId) {
                arrivalIndex = i;
                break;
            }
        }

        if (arrivalIndex == -1) return -1;

        // A fix, oramutato szerinti kiosztas miatt a szemkozti el indexe:
        // (belépési index + 3) % 6
        int oppositeIndex = (arrivalIndex + 3) % 6;

        return n[oppositeIndex];
    }

    void initializeGrid() {
        //Mezok alaphelyzetbe allitasa fix meretu szomszed-listakkal
        for (int i = 1; i <= 72; ++i) {
            tiles[i].id = i;
            tiles[i].type = isPentagon(i) ? TileType::Pentagon : TileType::Hexagon;
            int size = (tiles[i].type == TileType::Pentagon ? 5 : 6);
            tiles[i].edgeNeighbors.assign(size, -1);
            tiles[i].vertexNeighbors.assign(size, -1);
        }

        std::map<int, std::vector<int>> pLinks;
        //felso pont koruli otszogek
        pLinks[1]  = {7, 13, 19, 25, 31};
        pLinks[7]  = {1, 31, 49, 43, 13};
        pLinks[13] = {1, 7, 43, 37, 19};
        pLinks[19] = {1, 13, 37, 61, 25};
        pLinks[25] = {1, 19, 61, 55, 31};
        pLinks[31] = {1, 25, 55, 49, 7};

        //also pont koruli otszogek
        pLinks[67] = {37, 43, 49, 55, 61};
        pLinks[37] = {67, 61, 19, 13, 43};
        pLinks[43] = {67, 37, 13, 7, 49};
        pLinks[49] = {67, 43, 7, 31, 55};
        pLinks[55] = {67, 49, 31, 25, 61};
        pLinks[61] = {67, 55, 25, 19, 37};

        // halozat felepitese fix index-kiosztassal
        for (int p = 1; p <= 67; p += 6) {
            if (!isPentagon(p)) continue;

            for (int i = 0; i < 5; ++i) {
                int hex = p + 1 + i;
                int nextHex = p + 1 + (i + 1) % 5;
                int prevHex = p + 1 + (i + 4) % 5;
                int targetP = pLinks[p][i];
                int nextTargetP = pLinks[p][(i + 1) % 5];

                //HATSZOG ORIENTALT SZOMSZEDAI (0-5 indexek)
                tiles[hex].edgeNeighbors[0] = p;
                tiles[hex].edgeNeighbors[1] = prevHex;

                int j = -1;
                for(int k=0; k<5; ++k) {
                    if(pLinks[targetP][k] == p) j = k;
                }

                tiles[hex].edgeNeighbors[2] = targetP + 1 + j;
                tiles[hex].edgeNeighbors[3] = targetP + 1 + (j + 4) % 5;

                int k_idx = -1;
                for(int k=0; k<5; ++k) {
                    if(pLinks[nextTargetP][k] == p) k_idx = k;
                }
                tiles[hex].edgeNeighbors[4] = nextTargetP + 1 + k_idx;
                tiles[hex].edgeNeighbors[5] = nextHex;

                //OTSZOG SZOMSZEDAI
                tiles[p].edgeNeighbors[i] = hex;
            }
        }

        //Csucsszomszedsagi algoritmus
        for (int p = 1; p <= 72; ++p) {
            int pSize = tiles[p].edgeNeighbors.size();

            for (int k = 0; k < pSize; ++k) {
                int n1 = tiles[p].edgeNeighbors[k];
                int n2 = tiles[p].edgeNeighbors[(k + 1) % pSize];

                if (n1 == -1 || n2 == -1) continue;

                int n1Size = tiles[n1].edgeNeighbors.size();
                int n2Size = tiles[n2].edgeNeighbors.size();

                for (int i = 0; i < n1Size; ++i) {
                    for (int j = 0; j < n2Size; ++j) {
                        if (tiles[n1].edgeNeighbors[i] != -1 &&
                            tiles[n1].edgeNeighbors[i] != p &&
                            tiles[n1].edgeNeighbors[i] == tiles[n2].edgeNeighbors[j]) {
                            tiles[p].vertexNeighbors[k] = tiles[n1].edgeNeighbors[i];
                        }
                    }
                }
            }
        }
    }

    std::vector<int> getVertexNeighbors(int id) const {
        if (tiles.find(id) == tiles.end()) return {};
        return tiles.at(id).vertexNeighbors;
    }

    //teszteleshez kiirja a szomszedokat
    void printNeighbors(int id) {
        if (tiles.find(id) == tiles.end()) return;
        std::vector<int> n = tiles[id].edgeNeighbors;
        std::cout << "Mezo " << id << " (" << (tiles[id].type == TileType::Pentagon ? "Otszog" : "Hatszog") << ")\nLapszomszedok: ";
        for (int x : n) std::cout << x << " ";

        std::vector<int> neighbors = getVertexNeighbors(id);
        std::cout << "\nCsúcsszomszédok: " ;
        for (int y : neighbors) std::cout << y << " ";
        std::cout << "\n\n";
    }
};

#endif //ONLAB_FULLERENE_ENGINE_H