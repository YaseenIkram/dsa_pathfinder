#include "Grid.hpp"
#include <cmath>
#include <algorithm>

// -----------------------------------------------------------------
// Small local helpers (UI only — no algorithm/logic changes below)
// -----------------------------------------------------------------

namespace {

    // Deterministic per-cell "noise" so stone blocks don't look
    // perfectly uniform (cheap fake texture, no assets needed).
    int cellNoise(int x, int y) {
        int hash = (x * 928371 + y * 689287) % 17;
        return (hash % 7) - 3; // roughly -3..+3
    }

    sf::Color shade(sf::Color base, int delta) {
        auto clamp255 = [](int v) {
            return static_cast<sf::Uint8>(
                std::min(255, std::max(0, v))
            );
        };
        return sf::Color(
            clamp255(base.r + delta),
            clamp255(base.g + delta),
            clamp255(base.b + delta),
            base.a
        );
    }

    sf::Color lerpColor(sf::Color a, sf::Color b, float t) {
        t = std::min(1.0f, std::max(0.0f, t));
        return sf::Color(
            static_cast<sf::Uint8>(a.r + (b.r - a.r) * t),
            static_cast<sf::Uint8>(a.g + (b.g - a.g) * t),
            static_cast<sf::Uint8>(a.b + (b.b - a.b) * t),
            static_cast<sf::Uint8>(a.a + (b.a - a.a) * t)
        );
    }

}


Grid::Grid(int r, int c, float size, float m) {
    rows = r;
    cols = c;
    nodeSize = size;
    margin = m;

    table.resize(rows, std::vector<Node>(cols));

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            table[y][x] = Node(x, y);
        }
    }
}

