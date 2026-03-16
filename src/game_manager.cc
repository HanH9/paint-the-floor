#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include "game_manager.h"

using namespace enviro;

// Keeps the hidden game manager body outside the playable arena.
static constexpr double GM_ANCHOR_X = -10000.0;
static constexpr double GM_ANCHOR_Y = -10000.0;

// Register tile-claim events, bot position updates, and restart button clicks.
void GameManagerController::init() {

    watch("claim_tile", [this](Event& e) {
        try {
            std::string owner = e.value()["owner"];
            int row = e.value()["row"];
            int col = e.value()["col"];
            claim_tile(owner, row, col);
        } catch (...) {
            return;
        }
    });

    watch("bot_pose", [this](Event& e) {
        try {
            std::string owner = e.value()["owner"];
            double x = e.value()["x"];
            double y = e.value()["y"];

            if ( owner == "blue" ) {
                blue_x = x;
                blue_y = y;
            } else if ( owner == "red" ) {
                red_x = x;
                red_y = y;
            }
        } catch (...) {
            return;
        }
    });

    watch("button_click", [this](Event& e) {
        begin_round();
    });
}

// Seed randomness, anchor the hidden manager body, and start the first round.
void GameManagerController::start() {
    std::srand(std::time(nullptr));
    prevent_rotation();
    teleport(GM_ANCHOR_X, GM_ANCHOR_Y, 0);
    begin_round();
}

// Update hazard timers, hazard contacts, and round timing every frame.
void GameManagerController::update() {
    teleport(GM_ANCHOR_X, GM_ANCHOR_Y, 0);
    track_velocity(0, 0);

    if ( !round_running ) {
        update_hud();
        return;
    }

    auto now = std::chrono::steady_clock::now();

    for ( int i = 0; i < hazard_count; i++ ) {
        int hazard_elapsed =
            std::chrono::duration_cast<std::chrono::seconds>(now - hazard_move_times[i]).count();

        if ( hazard_elapsed >= hazard_refresh_seconds ) {
            place_hazard_randomly(i, true);
            hazard_move_times[i] = now;
        }
    }

    check_hazard_contacts();

    if ( seconds_left() <= 0 ) {
        finish_round();
        return;
    }

    update_hud();
}

// No special shutdown work is needed for the game manager.
void GameManagerController::stop() {}

// Reset board state, reposition bots, randomize hazards, and begin a new round.
void GameManagerController::begin_round() {
    teleport(GM_ANCHOR_X, GM_ANCHOR_Y, 0);
    reset_board_data();

    round_running = true;
    round_start_time = std::chrono::steady_clock::now();

    blue_x = -260.0;
    blue_y = 0.0;
    red_x = 260.0;
    red_y = 0.0;

    blue_hazard_cooldown_until = std::chrono::steady_clock::now();
    red_hazard_cooldown_until = std::chrono::steady_clock::now();

    emit(Event("round_state", {
        {"running", true}
    }));

    emit(Event("reset_blue", {
        {"x", -260.0},
        {"y", 0.0},
        {"theta", 0.0}
    }));

    emit(Event("reset_red", {
        {"x", 260.0},
        {"y", 0.0},
        {"theta", 180.0}
    }));

    place_all_hazards_randomly();
    redraw_board();
    update_hud();
}

// Clear board ownership, scores, and all hazard bookkeeping.
void GameManagerController::reset_board_data() {
    grid_owner.assign(rows, std::vector<int>(cols, 0));
    blue_score = 0;
    red_score = 0;
    hazard_rows.assign(hazard_count, -1);
    hazard_cols.assign(hazard_count, -1);
    hazard_move_times.assign(hazard_count, std::chrono::steady_clock::now());
}

// Return the number of whole seconds remaining in the current round.
int GameManagerController::seconds_left() const {
    if ( !round_running ) {
        return 0;
    }

    auto now = std::chrono::steady_clock::now();
    int elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - round_start_time).count();
    int left = round_length_seconds - elapsed;

    if ( left < 0 ) {
        left = 0;
    }

    return left;
}

