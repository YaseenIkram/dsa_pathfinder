#include "MazeLibrary.hpp"
#include <cctype>
#include <fstream>
#include <sstream>

namespace {

    std::string trim(const std::string& s) {
        size_t a = 0, b = s.size();
        while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) a++;
        while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) b--;
        return s.substr(a, b - a);   // also strips the '\r' of Windows files
    }

    bool isMazeRow(const std::string& s, int size) {
        if (static_cast<int>(s.size()) != size) return false;
        for (char c : s) {
            if (c != '0' && c != '1') return false;
        }
        return true;
    }

}

bool MazeLibrary::load(const std::string& path) {

    std::ifstream in(path);
    if (!in.is_open()) return false;

    bySize.clear();
    totalCount = 0;
    skippedCount = 0;

    enum class State { WaitHeader, ReadRows };
    State state = State::WaitHeader;

    MazeData current;
    bool bad = false;

    std::string raw;

    while (std::getline(in, raw)) {

        std::string line = trim(raw);

        // Comments and blank lines are ignored everywhere.
        if (line.empty() || line[0] == ';') continue;

        // ---------------- HEADER: MAZE <size> <name...> ----------------
        if (line.compare(0, 4, "MAZE") == 0) {

            // A new header while still reading = previous block had no END.
            if (state == State::ReadRows) skippedCount++;

            std::istringstream hs(line);
            std::string tag;
            int size = 0;
            hs >> tag >> size;

            if (tag != "MAZE" || size <= 0 || size > 200) {
                skippedCount++;
                state = State::WaitHeader;
                continue;
            }

            std::string name;
            std::getline(hs, name);
            name = trim(name);

            current = MazeData();
            current.size = size;
            current.name = name.empty() ? "Maze" : name;
            bad = false;
            state = State::ReadRows;
        }

        // ---------------------------- END ------------------------------
        else if (line == "END") {

            if (state == State::ReadRows) {
                if (!bad && static_cast<int>(current.rows.size()) == current.size) {
                    bySize[current.size].push_back(current);
                    totalCount++;
                } else {
                    skippedCount++;
                }
            }
            state = State::WaitHeader;
        }

        // --------------------------- ROW DATA --------------------------
        else if (state == State::ReadRows) {

            if (isMazeRow(line, current.size)) current.rows.push_back(line);
            else bad = true;
        }
        // Anything else (stray text between mazes) is ignored.
    }

    // File ended in the middle of a maze.
    if (state == State::ReadRows) skippedCount++;

    return true;
}

std::vector<int> MazeLibrary::sizes() const {
    std::vector<int> out;
    for (const auto& kv : bySize) out.push_back(kv.first);   // std::map = sorted
    return out;
}

int MazeLibrary::count(int size) const {
    auto it = bySize.find(size);
    return it == bySize.end() ? 0 : static_cast<int>(it->second.size());
}

const MazeData* MazeLibrary::get(int size, int index) const {
    auto it = bySize.find(size);
    if (it == bySize.end()) return nullptr;
    if (index < 0 || index >= static_cast<int>(it->second.size())) return nullptr;
    return &it->second[index];
}
