#ifndef __HAZARD_AGENT__H
#define __HAZARD_AGENT__H

#include "enviro.h"

using namespace enviro;

// Controls one hazard agent and links it to a stable hazard index.
class HazardController : public Process, public AgentInterface {

    public:
    // Construct the controller and attach it to Enviro's process system.
    HazardController() : Process(), AgentInterface() {}

    // Register move events for this hazard.
    void init();
    // Initialize the hazard when the simulation starts.
    void start();
    // Keep the hazard still between move events.
    void update();
    // Clean shutdown hook for the hazard.
    void stop();

    private:
    // Assign a stable hazard index from the initial anchor position.
    void assign_index_from_anchor();

    // Unique index used to route move events to this hazard.
    int hazard_index = -1;
};

// Enviro agent wrapper for one hazard object.
class Hazard : public Agent {
    public:
    // Attach the controller process to the agent.
    Hazard(json spec, World& world) : Agent(spec, world) {
        add_process(c);
    }

    private:
    HazardController c;
};

DECLARE_INTERFACE(Hazard)

#endif