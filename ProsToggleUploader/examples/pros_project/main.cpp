#include "main.h"
#include "app_features.hpp"

// Replace these declarations with your own robot subsystem functions.
void update_drive();
void update_intake();
void update_lift();
void update_pneumatics();
void update_odometry();
void run_selected_autonomous();

void initialize() {
    ROBOT_DEBUG_LOG("Robot initialized\n");
}

void disabled() {}
void competition_initialize() {}

void autonomous() {
#if ROBOT_FEATURE_AUTONOMOUS
    run_selected_autonomous();
#else
    ROBOT_DEBUG_LOG("Autonomous disabled in desktop app\n");
#endif
}

void opcontrol() {
    while (true) {
#if ROBOT_FEATURE_DRIVE
        update_drive();
#endif

#if ROBOT_FEATURE_INTAKE
        update_intake();
#endif

#if ROBOT_FEATURE_LIFT
        update_lift();
#endif

#if ROBOT_FEATURE_PNEUMATICS
        update_pneumatics();
#endif

#if ROBOT_FEATURE_ODOMETRY
        update_odometry();
#endif

        // Every infinite PROS task loop needs a delay.
        pros::delay(10);
    }
}
