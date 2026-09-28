#ifndef PATHFINDER_HPP
#define PATHFINDER_HPP

#include "Grid.hpp"
#include "MinHeap.hpp"

class Pathfinder {
public:
    bool isSolving;        // Tracks if the animation is currently running
    MinHeap openSet;       // The priority queue
    Node* startNode;
    Node* targetNode;

    Pathfinder();
    
    // Setup the start and end points
    void start(Grid& grid, int startX, int startY, int endX, int endY);
    
    // Process exactly ONE tile (Called 60 times a second)
    void step(Grid& grid); 
    
private:
    int getDistance(Node* nodeA, Node* nodeB);
    void retracePath(Node* startNode, Node* endNode);
};

#endif