#include "lander.h"
#include <stdint.h>
#include<math.h>
/*TODO:
 * 1. Are we keeping track of the fuel weight?
 * 2. Most things are being kept track of in lander.ino, don't overcomplicate the work
 * by wanting to keep track of all the variables yourself. */

/*Set the shipweight explicitly since we can't acces the .ino file*/
double explicit_ship_weight = ship_weight;
double mass_rocket = explicit_ship_weight; /* Measured in kgs */
double mass_fuel = 1000.0; /* Measured in kgs */
/* double mass_total = mass_rocket + mass_fuel;  Measured in kgs */
double mass_total = mass_rocket + mass_fuel;
double burn_rate = 0.0; /* Measured in ? */
double final_height = 0.0;/* Measured in meters */
/* double target_height = 0.0;  The target height for the ship to reach */
double altitude = 0.0; /* Keeps track of the current height */
double change_in_altitude_since_last_time_slice = 0.0; /* */
double force_1 = 0.0; /* measured in Newtons */
double force_2 = 0.0; /* measured in Newtons */
/* nozzle velocity is declared in lander.h. and is 2000 m/s */
double acceleration = 0.0; /* measured in meters/seconds^(2) */
double final_velocity = 0.0; /* Measured in meters/second */
double initial_velocity = 0.0;
double change_in_velocity_since_last_time_slice = 0.0;
double range = 0.0; /* measured in meters */
double top_of_range = final_height + range;
double bottom_of_range = final_height - range;
/* Originally initialized as bottom_of_range + hover_range but that is not defined*/
double target_height = bottom_of_range + range;
double T = 0.0;
double height_cutoff = target_height - 200;
double velocity_max = sqrt(4000);
double time_slice = 0.1;
double the_gravity = 10.0;

double take_off() {
    force_1 = mass_total * 109;
    burn_rate = force_1 / nozzle_velocity;

    /* Update the following:
     * 1. Total mass
     * 2. Acceleration
     * 3. Altitude
     * 4. Final velocity
     * 5. Initial Velocity
     * 6. Change in altitude
     * */
    mass_total = mass_total - (burn_rate * time_slice);
    acceleration = (force_1 / mass_total) - 10;
    altitude = altitude + (initial_velocity * time_slice) + (0.5 * acceleration * time_slice * time_slice);
    final_velocity = initial_velocity + (acceleration * time_slice);
    initial_velocity = final_velocity;
    change_in_altitude_since_last_time_slice = height_cutoff - altitude;

    /* Increase the time slice*/
    T += time_slice;

    return burn_rate;
}

double take_us_to_goal() {
    if (initial_velocity >= velocity_max) {
        force_2 = 0.0;
    } else {
        force_2 = (((velocity_max * velocity_max) - (initial_velocity * initial_velocity)) * mass_total) /
                  (2.0 * change_in_altitude_since_last_time_slice);
        force_1 = mass_total * 109;
    }

    if (force_1 < force_2) {
        burn_rate = (force_1 / nozzle_velocity);
        mass_total = mass_total - (burn_rate * time_slice);
        acceleration = (force_1 / mass_total) - the_gravity;
    } else {
        burn_rate = force_2 / nozzle_velocity;
        mass_total = mass_total - (burn_rate * time_slice);
        acceleration = (force_2 / mass_total) - the_gravity;
    }

    altitude = altitude + (initial_velocity * time_slice) + (0.5 * acceleration * time_slice * time_slice);
    final_velocity = initial_velocity + (acceleration * time_slice);
    initial_velocity = final_velocity;
    change_in_altitude_since_last_time_slice = height_cutoff - altitude;


    /*The following code cannot be implemented because we cannot change the ship_state:
     * if altitude = hf
		ship_state = 2
     */
    /* Can't change the ship_state*/

    return burn_rate;
}

double hover(ship_state_type ship) {
    while (ship.run_state == 2) {

        /* TODO:
         * 1. Change the condition to if, since it will never exit the while loop otherwise.
         * 2. Change the conditions to if (altitude >= (hover_altitude - hover_range)) &&
         *    (altitude <= (hover_altitude + hover_range))
         * */
        while (altitude >= bottom_of_range && altitude <= top_of_range) {
            /* Questionable statement .. Is this correct? */
            change_in_altitude_since_last_time_slice = top_of_range - bottom_of_range;
            final_velocity = sqrt(2.0 * the_gravity * change_in_altitude_since_last_time_slice);
            force_2 = (final_velocity * final_velocity * mass_total) / (2.0 * change_in_altitude_since_last_time_slice);
            force_1 = mass_total * 109;
            if (force_1 < force_2) {
                burn_rate = force_1 / nozzle_velocity;
                mass_total = mass_total - (burn_rate * time_slice);
                acceleration = (force_1 / mass_total) - 10;
            } else {
                burn_rate = force_2 / nozzle_velocity;
                mass_total = mass_total - (burn_rate * time_slice);
                acceleration = (force_2 / mass_total) - 10;
            }
            altitude = altitude + (initial_velocity * time_slice) + (0.5 * acceleration * time_slice * time_slice);

            final_velocity = initial_velocity + (acceleration * time_slice);
            initial_velocity = final_velocity;
            T += time_slice;

            /*Cannot implement
             * if (T == 5) {
             *  ss.ship_state = 3;
             * }*/
        }
    }
    return burn_rate;
}

double landing(ship_state_type ship) {
    while (ship.run_state == 3) {
        altitude = altitude + (initial_velocity * time_slice) + (0.5 * (-the_gravity) * time_slice * time_slice);
        final_velocity = initial_velocity + (-the_gravity * time_slice);
        initial_velocity = final_velocity;

        if (altitude == 100) {
            force_2 = (((-3.0 * -3.0) - (initial_velocity * initial_velocity) * mass_total) / (2.0 * altitude));
            force_1 = mass_total * 109;

            if (force_1 > force_2) {
                burn_rate = (force_1 / nozzle_velocity);
                mass_total = mass_total - (burn_rate * time_slice);
                acceleration = (force_1 / mass_total) - the_gravity;
            } else {
                burn_rate = (force_2 / nozzle_velocity);
                mass_total = mass_total - (burn_rate * time_slice);
                acceleration = (force_2 / mass_total) - the_gravity;
            }
            altitude = altitude + (initial_velocity * time_slice) + (0.5 * acceleration * time_slice * time_slice);
            final_velocity = initial_velocity + (acceleration * time_slice);
            initial_velocity = final_velocity;
        }

    }
    return burn_rate;
}

/*
 * Use a stack initialized within this stack that pushes the initial altitude onto it, and
 * then pops it into the parameter of this function along with the final_height in order to return the change
 * in altitude
 * double calculate_change_in_altitude(double initial_altitude_at_beginning_of_time_slice, double final_height) {
 *
 *  return final_height - initial_altitude_at_beginning_of_time_slice;
 * }
 * */



double calculate_change_in_velocity(double the_initial_velocity_at_beginning_of_time_slice, double the_final_velocity) {
    return the_final_velocity - the_initial_velocity_at_beginning_of_time_slice;
}


double setBurnRate(ship_state_type ss) {
    double ze_burn_rate = 0.0;
    /*if (ss.altitude <   && ss.run_state == 1){*/
    if (ss.altitude < hover_altitude) {
        ze_burn_rate = take_off();
    }
    /* else if ((altitude < height_cutoff) && (T>= 0.5)) {
         ze_burn_rate = take_us_to_goal();
     }*/

    return ze_burn_rate;
}
/*

int main() {
    return 0;
}
*/
