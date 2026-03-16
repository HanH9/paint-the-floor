#include <cmath>
#include "hazard.h"

using namespace enviro;

// Match this hazard instance to the nearest predefined anchor used at startup.
void HazardController::assign_index_from_anchor() {
    if ( hazard_index != -1 ) {
        return;
    }

    static const double anchor_x[8] = {
        -260.0, -180.0, -100.0, -20.0, 60.0, 140.0, 220.0, 260.0
    };

    static const double anchor_y[8] = {
        -120.0, 120.0, -120.0, 120.0, -120.0, 120.0, -120.0, 120.0
    };

    double best_dist = 1e18;
    int best_index = 0;

    for ( int i = 0; i < 8; i++ ) {
        double dx = x() - anchor_x[i];
        double dy = y() - anchor_y[i];
        double d2 = dx * dx + dy * dy;

        if ( d2 < best_dist ) {
            best_dist = d2;
            best_index = i;
        }
    }

    hazard_index = best_index;
}

// Register movement updates and route each one to the correct hazard instance.
void HazardController::init() {
    assign_index_from_anchor();

    watch("move_hazard", [this](Event& e) {
        try {
            assign_index_from_anchor();

            int index = e.value()["index"];
            if ( index != hazard_index ) {
                return;
            }

            double new_x = e.value()["x"];
            double new_y = e.value()["y"];
            teleport(new_x, new_y, 0);
        } catch (...) {
            return;
        }
    });
}

// Initialize the hazard and hide any default label.
void HazardController::start() {
    assign_index_from_anchor();
    label("", 0, 0);
}

// Keep the hazard still unless the game manager moves it.
void HazardController::update() {
    damp_movement();
    track_velocity(0, 0);
}

// No special shutdown work is needed for hazards.
void HazardController::stop() {}
