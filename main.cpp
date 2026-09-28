#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>
#include "Grid.hpp"
#include "Pathfinder.hpp"
#include "MazeLibrary.hpp"

// ---------------------------------------------------------------
// Layout constants
// ---------------------------------------------------------------
namespace {

    const float WINDOW_W = 800.0f;
    const float WINDOW_H = 700.0f;
    const float TOP_H    = 84.0f;   // tab bar (row 1) + control strip (row 2)
    const float MARGIN   = 30.0f;   // space around the grid frame

    enum class Mode { Custom, Prebuilt };

    // The program can be launched from an IDE or from a terminal, so
    // the working directory differs. Try a few likely locations.
    bool loadLibrary(MazeLibrary& lib, std::string& usedPath) {
        const char* candidates[] = {
            "mazes.txt", "../mazes.txt", "../../mazes.txt", "assets/mazes.txt"
        };
        for (const char* p : candidates) {
            if (lib.load(p)) {
                usedPath = p;
                return true;
            }
        }
        return false;
    }

    // Tab labels need a font. Try a local copy first, then common
    // system locations on Windows / Linux / macOS.
    bool loadAnyFont(sf::Font& font) {
        const char* candidates[] = {
            "font.ttf", "assets/font.ttf",
            "C:/Windows/Fonts/segoeui.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/System/Library/Fonts/Supplemental/Arial.ttf",
            "/Library/Fonts/Arial.ttf"
        };
        for (const char* p : candidates) {
            if (font.loadFromFile(p)) return true;
        }
        return false;
    }
}

