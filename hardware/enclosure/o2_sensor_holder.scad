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
inlet_horizontal_length = 6; // Horizontal inlet length measured from the central tube axis.
inlet_elbow_clearance = 1; // Gap between the horizontal spigot exterior and holder floor.
inlet_angle = 40; // Side inlet direction around Z, in degrees; zero points along positive X.

exhaust_hole_count = 5; // Number of vertical floor outlets; set to zero to disable.
exhaust_hole_diameter = 3.9; // Diameter of each exhaust bore through the floor.
exhaust_center_radius = 5; // Hole-center radius from the inlet tube axis.

inlet_rib_thickness = 1.5; // Tangential rib thickness before trimming to the exhaust bores.
inlet_rib_reach = 2; // Radial extension beyond the central tube at the floor.
inlet_rib_height = 2; // Rib depth below the floor; the hose-facing gap stays unsupported.

holder_outer_diameter = sensor_thread_diameter + 2 * holder_wall; // Derived outside diameter.
sensor_tip_z = floor_thickness; // Sensor threaded tip seats directly against the floor.
holder_height = sensor_tip_z + sensor_thread_engagement; // Total holder height with the thread starting at the floor.
gas_tube_tip_z = sensor_tip_z + gas_tube_protrusion; // Tube tip measured from the sensor's threaded tip.
inlet_elbow_z = -gas_tube_outer_diameter / 2 - inlet_elbow_clearance; // Derived elbow center below the floor.
exhaust_gap_angle = exhaust_hole_count > 0 ? 360 / exhaust_hole_count : 0; // Angular spacing between exhaust holes.
hose_gap_index = exhaust_hole_count > 0
    ? floor(((inlet_angle % 360 + 360) % 360) / exhaust_gap_angle) : -1; // Gap containing the hose direction.
inlet_rib_angles = exhaust_hole_count > 1
    ? [for (gap_index = [0 : exhaust_hole_count - 1])
        if (gap_index != hose_gap_index) (gap_index + 0.5) * exhaust_gap_angle]
    : []; // One rib between each pair of holes, except the hose gap.

assert(sensor_thread_diameter > 0 && sensor_thread_pitch > 0 && thread_tolerance >= 0);
assert(holder_wall > 0 && gas_tube_protrusion > 0);
assert(sensor_thread_engagement > 0 && floor_thickness > 0);
assert(gas_tube_inner_diameter > 0 && gas_tube_inner_diameter < gas_tube_outer_diameter);
assert(inlet_horizontal_length > gas_tube_outer_diameter / 2 && inlet_elbow_clearance > 0);
assert(gas_tube_outer_diameter < sensor_thread_diameter - 2 * sensor_thread_pitch);
assert(exhaust_hole_count >= 0 && exhaust_hole_count == floor(exhaust_hole_count),
    "Exhaust hole count must be a nonnegative integer.");
assert(exhaust_hole_count == 0 || (exhaust_hole_diameter > 0
    && exhaust_center_radius - exhaust_hole_diameter / 2 > gas_tube_outer_diameter / 2
    && exhaust_center_radius + exhaust_hole_diameter / 2 < (sensor_thread_diameter - 2 * sensor_thread_pitch) / 2),
    "Floor exhaust holes must clear the inlet tube and stay inside the thread's clear bore.");
assert(exhaust_hole_count <= 1
    || 2 * exhaust_center_radius * sin(180 / exhaust_hole_count) > exhaust_hole_diameter,
    "Adjacent floor exhaust holes must not touch or overlap.");
assert(inlet_rib_thickness > 0 && inlet_rib_reach > 0);
assert(inlet_rib_height > 0, "Rib height must be positive.");
assert(gas_tube_outer_diameter / 2 + inlet_rib_reach < holder_outer_diameter / 2);

module threaded_shell() {
    ScrewHole(sensor_thread_diameter, sensor_thread_engagement,
              position = [0, 0, sensor_tip_z], pitch = sensor_thread_pitch,
              tolerance = thread_tolerance)
        cylinder(h = holder_height, d = holder_outer_diameter, $fn = 96);
}

module inlet_body() {
    translate([0, 0, inlet_elbow_z])
        cylinder(h = gas_tube_tip_z - inlet_elbow_z,
                 d = gas_tube_outer_diameter, $fn = 64);

    translate([0, 0, inlet_elbow_z])
        rotate([0, 0, inlet_angle]) {
            sphere(d = gas_tube_outer_diameter, $fn = 64);
            rotate([0, 90, 0])
                cylinder(h = inlet_horizontal_length, d = gas_tube_outer_diameter, $fn = 64);
        }
}

module inlet_bore() {
    translate([0, 0, inlet_elbow_z])
        cylinder(h = gas_tube_tip_z - inlet_elbow_z + 0.1,
                 d = gas_tube_inner_diameter, $fn = 64);

    translate([0, 0, inlet_elbow_z])
        rotate([0, 0, inlet_angle]) {
            sphere(d = gas_tube_inner_diameter, $fn = 64);
            rotate([0, 90, 0])
                cylinder(h = inlet_horizontal_length + 0.1, d = gas_tube_inner_diameter, $fn = 64);
        }
}

module floor_exhausts(below_floor = 0) {
    if (exhaust_hole_count > 0)
        for (hole_index = [0 : exhaust_hole_count - 1])
            rotate([0, 0, hole_index * 360 / exhaust_hole_count])
                translate([exhaust_center_radius, 0, -below_floor - 0.1])
                    cylinder(h = floor_thickness + below_floor + 0.2,
                             d = exhaust_hole_diameter, $fn = 32);
}

module inlet_ribs() {
    tube_radius = gas_tube_outer_diameter / 2;
    difference() {
        union() {
            for (rib_angle = inlet_rib_angles)
                rotate([0, 0, rib_angle])
                    rotate([90, 0, 0])
                        linear_extrude(height = inlet_rib_thickness, center = true)
                            polygon([[tube_radius - 0.2, -inlet_rib_height],
                                     [tube_radius + inlet_rib_reach, 0],
                                     [tube_radius + inlet_rib_reach, 0.1],
                                     [tube_radius - 0.2, 0.1]]);
        }
        floor_exhausts(below_floor = inlet_rib_height);
    }
}

module o2_sensor_holder() {
    difference() {
        union() {
            threaded_shell();
            inlet_body();
            inlet_ribs();
        }
        inlet_bore();
        floor_exhausts();
    }
}

o2_sensor_holder();