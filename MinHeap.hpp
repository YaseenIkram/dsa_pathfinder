#ifndef MINHEAP_HPP
#define MINHEAP_HPP

#include <vector>
#include "Node.hpp"

class MinHeap {
private:
    std::vector<Node*> heap;

    // Helper math to find tree branches inside a flat array
    int parent(int i);
    int leftChild(int i);
    int rightChild(int i);
    
    // Core sorting mechanics
    void heapifyUp(int i);
    void heapifyDown(int i);

public:
    void insert(Node* node);
    Node* extractMin();         // Pops the best node off the top
    bool isEmpty();
    bool contains(Node* node);  // Checks if a tile is already in the queue
    void updateNode(Node* node);// Re-sorts a tile if we find a faster route to it
};

#endif