int main() {

    // ---------------------------------------------------------
    // 1. INPUT WITH CRASH PROTECTION (size for the CUSTOM tab)
    // ---------------------------------------------------------

    int customSize = 0;

    std::cout << "Enter grid size for the Custom tab (e.g., 5 for 5x5, 40 for 40x40): ";

    if (!(std::cin >> customSize) || customSize <= 0) {
        std::cin.clear();
        customSize = 20;
        std::cout << "Invalid input. Defaulting to 20x20.\n";
    }

    // ---------------------------------------------------------
    // 2. LOAD THE PREBUILT MAZE FILE
    // ---------------------------------------------------------

    MazeLibrary library;
    std::string libraryPath;

    bool libraryOk = loadLibrary(library, libraryPath) && library.total() > 0;

    if (libraryOk) {
        std::cout << "Loaded " << library.total() << " mazes from '"
                  << libraryPath << "'";
        if (library.skipped() > 0)
            std::cout << " (" << library.skipped() << " malformed block(s) skipped)";
        std::cout << ".\n";
    } else {
        std::cout << "Could not load mazes.txt - place it next to the "
                     "executable. Prebuilt tab disabled.\n";
    }

    const std::vector<int> sizes = library.sizes();

    sf::Font font;
    bool fontOk = loadAnyFont(font);
    if (!fontOk) {
        std::cout << "No font found - copy any .ttf to 'font.ttf' next to the "
                     "executable to see button labels.\n";
    }

    std::cout << "\n--- DSA PATHFINDER CONTROLS ---\n";
    std::cout << "Tabs (top)        : Custom Maze / Prebuilt Mazes (Tab key toggles)\n";
    std::cout << "Left Click + Drag : Draw walls or Erase walls (Smart Toggle)\n";
    std::cout << "Enter             : Start Algorithm\n";
    std::cout << "R                 : Reset board (Prebuilt: reload the maze)\n";
    std::cout << "Prebuilt tab      : Left/Right = prev/next maze, Up/Down = size\n\n";

    // ---------------------------------------------------------
    // 3. WINDOW
    // ---------------------------------------------------------

    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;

    sf::RenderWindow window(
        sf::VideoMode(
            static_cast<unsigned int>(WINDOW_W),
            static_cast<unsigned int>(WINDOW_H)
        ),
        "Maze Runner - A* Pathfinder",
        sf::Style::Titlebar | sf::Style::Close,
        settings
    );

    window.setFramerateLimit(60);

    // ---------------------------------------------------------
    // 4. GRID DIMENSIONS
    // ---------------------------------------------------------

    // Largest square that fits under the tab bar, inside the margins.
    const float drawableArea = std::min(
        WINDOW_W - 2.0f * MARGIN,
        WINDOW_H - TOP_H - 2.0f * MARGIN
    );

    int   gridSize = customSize;
    float nodeSize = drawableArea / static_cast<float>(gridSize);

    Grid grid(gridSize, gridSize, nodeSize, MARGIN);
    Pathfinder pathfinder;

    // The Grid class draws starting at (MARGIN, MARGIN). This view slides
    // that picture below the tab bar and centres it horizontally, so Grid
    // needs no changes. Passing this view to mapPixelToCoords() makes the
    // mouse line up with the shifted grid automatically.
    sf::View gridView(sf::FloatRect(
        MARGIN - (WINDOW_W - drawableArea) / 2.0f,
        -TOP_H,
        WINDOW_W,
        WINDOW_H
    ));

    // ---------------------------------------------------------
    // 5. UI STATE
    // ---------------------------------------------------------

    Mode mode = Mode::Custom;

    bool isMouseDragging = false;
    bool brushState = true;          // true = draw wall, false = erase

    // Prebuilt selection: index into `sizes`, and maze number for that size
    int sizeIdx = 0;
    for (size_t i = 0; i < sizes.size(); i++) {           // start near customSize
        sizeIdx = static_cast<int>(i);
        if (sizes[i] >= customSize) break;
    }
    int mazeIdx = 0;

    // Walls drawn in Custom mode are remembered while browsing prebuilt.
    std::vector<std::vector<char>> savedWalls;

    std::mt19937 rng{ std::random_device{}() };

    // Button rectangles (pixel coordinates in the window)
    const sf::FloatRect tabCustom (24.f,   6.f, 180.f, 32.f);
    const sf::FloatRect tabPrebuilt(212.f, 6.f, 200.f, 32.f);
    const sf::FloatRect sizeDown   (24.f,  46.f, 32.f, 30.f);
    const sf::FloatRect sizeUp     (196.f, 46.f, 32.f, 30.f);
    const sf::FloatRect mazeDown   (250.f, 46.f, 32.f, 30.f);
    const sf::FloatRect mazeUp     (522.f, 46.f, 32.f, 30.f);
    const sf::FloatRect randomBtn  (576.f, 46.f, 90.f, 30.f);

    // ---------------------------------------------------------
    // 6. HELPER ACTIONS
    // ---------------------------------------------------------

    // Wipe search results (visited/path) but keep the walls.
    auto clearSearch = [&]() {
        pathfinder.isSolving = false;
        for (int y = 0; y < grid.rows; y++) {
            for (int x = 0; x < grid.cols; x++) {
                grid.table[y][x].isVisited = false;
                grid.table[y][x].isPath = false;
                grid.table[y][x].parent = nullptr;
            }
        }
    };

    // Brand-new empty grid of the given size (nodeSize follows the size).
    auto rebuild = [&](int size) {
        pathfinder.isSolving = false;
        gridSize = size;
        nodeSize = drawableArea / static_cast<float>(size);
        grid = Grid(size, size, nodeSize, MARGIN);
    };

    // Copy the currently selected maze from the library into the grid.
    auto loadPrebuilt = [&]() {
        if (sizes.empty()) return;

        const int size = sizes[sizeIdx];
        rebuild(size);

        const MazeData* maze = library.get(size, mazeIdx);
        if (!maze) return;

        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                grid.table[y][x].isObstacle = (maze->rows[y][x] == '1');
            }
        }

        // Start and goal must always be open.
        grid.table[0][0].isObstacle = false;
        grid.table[size - 1][size - 1].isObstacle = false;
    };

    auto switchToPrebuilt = [&]() {
        if (mode == Mode::Prebuilt || !libraryOk) return;

        // Remember the custom drawing before we replace the grid.
        savedWalls.assign(gridSize, std::vector<char>(gridSize, 0));
        for (int y = 0; y < gridSize; y++)
            for (int x = 0; x < gridSize; x++)
                savedWalls[y][x] = grid.table[y][x].isObstacle ? 1 : 0;

        mode = Mode::Prebuilt;
        loadPrebuilt();
    };

    auto switchToCustom = [&]() {
        if (mode == Mode::Custom) return;

        mode = Mode::Custom;
        rebuild(customSize);

        if (static_cast<int>(savedWalls.size()) == customSize) {
            for (int y = 0; y < customSize; y++)
                for (int x = 0; x < customSize; x++)
                    grid.table[y][x].isObstacle = (savedWalls[y][x] != 0);
        }
    };

    auto changeSize = [&](int delta) {
        if (mode != Mode::Prebuilt) return;
        int next = sizeIdx + delta;
        if (next < 0 || next >= static_cast<int>(sizes.size())) return;
        sizeIdx = next;
        mazeIdx = 0;
        loadPrebuilt();
    };

    auto changeMaze = [&](int delta) {
        if (mode != Mode::Prebuilt) return;
        int n = library.count(sizes[sizeIdx]);
        if (n == 0) return;
        mazeIdx = ((mazeIdx + delta) % n + n) % n;   // wraps both ways
        loadPrebuilt();
    };

    auto randomMaze = [&]() {
        if (mode != Mode::Prebuilt) return;
        int n = library.count(sizes[sizeIdx]);
        if (n == 0) return;
        mazeIdx = static_cast<int>(rng() % static_cast<unsigned>(n));
        loadPrebuilt();
    };

    auto resetBoard = [&]() {
        if (mode == Mode::Prebuilt) {
            loadPrebuilt();                // reload the pristine maze
        } else {
            rebuild(customSize);
            savedWalls.clear();
        }
    };

    // ---------------------------------------------------------
    // 7. UI DRAWING HELPERS
    // ---------------------------------------------------------

    auto drawText = [&](const std::string& s, float x, float cy,
                        unsigned charSize, sf::Color color, bool centered) {
        if (!fontOk) return;
        sf::Text t(s, font, charSize);
        t.setFillColor(color);
        sf::FloatRect b = t.getLocalBounds();
        t.setOrigin(centered ? b.left + b.width / 2.0f : b.left,
                    b.top + b.height / 2.0f);
        t.setPosition(x, cy);
        window.draw(t);
    };

    auto drawButton = [&](const sf::FloatRect& r, const std::string& label,
                          bool active, bool enabled, bool hover) {
        sf::RectangleShape box(sf::Vector2f(r.width, r.height));
        box.setPosition(r.left, r.top);

        sf::Color fill(38, 42, 52);
        if (!enabled)      fill = sf::Color(27, 29, 35);
        else if (active)   fill = sf::Color(45, 120, 200);
        else if (hover)    fill = sf::Color(55, 60, 74);

        box.setFillColor(fill);
        box.setOutlineThickness(1.0f);
        box.setOutlineColor(sf::Color(70, 74, 84));
        window.draw(box);

        drawText(label, r.left + r.width / 2.0f, r.top + r.height / 2.0f, 15,
                 enabled ? sf::Color::White : sf::Color(90, 94, 104), true);
    };

    // ---------------------------------------------------------
    // 8. MAIN LOOP
    // ---------------------------------------------------------

    while (window.isOpen()) {

        sf::Event event;

        while (window.pollEvent(event)) {

            if (event.type == sf::Event::Closed) {
                window.close();
            }

            // -------------------------------------------------
            // MOUSE PRESSED
            // -------------------------------------------------

            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {

                const float mx = static_cast<float>(event.mouseButton.x);
                const float my = static_cast<float>(event.mouseButton.y);

                // ---- click landed on the tab bar / control strip ----
                if (my < TOP_H) {

                    if (tabCustom.contains(mx, my))         switchToCustom();
                    else if (tabPrebuilt.contains(mx, my))  switchToPrebuilt();
                    else if (mode == Mode::Prebuilt) {
                        if (sizeDown.contains(mx, my))       changeSize(-1);
                        else if (sizeUp.contains(mx, my))    changeSize(+1);
                        else if (mazeDown.contains(mx, my))  changeMaze(-1);
                        else if (mazeUp.contains(mx, my))    changeMaze(+1);
                        else if (randomBtn.contains(mx, my)) randomMaze();
                    }
                }

                // ---- click landed on the grid area ----
                else {

                    sf::Vector2f worldPos = window.mapPixelToCoords(
                        sf::Vector2i(event.mouseButton.x, event.mouseButton.y),
                        gridView
                    );

                    // floor() so clicks just above/left of the grid are
                    // not truncated toward cell 0.
                    int gridX = static_cast<int>(
                        std::floor((worldPos.x - MARGIN) / nodeSize));
                    int gridY = static_cast<int>(
                        std::floor((worldPos.y - MARGIN) / nodeSize));

                    if (gridX >= 0 && gridX < grid.cols &&
                        gridY >= 0 && gridY < grid.rows) {

                        isMouseDragging = true;

                        // Clicked a wall -> eraser, empty -> pen.
                        brushState = !grid.table[gridY][gridX].isObstacle;

                        grid.setObstacle(worldPos.x, worldPos.y, brushState);

                        // Auto-clear old path when the maze is edited
                        if (pathfinder.isSolving || grid.table[0][0].isVisited) {
                            clearSearch();
                        }
                    }
                }
            }

            if (event.type == sf::Event::MouseButtonReleased &&
                event.mouseButton.button == sf::Mouse::Left) {
                isMouseDragging = false;
            }

            // -------------------------------------------------
            // KEYBOARD
            // -------------------------------------------------

            if (event.type == sf::Event::KeyPressed) {

                switch (event.key.code) {

                    case sf::Keyboard::Enter:
                        pathfinder.start(grid, 0, 0, gridSize - 1, gridSize - 1);
                        break;

                    case sf::Keyboard::R:
                        resetBoard();
                        break;

                    case sf::Keyboard::Tab:
                        if (mode == Mode::Custom) switchToPrebuilt();
                        else                      switchToCustom();
                        break;

                    case sf::Keyboard::Left:  changeMaze(-1); break;
                    case sf::Keyboard::Right: changeMaze(+1); break;
                    case sf::Keyboard::Up:    changeSize(+1); break;
                    case sf::Keyboard::Down:  changeSize(-1); break;

                    default: break;
                }
            }
        }

        // ---------------------------------------------------------
        // MOUSE DRAGGING (paint walls while button is held)
        // ---------------------------------------------------------

        if (isMouseDragging) {
            sf::Vector2f worldPos = window.mapPixelToCoords(
                sf::Mouse::getPosition(window), gridView);
            grid.setObstacle(worldPos.x, worldPos.y, brushState);
        }

        // ---------------------------------------------------------
        // PATHFINDER STEP
        // ---------------------------------------------------------

        pathfinder.step(grid);

        // ---------------------------------------------------------
        // DRAW
        // ---------------------------------------------------------

        window.clear(sf::Color(15, 16, 20));

        // Grid (shifted below the tab bar by gridView)
        window.setView(gridView);
        grid.draw(window);

        // UI (normal pixel coordinates)
        window.setView(window.getDefaultView());

        sf::RectangleShape bar(sf::Vector2f(WINDOW_W, TOP_H));
        bar.setFillColor(sf::Color(22, 24, 30));
        window.draw(bar);

        sf::RectangleShape line(sf::Vector2f(WINDOW_W, 2.0f));
        line.setPosition(0.0f, TOP_H - 2.0f);
        line.setFillColor(sf::Color(55, 58, 66));
        window.draw(line);

        sf::Vector2i mp = sf::Mouse::getPosition(window);
        const float mpx = static_cast<float>(mp.x);
        const float mpy = static_cast<float>(mp.y);

        // Row 1: tabs + library status
        drawButton(tabCustom, "Custom Maze", mode == Mode::Custom, true,
                   tabCustom.contains(mpx, mpy));
        drawButton(tabPrebuilt, "Prebuilt Mazes", mode == Mode::Prebuilt,
                   libraryOk, tabPrebuilt.contains(mpx, mpy));

        if (libraryOk) {
            drawText(std::to_string(library.total()) + " mazes loaded",
                     440.0f, 22.0f, 14, sf::Color(130, 135, 148), false);
        } else {
            drawText("mazes.txt not found", 440.0f, 22.0f, 14,
                     sf::Color(240, 90, 90), false);
        }

        // Row 2: controls
        if (mode == Mode::Custom) {

            drawText("Drag to draw/erase walls   |   Enter: solve   |   "
                     "R: clear   |   Tab: prebuilt",
                     24.0f, 61.0f, 14, sf::Color(150, 155, 165), false);

        } else {

            const int size = sizes[sizeIdx];
            const int n    = library.count(size);
            const MazeData* m = library.get(size, mazeIdx);

            drawButton(sizeDown, "<", false, sizeIdx > 0,
                       sizeDown.contains(mpx, mpy));
            drawText("Size " + std::to_string(size) + " x " + std::to_string(size),
                     126.0f, 61.0f, 15, sf::Color::White, true);
            drawButton(sizeUp, ">", false,
                       sizeIdx + 1 < static_cast<int>(sizes.size()),
                       sizeUp.contains(mpx, mpy));

            drawButton(mazeDown, "<", false, n > 1, mazeDown.contains(mpx, mpy));
            drawText(std::to_string(mazeIdx + 1) + "/" + std::to_string(n) +
                     "   " + (m ? m->name : std::string("-")),
                     402.0f, 61.0f, 15, sf::Color::White, true);
            drawButton(mazeUp, ">", false, n > 1, mazeUp.contains(mpx, mpy));

            drawButton(randomBtn, "Random", false, n > 1,
                       randomBtn.contains(mpx, mpy));
        }

        window.display();
    }

    return 0;
}