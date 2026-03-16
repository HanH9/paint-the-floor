#ifndef __GAME_MANAGER_AGENT__H
#define __GAME_MANAGER_AGENT__H

#include <chrono>
#include <string>
#include <vector>
#include "enviro.h"

using namespace enviro;

// Coordinates the board state, score, timer, hazards, and restart flow.
class GameManagerController : public Process, public AgentInterface {

    public:
    // Construct the controller and attach it to Enviro's process system.
    GameManagerController() : Process(), AgentInterface() {}

    // Register all events used to run the match.
    void init();
    // Initialize the game manager when the simulation starts.
    void start();
    // Update timers, hazard refreshes, and contacts each frame.
    void update();
    // Clean shutdown hook for the game manager.
    void stop();

    private:
    // Start a fresh round, resetting board state and agent positions.
    void begin_round();
    // Reset board ownership, score, and hazard bookkeeping.
    void reset_board_data();
    // Refresh the visible HUD.
    void update_hud();
    // Redraw the board and HUD SVG.
    void redraw_board();
    // Apply a tile claim to the board state.
    void claim_tile(const std::string& owner, int row, int col);

    // Return true if the given tile is occupied by a static obstacle.
    bool blocked_tile(int row, int col) const;
    // Return true if a hazard can be placed on the given tile.
    bool valid_hazard_tile(int row, int col) const;

    // Convert a tile column index to world x-coordinate.
    double tile_x(int col) const;
    // Convert a tile row index to world y-coordinate.
    double tile_y(int row) const;

    // Build the board and HUD SVG string.
    std::string board_svg() const;
    // Return the fill color for a tile owner value.
    std::string tile_color(int owner) const;
    // Return the border color for a tile owner value.
    std::string tile_stroke(int owner) const;
    // Return the final result text for the completed round.
    std::string result_text() const;

    // Place one hazard at a random legal tile.
    void place_hazard_randomly(int hazard_index, bool force_new_spot);
    // Randomize all hazards at the start of a round.
    void place_all_hazards_randomly();
    // Apply the hazard freeze effect to a bot.
    void apply_hazard_to(const std::string& owner);
    // End the round and freeze game activity.
    void finish_round();
    // Return the remaining round time in seconds.
    int seconds_left() const;

    // Check all bot-hazard contacts using current positions.
    void check_hazard_contacts();
    // Return true if a bot is close enough to trigger a specific hazard.
    bool bot_is_touching_hazard(double bot_x, double bot_y, int hazard_index) const;

    // Board dimensions used for tile ownership and hazard placement.
    int rows = 9;
    int cols = 16;
    double tile_size = 40.0;

    // Stores tile ownership: 0 neutral, 1 blue, 2 red.
    std::vector<std::vector<int>> grid_owner;

    // Current tile counts for each side.
    int blue_score = 0;
    int red_score = 0;

    // Hazard bookkeeping for eight independent hazards.
    static const int hazard_count = 8;
    std::vector<int> hazard_rows;
    std::vector<int> hazard_cols;
    std::vector<std::chrono::steady_clock::time_point> hazard_move_times;

    // Round timing configuration and state.
    bool round_running = true;
    int round_length_seconds = 40;
    int hazard_refresh_seconds = 5;

    // Reference time for the current round.
    std::chrono::steady_clock::time_point round_start_time;

    // Latest reported bot positions used for hazard contact tests.
    double blue_x = -260.0;
    double blue_y = 0.0;
    double red_x = 260.0;
    double red_y = 0.0;

    // Short cooldowns prevent repeated hazard triggers on consecutive frames.
    std::chrono::steady_clock::time_point blue_hazard_cooldown_until;
    std::chrono::steady_clock::time_point red_hazard_cooldown_until;
};

// Enviro agent wrapper for the hidden game manager anchor.
class GameManager : public Agent {
    public:
    // Attach the controller process to the agent.
    GameManager(json spec, World& world) : Agent(spec, world) {
        add_process(c);
    }

    private:
    GameManagerController c;
};

DECLARE_INTERFACE(GameManager)

#endif