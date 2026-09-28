#ifndef NODE_HPP
#define NODE_HPP

struct Node {
    int x, y;               // Grid coordinates
    
    bool isObstacle;        // True if the user drew a wall here
    bool isVisited;         // True if the algorithm has already checked it
    bool isPath;            // True if it is part of the final successful route
    
    int gCost;              // Distance from the starting point
    int hCost;              // Estimated distance to the target
    int fCost;              // Total score (gCost + hCost)
    
    Node* parent;           // Points back to the previous node to trace the final path

    // Constructor to set default values when a Node is first created
    Node(int startX = 0, int startY = 0) {
        x = startX;
        y = startY;
        isObstacle = false;
        isVisited = false;
        isPath = false;
        gCost = 0;
        hCost = 0;
        fCost = 0;
        parent = nullptr;
    }
};

#endif