// Compute the final result string from the current score.
std::string GameManagerController::result_text() const {
    if ( blue_score > red_score ) {
        return "Blue Wins";
    } else if ( red_score > blue_score ) {
        return "Red Wins";
    } else {
        return "Tie";
    }
}

// Stop the round and notify all agents that gameplay is over.
void GameManagerController::finish_round() {
    round_running = false;

    emit(Event("round_state", {
        {"running", false}
    }));

    update_hud();
}

// Refresh the visible board and HUD overlay.
void GameManagerController::update_hud() {
    redraw_board();
}

// Convert a board column to the x-coordinate of that tile center.
double GameManagerController::tile_x(int col) const {
    return -300.0 + col * tile_size;
}

// Convert a board row to the y-coordinate of that tile center.
double GameManagerController::tile_y(int row) const {
    return 160.0 - row * tile_size;
}

// Return true when the given tile lies inside one of the static obstacles.
bool GameManagerController::blocked_tile(int row, int col) const {
    if ( row >= 2 && row <= 3 && col >= 3 && col <= 4 ) {
        return true;
    }

    if ( row >= 5 && row <= 6 && col >= 7 && col <= 8 ) {
        return true;
    }

    if ( row >= 2 && row <= 3 && col >= 11 && col <= 12 ) {
        return true;
    }

    return false;
}

// Return true when a hazard may legally occupy the given tile.
bool GameManagerController::valid_hazard_tile(int row, int col) const {
    if ( row < 0 || row >= rows || col < 0 || col >= cols ) {
        return false;
    }

    if ( blocked_tile(row, col) ) {
        return false;
    }

    return true;
}

// Return the fill color used for a tile owner value.
std::string GameManagerController::tile_color(int owner) const {
    if ( owner == 1 ) {
        return "#5DADE2";
    } else if ( owner == 2 ) {
        return "#EC7063";
    } else {
        return "#EAECEE";
    }
}

// Return the outline color used for a tile owner value.
std::string GameManagerController::tile_stroke(int owner) const {
    if ( owner == 1 ) {
        return "#2E86C1";
    } else if ( owner == 2 ) {
        return "#CB4335";
    } else {
        return "#BDC3C7";
    }
}

// Build the SVG for the entire board and top HUD text.
std::string GameManagerController::board_svg() const {
    std::ostringstream out;

    std::string hud =
        "Time: " + std::to_string(seconds_left()) +
        "   |   Blue: " + std::to_string(blue_score) +
        "   |   Red: " + std::to_string(red_score);

    if ( !round_running ) {
        hud += "   |   " + result_text();
    }

    out << "<g transform='translate(" << (-GM_ANCHOR_X) << "," << (-GM_ANCHOR_Y) << ")'>";

    out << "<text x='0' y='-215' text-anchor='middle' "
        << "font-size='18' font-family='Arial' fill='black'>"
        << hud
        << "</text>";

    for ( int row = 0; row < rows; row++ ) {
        for ( int col = 0; col < cols; col++ ) {
            if ( blocked_tile(row, col) ) {
                continue;
            }

            double cx = tile_x(col);
            double cy = tile_y(row);
            int owner = grid_owner[row][col];

            out << "<rect x='" << (cx - 16)
                << "' y='" << (cy - 16)
                << "' width='32' height='32' "
                << "style='fill:" << tile_color(owner)
                << ";stroke:" << tile_stroke(owner)
                << ";stroke-width:1' />";
        }
    }

    out << "</g>";
    return out.str();
}

// Push the latest board SVG to the hidden game manager agent.
void GameManagerController::redraw_board() {
    decorate(board_svg());
}

// Freeze the specified bot for two seconds after a hazard hit.
void GameManagerController::apply_hazard_to(const std::string& owner) {
    emit(Event("freeze_bot", {
        {"owner", owner},
        {"ms", 2000}
    }));
}

