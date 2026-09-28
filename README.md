# DSA Pathfinder Engine (A*)

A high-performance A* (A-Star) pathfinding visualization engine written in C++. 

Instead of relying on the C++ Standard Template Library (STL) for algorithm logic, this engine is driven by a custom-built **Min-Heap (Priority Queue)** to demonstrate core Data Structures and Algorithms (DSA) memory management. The graphical front-end is rendered in real-time using SFML, featuring a polished UI and a prebuilt maze library.

## 🌟 Core Features

* **Custom Priority Queue:** The A* algorithm utilizes a custom Min-Heap to continuously evaluate and sort the lowest F-Cost nodes, ensuring $O(\log n)$ extraction time.
* **Prebuilt Maze Library:** Includes a custom file parser that reads and loads complex maze layouts from local `.txt` files.
* **Interactive UI & Smart Brush:** Features a fully clickable top control bar to toggle between Custom and Prebuilt modes, cycle through maze sizes, and randomly generate grids. Use the mouse to smoothly paint or erase walls in real-time.
* **Advanced Rendering:** Uses custom SFML drawing logic to render 3D-beveled stone walls, a checkerboard floor, dynamic heatmap footprints (fading from blue to violet based on search depth), and a glowing final path.

## 🛠️ The Tech Stack

* **Language:** C++17
* **Graphics:** SFML (Simple and Fast Multimedia Library)
* **File I/O:** Standard C++ streams for parsing text-based maze libraries.
* **Build System:** Makefile for streamlined Linux compilation.
* **Architecture:** Modular Object-Oriented Design (`Grid`, `Node`, `MinHeap`, `Pathfinder`, `MazeLibrary`).

## 🚀 How to Run (Linux / Ubuntu)

**1. Install SFML Dependencies**
Ensure you have the SFML development libraries installed on your system:
```bash
sudo apt-get install libsfml-dev