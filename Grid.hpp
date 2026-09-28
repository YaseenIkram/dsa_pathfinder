#ifndef GRID_HPP
#define GRID_HPP

#include <vector>
#include <SFML/Graphics.hpp>
#include "Node.hpp"

class Grid {
public:
    int rows;
    int cols;
    float nodeSize; 
    float margin; // NEW: Protects the grid from window clipping
    
    std::vector<std::vector<Node>> table;

    Grid(int r, int c, float size, float m);
    void draw(sf::RenderWindow& window);
    void setObstacle(float mouseX, float mouseY, bool state); 
};

#endif