// Return true if a bot is close enough to the hazard center to trigger it.
bool GameManagerController::bot_is_touching_hazard(double bot_x, double bot_y, int hazard_index) const {
    double hx = tile_x(hazard_cols[hazard_index]);
    double hy = tile_y(hazard_rows[hazard_index]);

    double dx = bot_x - hx;
    double dy = bot_y - hy;
    double dist = std::sqrt(dx * dx + dy * dy);

    return dist <= 25.0;
}

// Check all hazards against both bots and trigger any necessary freeze effects.
void GameManagerController::check_hazard_contacts() {
    auto now = std::chrono::steady_clock::now();

    for ( int i = 0; i < hazard_count; i++ ) {
        if ( hazard_rows[i] < 0 || hazard_cols[i] < 0 ) {
            continue;
        }

        if ( now >= blue_hazard_cooldown_until && bot_is_touching_hazard(blue_x, blue_y, i) ) {
            apply_hazard_to("blue");
            place_hazard_randomly(i, true);
            hazard_move_times[i] = now;
            blue_hazard_cooldown_until = now + std::chrono::milliseconds(300);
            continue;
        }

        if ( now >= red_hazard_cooldown_until && bot_is_touching_hazard(red_x, red_y, i) ) {
            apply_hazard_to("red");
            place_hazard_randomly(i, true);
            hazard_move_times[i] = now;
            red_hazard_cooldown_until = now + std::chrono::milliseconds(300);
            continue;
        }
    }
}

// Randomize one hazard to a valid, non-overlapping tile.
void GameManagerController::place_hazard_randomly(int hazard_index, bool force_new_spot) {
    int row = hazard_rows[hazard_index];
    int col = hazard_cols[hazard_index];

    bool ok = false;

    while ( !ok ) {
        row = std::rand() % rows;
        col = std::rand() % cols;

        if ( !valid_hazard_tile(row, col) ) {
            continue;
        }

        if ( force_new_spot && row == hazard_rows[hazard_index] && col == hazard_cols[hazard_index] ) {
            continue;
        }

        ok = true;
        for ( int i = 0; i < hazard_count; i++ ) {
            if ( i == hazard_index ) {
                continue;
            }
            if ( hazard_rows[i] == row && hazard_cols[i] == col ) {
                ok = false;
                break;
            }
        }
    }

    hazard_rows[hazard_index] = row;
    hazard_cols[hazard_index] = col;

    emit(Event("move_hazard", {
        {"index", hazard_index},
        {"x", tile_x(col)},
        {"y", tile_y(row)}
    }));
}

// Randomize every hazard and sync its timer to the current moment.
void GameManagerController::place_all_hazards_randomly() {
    auto now = std::chrono::steady_clock::now();

    for ( int i = 0; i < hazard_count; i++ ) {
        place_hazard_randomly(i, false);
        hazard_move_times[i] = now;
    }
}

// Update board ownership when a bot enters a new tile.
void GameManagerController::claim_tile(const std::string& owner, int row, int col) {
    if ( !round_running ) {
        return;
    }

    if ( row < 0 || row >= rows || col < 0 || col >= cols ) {
        return;
    }

    if ( blocked_tile(row, col) ) {
        return;
    }

    int new_owner = 0;
    if ( owner == "blue" ) {
        new_owner = 1;
    } else if ( owner == "red" ) {
        new_owner = 2;
    } else {
        return;
    }

    int old_owner = grid_owner[row][col];
    if ( old_owner != new_owner ) {
        if ( old_owner == 1 ) {
            blue_score--;
        } else if ( old_owner == 2 ) {
            red_score--;
        }

        grid_owner[row][col] = new_owner;

        if ( new_owner == 1 ) {
            blue_score++;
        } else if ( new_owner == 2 ) {
            red_score++;
        }

        redraw_board();
    }
}
