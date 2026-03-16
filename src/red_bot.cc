#include "red_bot.h"

using namespace enviro;

// Register hazard, round, and reset events for the autonomous bot.
void RedBotController::init() {
    watch("freeze_bot", [this](Event& e) {
        std::string owner = e.value()["owner"];
        if ( owner == "red" ) {
            int ms = e.value()["ms"];
            frozen_until = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
        }
    });

    watch("round_state", [this](Event& e) {
        round_running = e.value()["running"];
        if ( !round_running ) {
            turn_steps_remaining = 0;
            backing_steps_remaining = 0;
        }
    });

    watch("reset_red", [this](Event& e) {
        double x = e.value()["x"];
        double y = e.value()["y"];
        double theta = e.value()["theta"];

        teleport(x, y, theta);

        turn_steps_remaining = 0;
        backing_steps_remaining = 0;
        drift_ticks = 0;
        drift_direction = 1;
        turn_direction = 1;
        backing_direction = 1;

        last_row = -999;
        last_col = -999;
        frozen_until = std::chrono::steady_clock::now();
    });
}

// Initialize the red bot state at the start of a round.
void RedBotController::start() {
    label("", 0, 0);
    round_running = true;
    frozen_until = std::chrono::steady_clock::now();
}

// Advance the autonomous bot each frame.
void RedBotController::update() {
    update_motion();
    emit_pose();

    if ( round_running ) {
        claim_current_tile();
    }
}

// No special shutdown work is needed for the red bot.
void RedBotController::stop() {}

// Run the rule-based movement policy with avoidance and corner escape behavior.
void RedBotController::update_motion() {
    if ( !round_running ) {
        track_velocity(0, 0);
        return;
    }

    if ( std::chrono::steady_clock::now() < frozen_until ) {
        track_velocity(0, 0);
        return;
    }

    if ( backing_steps_remaining > 0 ) {
        track_velocity(-reverse_speed, backing_direction * 0.6);
        backing_steps_remaining--;
        return;
    }

    if ( turn_steps_remaining > 0 ) {
        track_velocity(0, turn_direction * turn_speed);
        turn_steps_remaining--;
        return;
    }

    double front = sensor_value(0);
    double left = sensor_value(1);
    double right = sensor_value(2);

    if ( front < 35 && left < 35 && right < 35 ) {
        int dir = (std::rand() % 2 == 0) ? -1 : 1;
        start_escape(dir);
        track_velocity(-reverse_speed, backing_direction * 0.6);
        return;
    }

    if ( front < 50 ) {
        if ( left > right + 6 ) {
            start_turn(-1);
        } else if ( right > left + 6 ) {
            start_turn(1);
        } else {
            start_turn((std::rand() % 2 == 0) ? -1 : 1);
        }

        track_velocity(0, turn_direction * turn_speed);
        return;
    }

    if ( left < 24 && right < 24 ) {
        int dir = (std::rand() % 2 == 0) ? -1 : 1;
        start_escape(dir);
        track_velocity(-reverse_speed, backing_direction * 0.6);
        return;
    }

    if ( left < 24 ) {
        start_turn(1);
        track_velocity(0, turn_direction * turn_speed);
        return;
    }

    if ( right < 24 ) {
        start_turn(-1);
        track_velocity(0, turn_direction * turn_speed);
        return;
    }

    drift_ticks++;
    if ( drift_ticks >= 120 ) {
        drift_ticks = 0;
        drift_direction = (std::rand() % 2 == 0) ? -1 : 1;
    }

    double w = 0.0;

    if ( left < 40 ) {
        w = 0.8;
    } else if ( right < 40 ) {
        w = -0.8;
    } else {
        w = 0.18 * drift_direction;
    }

    track_velocity(forward_speed, w);
}

// Start an in-place turn in the requested direction.
void RedBotController::start_turn(int dir) {
    turn_direction = dir;
    turn_steps_remaining = 12 + (std::rand() % 10);
    backing_steps_remaining = 0;
    drift_ticks = 0;
}

// Start a short reverse-and-turn maneuver to escape corners.
void RedBotController::start_escape(int dir) {
    backing_direction = dir;
    backing_steps_remaining = 10 + (std::rand() % 8);
    turn_direction = dir;
    turn_steps_remaining = 12 + (std::rand() % 10);
    drift_ticks = 0;
}

// Send the current red bot position to the game manager.
void RedBotController::emit_pose() {
    emit(Event("bot_pose", {
        {"owner", "red"},
        {"x", x()},
        {"y", y()}
    }));
}

// Report tile ownership when the red bot enters a new grid cell.
void RedBotController::claim_current_tile() {
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
        {"owner", "red"},
        {"row", row},
        {"col", col}
    }));
}
