#include "TileIndex.hpp"
#include <fstream>
#include <sstream>

using namespace std;

bool TileIndex::load(const string& metaFile){
    tiles_.clear();
    ifstream fin(metaFile);
    if(!fin) return false;
    string header; getline(fin, header); // simple header
    string line;
    while(getline(fin,line)){
        if(line.empty()) continue;
        stringstream ss(line);
        TileMeta m; ss>>m.x>>m.y>>m.w>>m.h>>m.file; if(ss) tiles_.push_back(m);
    }
    return true;
}

bool TileIndex::save(const string& metaFile) const {
    ofstream fout(metaFile);
    if(!fout) return false;
    fout << "x y w h file" << '\n';
    for(auto &m: tiles_){
        fout << m.x << ' ' << m.y << ' ' << m.w << ' ' << m.h << ' ' << m.file << '\n';
    }
    return true;
}

void TileIndex::setTiles(vector<TileMeta> tiles){ tiles_ = std::move(tiles); }

vector<TileMeta> TileIndex::query(const Viewport& vp) const {
    vector<TileMeta> out;
    for(auto &m: tiles_){
        bool overlap = !(m.x + m.w <= vp.x || m.y + m.h <= vp.y || m.x >= vp.x + vp.w || m.y >= vp.y + vp.h);
        if(overlap) out.push_back(m);
    }
    return out;
}
