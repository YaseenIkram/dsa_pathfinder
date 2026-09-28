#include "Pathfinder.hpp"
#include <cmath>

Pathfinder::Pathfinder() {
    isSolving = false;
    startNode = nullptr;
    targetNode = nullptr;
}

int Pathfinder::getDistance(Node* nodeA, Node* nodeB) {
    return std::abs(nodeA->x - nodeB->x) + std::abs(nodeA->y - nodeB->y);
}

void Pathfinder::retracePath(Node* startNode, Node* endNode) {
    Node* currentNode = endNode;
    while (currentNode != startNode) {
        currentNode->isPath = true;       
        currentNode = currentNode->parent; 
    }
    startNode->isPath = true;
}

void Pathfinder::start(Grid& grid, int startX, int startY, int endX, int endY) {
    startNode = &grid.table[startY][startX];
    targetNode = &grid.table[endY][endX];
    
    // Reset grid variables
    for (int y = 0; y < grid.rows; y++) {
        for (int x = 0; x < grid.cols; x++) {
            grid.table[y][x].gCost = 9999999;
            grid.table[y][x].isVisited = false;
            grid.table[y][x].isPath = false;
            grid.table[y][x].parent = nullptr;
        }
    }
    
    openSet = MinHeap(); // Clear the heap
    startNode->gCost = 0;
    startNode->hCost = getDistance(startNode, targetNode);
    startNode->fCost = startNode->gCost + startNode->hCost;
    
    openSet.insert(startNode);
    isSolving = true;
}

void Pathfinder::step(Grid& grid) {
    if (!isSolving) return; // Only run if we hit Enter
    
    if (openSet.isEmpty()) {
        isSolving = false; // Maze is impossible to solve
        return;
    }
    
    Node* currentNode = openSet.extractMin();
    currentNode->isVisited = true; // Mark as explored (Colors it Blue)
    
    if (currentNode == targetNode) {
        retracePath(startNode, targetNode);
        isSolving = false; // Stop the animation, we found the end!
        return;
    }
    
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    
    for (int i = 0; i < 4; i++) {
        int nx = currentNode->x + dx[i];
        int ny = currentNode->y + dy[i];
        
        if (nx < 0 || nx >= grid.cols || ny < 0 || ny >= grid.rows) continue;
        
        Node* neighbor = &grid.table[ny][nx];
        if (neighbor->isObstacle || neighbor->isVisited) continue;
        
        int newCostToNeighbor = currentNode->gCost + 1;
        if (newCostToNeighbor < neighbor->gCost) {
            neighbor->gCost = newCostToNeighbor;
            neighbor->hCost = getDistance(neighbor, targetNode);
            neighbor->fCost = neighbor->gCost + neighbor->hCost;
            neighbor->parent = currentNode;
            
            if (!openSet.contains(neighbor)) {
                openSet.insert(neighbor);
            } else {
                openSet.updateNode(neighbor);
            }
        }
    }
}