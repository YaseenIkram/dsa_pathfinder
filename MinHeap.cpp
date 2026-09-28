#include "MinHeap.hpp"
#include <algorithm>

// Multiply and divide by 2 to navigate the tree structure
int MinHeap::parent(int i) { return (i - 1) / 2; }
int MinHeap::leftChild(int i) { return 2 * i + 1; }
int MinHeap::rightChild(int i) { return 2 * i + 2; }

void MinHeap::insert(Node* node) {
    heap.push_back(node);
    heapifyUp(heap.size() - 1); // Bubble the new node up to its correct spot
}

Node* MinHeap::extractMin() {
    if (heap.empty()) return nullptr;
    
    Node* root = heap[0];       // Grab the best node
    heap[0] = heap.back();      // Move the last node to the top
    heap.pop_back();            // Delete the duplicate at the end
    
    if (!heap.empty()) {
        heapifyDown(0);         // Bubble the top node down to its correct spot
    }
    
    return root;
}

void MinHeap::heapifyUp(int i) {
    // While we aren't at the root, and our parent's fCost is worse (higher) than ours
    while (i != 0 && heap[parent(i)]->fCost > heap[i]->fCost) {
        std::swap(heap[i], heap[parent(i)]);
        i = parent(i);
    }
}

void MinHeap::heapifyDown(int i) {
    int smallest = i;
    int left = leftChild(i);
    int right = rightChild(i);
    int size = heap.size();

    // Find the smallest fCost among the parent and its two children
    if (left < size && heap[left]->fCost < heap[smallest]->fCost)
        smallest = left;

    if (right < size && heap[right]->fCost < heap[smallest]->fCost)
        smallest = right;

    // If one of the children was smaller, swap them and keep bubbling down
    if (smallest != i) {
        std::swap(heap[i], heap[smallest]);
        heapifyDown(smallest);
    }
}

bool MinHeap::isEmpty() {
    return heap.empty();
}

bool MinHeap::contains(Node* node) {
    for (Node* n : heap) {
        if (n == node) return true;
    }
    return false;
}

void MinHeap::updateNode(Node* node) {
    // Find the node and bubble it up (in A*, costs only decrease when updated)
    for (size_t i = 0; i < heap.size(); i++) {
        if (heap[i] == node) {
            heapifyUp(i);
            break;
        }
    }
}