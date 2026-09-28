#pragma once
#include <map>
#include <string>
#include <vector>

// One maze read from mazes.txt.
struct MazeData {
    std::string name;
    int size = 0;                    // grid is size x size
    std::vector<std::string> rows;   // each row: '1' = wall, '0' = open
};

// Loads and indexes every maze in the text file, grouped by size.
class MazeLibrary {
public:
    // Returns false only if the file could not be opened.
    bool load(const std::string& path);

    std::vector<int> sizes() const;               // sorted sizes that have mazes
    int count(int size) const;                    // how many mazes of this size
    const MazeData* get(int size, int index) const; // nullptr if out of range

    int total() const { return totalCount; }
    int skipped() const { return skippedCount; }  // malformed blocks ignored

private:
    std::map<int, std::vector<MazeData>> bySize;
    int totalCount = 0;
    int skippedCount = 0;
};
