#ifndef __TILE_AGENT__H
#define __TILE_AGENT__H

#include <string>
#include "enviro.h"

using namespace enviro;

// Legacy tile controller kept for tile-based coloring experiments.
class TileController : public Process, public AgentInterface {

    public:
    // Construct the controller and attach it to Enviro's process system.
    TileController() : Process(), AgentInterface() {}

    // Register tile paint events.
    void init();
    // Initialize tile coordinates and appearance.
    void start();
    // Keep the tile still each frame.
    void update();
    // Clean shutdown hook for the tile.
    void stop();

    private:
    // Update the tile color based on the current owner.
    void set_owner(int owner);
    // Compute the tile row from world position.
    int row_from_position();
    // Compute the tile column from world position.
    int col_from_position();
    // Build a simple SVG rectangle for the tile.
    std::string tile_svg(const std::string& fill, const std::string& stroke);

    // Cached tile grid position and owner state.
    int row = -1;
    int col = -1;
    int current_owner = 0;
    double home_x = 0.0;
    double home_y = 0.0;

    // Board layout constants shared with the game manager.
    static constexpr double TILE_SIZE = 40.0;
    static constexpr double BOARD_LEFT = -300.0;
    static constexpr double BOARD_TOP = 160.0;
};

// Enviro agent wrapper for a tile.
class Tile : public Agent {
    public:
    // Attach the controller process to the agent.
    Tile(json spec, World& world) : Agent(spec, world) {
        add_process(c);
    }

    private:
    TileController c;
};

DECLARE_INTERFACE(Tile)

#endif