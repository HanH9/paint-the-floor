#include <cmath>
#include <sstream>
#include "blue_bot.h"

using namespace enviro;

// Register keyboard input and round-related events for the player bot.
void BlueBotController::init() {

    watch("keydown", [this](Event& e) {
        std::string key = e.value()["key"];

        if ( key == "w" || key == "W" ) {
            w_down = true;
        } else if ( key == "a" || key == "A" ) {
            a_down = true;
        } else if ( key == "s" || key == "S" ) {
            s_down = true;
        } else if ( key == "d" || key == "D" ) {
            d_down = true;
        }
    });

    watch("keyup", [this](Event& e) {
        std::string key = e.value()["key"];

        if ( key == "w" || key == "W" ) {
            w_down = false;
        } else if ( key == "a" || key == "A" ) {
            a_down = false;
        } else if ( key == "s" || key == "S" ) {
            s_down = false;
        } else if ( key == "d" || key == "D" ) {
            d_down = false;
        }
    });

    watch("freeze_bot", [this](Event& e) {
        std::string owner = e.value()["owner"];
        if ( owner == "blue" ) {
            int ms = e.value()["ms"];
            frozen_until = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
        }
    });

    watch("round_state", [this](Event& e) {
        round_running = e.value()["running"];
        if ( !round_running ) {
            omni_track_velocity(0, 0);
        }
    });

    watch("reset_blue", [this](Event& e) {
        double x = e.value()["x"];
        double y = e.value()["y"];
        double theta = e.value()["theta"];

        teleport(x, y, theta);

        w_down = false;
        a_down = false;
        s_down = false;
        d_down = false;

        heading_dx = 0.0;
        heading_dy = -1.0;

        last_row = -999;
        last_col = -999;
        frozen_until = std::chrono::steady_clock::now();

        draw_body();
    });
}

// Initialize the visible state of the player bot at the start of a round.
void BlueBotController::start() {
    label("", 0, 0);
    prevent_rotation();
    round_running = true;
    frozen_until = std::chrono::steady_clock::now();
    heading_dx = 0.0;
    heading_dy = -1.0;
    draw_body();
}

// Advance the player bot each frame.
void BlueBotController::update() {
    update_motion();
    emit_pose();

    if ( round_running ) {
        claim_current_tile();
    }
}

// No special shutdown work is needed for the player bot.
void BlueBotController::stop() {}

// Draw a square player body with a heading line to show movement direction.
void BlueBotController::draw_body() {
    double line_len = 11.0;
    double x2 = heading_dx * line_len;
    double y2 = heading_dy * line_len;

    std::ostringstream out;
    out << "<rect x='-10' y='-10' width='20' height='20' "
        << "style='fill:#4A90E2;stroke:black;stroke-width:1.5' />"
        << "<line x1='0' y1='0' x2='" << x2 << "' y2='" << y2
        << "' style='stroke:black;stroke-width:2' />";

    decorate(out.str());
}

// Move the player using omni-directional velocity based on current key input.
void BlueBotController::update_motion() {
    if ( !round_running ) {
        omni_track_velocity(0, 0);
        return;
    }

    if ( std::chrono::steady_clock::now() < frozen_until ) {
        omni_track_velocity(0, 0);
        return;
    }

    double vx = 0.0;
    double vy = 0.0;

    if ( w_down && !s_down ) {
        vy -= move_speed;
    } else if ( s_down && !w_down ) {
        vy += move_speed;
    }

    if ( a_down && !d_down ) {
        vx -= move_speed;
    } else if ( d_down && !a_down ) {
        vx += move_speed;
    }

    if ( vx == 0.0 && vy == 0.0 ) {
        omni_track_velocity(0, 0, 12);
        return;
    }

    double len = std::sqrt(vx * vx + vy * vy);
    if ( len > move_speed ) {
        vx = vx / len * move_speed;
        vy = vy / len * move_speed;
    }

    heading_dx = vx / move_speed;
    heading_dy = vy / move_speed;
    draw_body();

    omni_track_velocity(vx, vy, 12);
}

// Send the current player position to the game manager.
void BlueBotController::emit_pose() {
    emit(Event("bot_pose", {
        {"owner", "blue"},
        {"x", x()},
        {"y", y()}
    }));
}

// Report tile ownership when the player enters a new grid cell.
void BlueBotController::claim_current_tile() {
    int col = static_cast<int>((x() - BOARD_LEFT + TILE_SIZE / 2.0) / TILE_SIZE);
    int row = static_cast<int>((BOARD_TOP - y() + TILE_SIZE / 2.0) / TILE_SIZE);

    if ( row < 0 || row >= ROWS || col < 0 || col >= COLS ) {
        return;
    }

    if ( row == last_row && col == last_col ) {
        return;
    }

    last_row = row;
    last_col = col;

    emit(Event("claim_tile", {
        {"owner", "blue"},
        {"row", row},
        {"col", col}
    }));
}