#ifndef __RED_BOT_AGENT__H
#define __RED_BOT_AGENT__H

#include <chrono>
#include <cstdlib>
#include <string>
#include "enviro.h"

using namespace enviro;

// Controls the autonomous red bot, including avoidance and escape behavior.
class RedBotController : public Process, public AgentInterface {

    public:
    // Construct the controller and attach it to Enviro's process system.
    RedBotController() : Process(), AgentInterface() {}

    // Register round and reset events for the red bot.
    void init();
    // Initialize the red bot when the simulation starts.
    void start();
    // Update red bot movement and tile claiming each frame.
    void update();
    // Clean shutdown hook for the red bot.
    void stop();

    private:
    // Run the main movement and avoidance policy.
    void update_motion();
    // Begin a turning maneuver in the given direction.
    void start_turn(int dir);
    // Begin a backing-up escape maneuver.
    void start_escape(int dir);
    // Claim the tile currently occupied by the red bot.
    void claim_current_tile();
    // Publish the bot's current position for hazard checks.
    void emit_pose();

    // Basic forward, reverse, and angular speeds.
    double forward_speed = 65.0;
    double reverse_speed = 50.0;
    double turn_speed = 2.0;

    // Round state and freeze timer from hazard contact.
    bool round_running = true;
    std::chrono::steady_clock::time_point frozen_until;

    // Current turn state.
    int turn_direction = 1;
    int turn_steps_remaining = 0;

    // Current backing-up state used when escaping corners.
    int backing_direction = 1;
    int backing_steps_remaining = 0;

    // Small random drift keeps the bot from following the exact same path.
    int drift_direction = 1;
    int drift_ticks = 0;

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

// Enviro agent wrapper for the autonomous red bot.
class RedBot : public Agent {
    public:
    // Attach the controller process to the agent.
    RedBot(json spec, World& world) : Agent(spec, world) {
        add_process(c);
    }

    private:
    RedBotController c;
};

DECLARE_INTERFACE(RedBot)

#endif