#ifndef __BLUE_BOT_AGENT__H
#define __BLUE_BOT_AGENT__H

#include <chrono>
#include <string>
#include "enviro.h"

using namespace enviro;

// Controls the player bot, including keyboard input, movement, and tile claiming.
class BlueBotController : public Process, public AgentInterface {

    public:
    // Construct the controller and attach it to Enviro's process system.
    BlueBotController() : Process(), AgentInterface() {}

    // Register event listeners used by the player bot.
    void init();
    // Initialize the player bot state when the simulation starts.
    void start();
    // Update player movement and game events each frame.
    void update();
    // Clean shutdown hook for the player bot.
    void stop();

    private:
    // Apply omni-directional movement from the current keyboard state.
    void update_motion();
    // Claim the tile currently occupied by the player bot.
    void claim_current_tile();
    // Publish the bot's current position for hazard checks.
    void emit_pose();
    // Draw the square body and heading line used for display.
    void draw_body();

    // Current keyboard state for four-direction movement.
    bool w_down = false;
    bool a_down = false;
    bool s_down = false;
    bool d_down = false;

    // Maximum omni-directional movement speed.
    double move_speed = 30.0;
    // Unit vector used to draw the heading line.
    double heading_dx = 0.0;
    double heading_dy = -1.0;

    // True while the round is active.
    bool round_running = true;
    // Time point until which the bot remains frozen by hazards.
    std::chrono::steady_clock::time_point frozen_until;

    // Last tile index reported to the game manager.
    int last_row = -999;
    int last_col = -999;

    // Board layout constants shared with the game manager.
    static constexpr double TILE_SIZE = 40.0;
    static constexpr double BOARD_LEFT = -300.0;
    static constexpr double BOARD_TOP = 160.0;
    static constexpr int ROWS = 9;
    static constexpr int COLS = 16;
};

// Enviro agent wrapper for the player-controlled blue bot.
class BlueBot : public Agent {
    public:
    // Attach the controller process to the agent.
    BlueBot(json spec, World& world) : Agent(spec, world) {
        add_process(c);
    }

    private:
    BlueBotController c;
};

DECLARE_INTERFACE(BlueBot)

#endif