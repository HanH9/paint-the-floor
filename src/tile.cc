#include "tile.h"

using namespace enviro;

// Register paint events for this tile.
void TileController::init() {

    watch("paint_tile", [this](Event& e) {
        int target_row = e.value()["row"];
        int target_col = e.value()["col"];
        int owner = e.value()["owner"];

        if ( target_row == row && target_col == col ) {
            set_owner(owner);
        }
    });
}

// Cache the tile grid position and initialize its appearance.
void TileController::start() {
    row = row_from_position();
    col = col_from_position();
    home_x = x();
    home_y = y();
    current_owner = -1;
    label("", 0, 0);
    set_owner(0);
}

// Keep the tile fixed in place.
void TileController::update() {
    damp_movement();
    track_velocity(0, 0);
}

// No special shutdown work is needed for tiles.
void TileController::stop() {}

// Convert the tile's current world position to a board row index.
int TileController::row_from_position() {
    return static_cast<int>((BOARD_TOP - y() + TILE_SIZE / 2.0) / TILE_SIZE);
}

// Convert the tile's current world position to a board column index.
int TileController::col_from_position() {
    return static_cast<int>((x() - BOARD_LEFT + TILE_SIZE / 2.0) / TILE_SIZE);
}

// Build a simple SVG rectangle for the tile.
std::string TileController::tile_svg(const std::string& fill, const std::string& stroke) {
    return "<rect x='-16' y='-16' width='32' height='32' "
           "style='fill:" + fill + ";stroke:" + stroke + ";stroke-width:1' />";
}

// Update the tile color to match its current owner value.
void TileController::set_owner(int owner) {
    if ( owner == current_owner ) {
        return;
    }

    current_owner = owner;

    if ( owner == 1 ) {
        decorate(tile_svg("#5DADE2", "#2E86C1"));
    } else if ( owner == 2 ) {
        decorate(tile_svg("#EC7063", "#CB4335"));
    } else {
        decorate(tile_svg("#EAECEE", "#BDC3C7"));
    }
}
