use <lib/threads.scad>

// Dimensions are millimeters. Sensor inserts from the top with a right-hand male M16x1 thread.
sensor_thread_diameter = 16; // Nominal sensor thread major diameter.
sensor_thread_pitch = 1; // Axial travel per full screw turn.
sensor_thread_engagement = 5; // Sensor thread length inserted into the holder.
thread_tolerance = 0.2; // Library fit allowance; tune with a thread fit-test print.
holder_wall = 1.5; // Wall outside the female thread's nominal major diameter.
floor_thickness = 1.5; // Solid holder floor around the inlet tube.

gas_tube_inner_diameter = 3.5; // Gas inlet bore through the floor and central tube.
gas_tube_outer_diameter = 5.5; // Outside diameter of the central delivery tube.
gas_tube_protrusion = 3; // Tube extension beyond the seated sensor's threaded tip.
inlet_spigot_length = 6; // Horizontal inlet length measured from the central tube axis.
inlet_elbow_clearance = 1; // Gap between the horizontal spigot exterior and holder floor.
inlet_angle = 40; // Side inlet direction around Z, in degrees; zero points along positive X.
inlet_elbow_z = -gas_tube_outer_diameter / 2 - inlet_elbow_clearance; // Derived elbow center below the floor.

exhaust_hole_count = 5; // Number of vertical floor outlets; set to zero to disable.
exhaust_hole_diameter = 3.9; // Diameter of each exhaust bore through the floor.
exhaust_radius = 5; // Hole-center radius from the inlet tube axis.

holder_outer_diameter = sensor_thread_diameter + 2 * holder_wall; // Derived outside diameter.
sensor_tip_z = floor_thickness; // Sensor threaded tip seats directly against the floor.
holder_height = sensor_tip_z + sensor_thread_engagement; // Total holder height with the thread starting at the floor.
gas_tube_tip_z = sensor_tip_z + gas_tube_protrusion; // Tube tip measured from the sensor's threaded tip.

assert(sensor_thread_engagement > 0 && floor_thickness > 0);
assert(gas_tube_inner_diameter > 0 && gas_tube_inner_diameter < gas_tube_outer_diameter);
assert(inlet_spigot_length > gas_tube_outer_diameter / 2 && inlet_elbow_clearance > 0);
assert(gas_tube_outer_diameter < sensor_thread_diameter - 2 * sensor_thread_pitch);
assert(exhaust_hole_count == 0 || (exhaust_hole_diameter > 0
    && exhaust_radius - exhaust_hole_diameter / 2 > gas_tube_outer_diameter / 2
    && exhaust_radius + exhaust_hole_diameter / 2 < (sensor_thread_diameter - 2 * sensor_thread_pitch) / 2),
    "Floor exhaust holes must clear the inlet tube and stay inside the thread's clear bore.");

module o2_sensor_holder() {
    difference() {
        union() {
            // Thread only the shell; keep the central gas tube separate from the thread cutter.
            ScrewHole(sensor_thread_diameter, sensor_thread_engagement,
                      position = [0, 0, sensor_tip_z], pitch = sensor_thread_pitch,
                      tolerance = thread_tolerance)
                cylinder(h = holder_height, d = holder_outer_diameter, $fn = 96);

            // Tube protrusion is measured from the seated sensor's threaded tip.
            translate([0, 0, inlet_elbow_z])
                cylinder(h = gas_tube_tip_z - inlet_elbow_z,
                         d = gas_tube_outer_diameter, $fn = 64);

            translate([0, 0, inlet_elbow_z])
                rotate([0, 0, inlet_angle]) {
                    sphere(d = gas_tube_outer_diameter, $fn = 64);
                    rotate([0, 90, 0])
                        cylinder(h = inlet_spigot_length, d = gas_tube_outer_diameter, $fn = 64);
                }
        }

        translate([0, 0, inlet_elbow_z])
            cylinder(h = gas_tube_tip_z - inlet_elbow_z + 0.1,
                     d = gas_tube_inner_diameter, $fn = 64);

        // Rounded bore junction turns the inlet upward without opening the elbow bottom.
        translate([0, 0, inlet_elbow_z])
            rotate([0, 0, inlet_angle]) {
                sphere(d = gas_tube_inner_diameter, $fn = 64);
                rotate([0, 90, 0])
                    cylinder(h = inlet_spigot_length + 0.1, d = gas_tube_inner_diameter, $fn = 64);
            }

        if (exhaust_hole_count > 0)
            for (hole_index = [0 : exhaust_hole_count - 1])
                rotate([0, 0, hole_index * 360 / exhaust_hole_count])
                    translate([exhaust_radius, 0, -0.1])
                        cylinder(h = floor_thickness + 0.2,
                                 d = exhaust_hole_diameter, $fn = 32);
    }
}

o2_sensor_holder();