void Grid::draw(sf::RenderWindow& window) {

    const float gridPixelW = nodeSize * cols;
    const float gridPixelH = nodeSize * rows;

    // ---------------------------------------------------------
    // PASS 0: Frame the maze like a plaque / stone border
    // ---------------------------------------------------------
    {
        sf::RectangleShape outerFrame;
        outerFrame.setPosition(margin - 10.0f, margin - 10.0f);
        outerFrame.setSize(
            sf::Vector2f(gridPixelW + 20.0f, gridPixelH + 20.0f)
        );
        outerFrame.setFillColor(sf::Color::Transparent);
        outerFrame.setOutlineThickness(4.0f);
        outerFrame.setOutlineColor(sf::Color(70, 74, 84));
        window.draw(outerFrame);

        sf::RectangleShape innerFrame;
        innerFrame.setPosition(margin - 4.0f, margin - 4.0f);
        innerFrame.setSize(
            sf::Vector2f(gridPixelW + 8.0f, gridPixelH + 8.0f)
        );
        innerFrame.setFillColor(sf::Color::Transparent);
        innerFrame.setOutlineThickness(2.0f);
        innerFrame.setOutlineColor(sf::Color(140, 145, 155));
        window.draw(innerFrame);
    }

    // ---------------------------------------------------------
    // PASS 1: Draw the maze layout (floor + walls)
    // ---------------------------------------------------------

    for (int y = 0; y < rows; y++) {

        for (int x = 0; x < cols; x++) {

            Node& currentSquare = table[y][x];

            /*
                Calculate the exact boundaries of this cell.

                Instead of repeatedly relying on:

                    margin + y * nodeSize

                we calculate both edges directly.

                This prevents small floating-point errors from
                accumulating across many rows.
            */

            float left =
                margin +
                (static_cast<float>(x) / cols) *
                (nodeSize * cols);

            float right =
                margin +
                (static_cast<float>(x + 1) / cols) *
                (nodeSize * cols);

            float top =
                margin +
                (static_cast<float>(y) / rows) *
                (nodeSize * rows);

            float bottom =
                margin +
                (static_cast<float>(y + 1) / rows) *
                (nodeSize * rows);


            float width = right - left;
            float height = bottom - top;


            sf::RectangleShape cell;

            cell.setPosition(left, top);
            cell.setSize(sf::Vector2f(width, height));


            // -------------------------------------------------
            // WALL — beveled stone block
            // -------------------------------------------------

            if (currentSquare.isObstacle) {

                sf::Color base = shade(
                    sf::Color(100, 105, 115),
                    cellNoise(x, y)
                );

                cell.setFillColor(base);
                window.draw(cell);

                // Only bevel the edges that border a non-wall
                // (open floor or outside the grid), so adjacent
                // wall cells fuse into one continuous block
                // instead of looking like separate tiles.

                bool wallUp    = (y == 0)        || table[y - 1][x].isObstacle;
                bool wallDown  = (y == rows - 1)  || table[y + 1][x].isObstacle;
                bool wallLeft  = (x == 0)        || table[y][x - 1].isObstacle;
                bool wallRight = (x == cols - 1)  || table[y][x + 1].isObstacle;

                float bevel = std::max(2.0f, nodeSize * 0.12f);

                sf::Color highlight = shade(base, 40);
                sf::Color shadow    = shade(base, -40);

                if (!wallUp) {
                    sf::RectangleShape edge(sf::Vector2f(width, bevel));
                    edge.setPosition(left, top);
                    edge.setFillColor(highlight);
                    window.draw(edge);
                }
                if (!wallLeft) {
                    sf::RectangleShape edge(sf::Vector2f(bevel, height));
                    edge.setPosition(left, top);
                    edge.setFillColor(highlight);
                    window.draw(edge);
                }
                if (!wallDown) {
                    sf::RectangleShape edge(sf::Vector2f(width, bevel));
                    edge.setPosition(left, bottom - bevel);
                    edge.setFillColor(shadow);
                    window.draw(edge);
                }
                if (!wallRight) {
                    sf::RectangleShape edge(sf::Vector2f(bevel, height));
                    edge.setPosition(right - bevel, top);
                    edge.setFillColor(shadow);
                    window.draw(edge);
                }
            }

            // -------------------------------------------------
            // EMPTY CELL — soft checkerboard floor
            // -------------------------------------------------

            else {

                bool alt = ((x + y) % 2 == 0);

                cell.setFillColor(
                    alt ? sf::Color(30, 33, 39) : sf::Color(26, 29, 35)
                );

                cell.setOutlineThickness(-1.0f);

                cell.setOutlineColor(
                    sf::Color(45, 48, 55, 120)
                );

                window.draw(cell);
            }
        }
    }


    // ---------------------------------------------------------
    // PASS 2: Draw visited nodes (heatmap trail) and final path
    // ---------------------------------------------------------

    // Rough normalizer for how "far" a visited node is from the
    // start, purely to colour the search trail — not used for
    // any pathfinding logic.
    const float maxSpread = static_cast<float>(rows + cols);

    for (int y = 0; y < rows; y++) {

        for (int x = 0; x < cols; x++) {

            Node& currentSquare = table[y][x];


            // Center of current node
            float centerX =
                margin +
                (x * nodeSize) +
                (nodeSize / 2.0f);

            float centerY =
                margin +
                (y * nodeSize) +
                (nodeSize / 2.0f);


            // -------------------------------------------------
            // FINAL PATH — glowing trail
            // -------------------------------------------------

            if (currentSquare.isPath) {

                if (currentSquare.parent != nullptr) {

                    float px =
                        margin +
                        (currentSquare.parent->x * nodeSize) +
                        (nodeSize / 2.0f);

                    float py =
                        margin +
                        (currentSquare.parent->y * nodeSize) +
                        (nodeSize / 2.0f);


                    bool vertical =
                        (currentSquare.x == currentSquare.parent->x);

                    sf::Vector2f glowSize = vertical
                        ? sf::Vector2f(nodeSize * 0.7f, nodeSize)
                        : sf::Vector2f(nodeSize, nodeSize * 0.7f);

                    sf::Vector2f lineSize = vertical
                        ? sf::Vector2f(nodeSize / 3.0f, nodeSize)
                        : sf::Vector2f(nodeSize, nodeSize / 3.0f);

                    sf::Vector2f pos = vertical
                        ? sf::Vector2f(centerX, (centerY + py) / 2.0f)
                        : sf::Vector2f((centerX + px) / 2.0f, centerY);

                    // Soft glow behind the path line
                    sf::RectangleShape glow;
                    glow.setSize(glowSize);
                    glow.setOrigin(glowSize.x / 2.0f, glowSize.y / 2.0f);
                    glow.setPosition(pos);
                    glow.setFillColor(sf::Color(80, 250, 120, 60));
                    window.draw(glow);

                    // Crisp line on top
                    sf::RectangleShape pathLine;
                    pathLine.setSize(lineSize);
                    pathLine.setOrigin(lineSize.x / 2.0f, lineSize.y / 2.0f);
                    pathLine.setPosition(pos);
                    pathLine.setFillColor(sf::Color(80, 250, 120));
                    window.draw(pathLine);
                }


                // Path node glow
                sf::CircleShape glowDot(nodeSize / 2.2f);
                glowDot.setOrigin(nodeSize / 2.2f, nodeSize / 2.2f);
                glowDot.setPosition(centerX, centerY);
                glowDot.setFillColor(sf::Color(80, 250, 120, 50));
                window.draw(glowDot);

                // Path node
                sf::RectangleShape dot(
                    sf::Vector2f(
                        nodeSize / 3.0f,
                        nodeSize / 3.0f
                    )
                );

                dot.setOrigin(
                    nodeSize / 6.0f,
                    nodeSize / 6.0f
                );

                dot.setPosition(
                    centerX,
                    centerY
                );

                dot.setFillColor(
                    sf::Color(80, 250, 120)
                );

                window.draw(dot);
            }


            // -------------------------------------------------
            // VISITED NODE — coloured by search "distance"
            // -------------------------------------------------

            else if (currentSquare.isVisited) {

                float t = 0.0f;
                if (currentSquare.gCost < 9999999) {
                    t = static_cast<float>(currentSquare.gCost) / maxSpread;
                }

                sf::Color trailColor = lerpColor(
                    sf::Color(45, 120, 200),   // near start: blue
                    sf::Color(170, 80, 220),   // far out: violet
                    t
                );

                sf::CircleShape dot(nodeSize / 4.0f);

                dot.setOrigin(
                    nodeSize / 4.0f,
                    nodeSize / 4.0f
                );

                dot.setPosition(
                    centerX,
                    centerY
                );

                dot.setFillColor(trailColor);

                window.draw(dot);
            }
        }
    }


    // ---------------------------------------------------------
    // PASS 3: Start and goal markers (always drawn on top)
    // ---------------------------------------------------------

    auto drawMarker = [&](int gx, int gy, sf::Color color, bool isGoal) {

        float centerX = margin + (gx * nodeSize) + (nodeSize / 2.0f);
        float centerY = margin + (gy * nodeSize) + (nodeSize / 2.0f);

        // Soft halo
        sf::CircleShape halo(nodeSize / 1.8f);
        halo.setOrigin(nodeSize / 1.8f, nodeSize / 1.8f);
        halo.setPosition(centerX, centerY);
        halo.setFillColor(sf::Color(color.r, color.g, color.b, 60));
        window.draw(halo);

        if (!isGoal) {
            // Start: bullseye marker
            sf::CircleShape outer(nodeSize / 3.0f);
            outer.setOrigin(nodeSize / 3.0f, nodeSize / 3.0f);
            outer.setPosition(centerX, centerY);
            outer.setFillColor(sf::Color::White);
            window.draw(outer);

            sf::CircleShape inner(nodeSize / 4.5f);
            inner.setOrigin(nodeSize / 4.5f, nodeSize / 4.5f);
            inner.setPosition(centerX, centerY);
            inner.setFillColor(color);
            window.draw(inner);
        } else {
            // Goal: diamond / flag-like marker
            sf::RectangleShape diamond(
                sf::Vector2f(nodeSize / 2.0f, nodeSize / 2.0f)
            );
            diamond.setOrigin(nodeSize / 4.0f, nodeSize / 4.0f);
            diamond.setPosition(centerX, centerY);
            diamond.setRotation(45.0f);
            diamond.setFillColor(sf::Color::White);
            window.draw(diamond);

            sf::RectangleShape diamondInner(
                sf::Vector2f(nodeSize / 3.0f, nodeSize / 3.0f)
            );
            diamondInner.setOrigin(nodeSize / 6.0f, nodeSize / 6.0f);
            diamondInner.setPosition(centerX, centerY);
            diamondInner.setRotation(45.0f);
            diamondInner.setFillColor(color);
            window.draw(diamondInner);
        }
    };

    drawMarker(0, 0, sf::Color(80, 250, 120), false);          // start
    drawMarker(cols - 1, rows - 1, sf::Color(240, 90, 90), true); // goal
}


void Grid::setObstacle(
    float mouseX,
    float mouseY,
    bool state
) {

    int gridX =
        static_cast<int>(
            (mouseX - margin) / nodeSize
        );

    int gridY =
        static_cast<int>(
            (mouseY - margin) / nodeSize
        );


    if (
        gridX >= 0 &&
        gridX < cols &&
        gridY >= 0 &&
        gridY < rows
    ) {

        // Don't allow start/end nodes to become walls
        if (
            (gridX == 0 && gridY == 0) ||
            (gridX == cols - 1 &&
             gridY == rows - 1)
        ) {
            return;
        }

        table[gridY][gridX].isObstacle = state;
    }
}