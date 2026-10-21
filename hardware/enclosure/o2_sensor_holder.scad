use <lib/threads.scad>

// Dimensions are millimeters. Sensor inserts from the top with a right-hand male M16x1 thread.
sensor_thread_diameter = 16; // Nominal sensor thread major diameter.
sensor_thread_pitch = 1; // Axial travel per full screw turn.
sensor_thread_engagement = 5; // Sensor thread length inserted into the holder.
thread_tolerance = 0.2; // Library fit allowance; tune with a thread fit-test print.
holder_wall = 1.5; // Wall outside the female thread's nominal major diameter.
floor_thickness = 1.5; // Solid holder floor around the inlet tube.
floor_edge_chamfer = 0.6; // Outer bottom floor bevel; zero keeps a square edge.

gas_tube_inner_diameter = 3.5; // Gas inlet bore through the floor and central tube.
gas_tube_outer_diameter = 5.5; // Outside diameter of the central delivery tube.
gas_tube_protrusion = 3; // Tube extension beyond the seated sensor's threaded tip.
inlet_hose_grip_length = 6; // Exposed round inlet length beyond the holder perimeter for the hose.
inlet_floor_overlap = 1; // Inlet crown overlap into the floor for a solid connection instead of tangential contact.

exhaust_hole_diameter = 4; // Diameter of each exhaust bore through the floor.
exhaust_center_radius = 5; // Hole-center radius from the inlet tube axis.

holder_outer_diameter = sensor_thread_diameter + 2 * holder_wall; // Derived outside diameter.
inlet_horizontal_length = holder_outer_diameter / 2 + inlet_hose_grip_length; // Total inlet length from the central axis.
sensor_tip_z = floor_thickness; // Sensor threaded tip seats directly against the floor.
holder_height = sensor_tip_z + sensor_thread_engagement; // Total holder height with the thread starting at the floor.
gas_tube_tip_z = sensor_tip_z + gas_tube_protrusion; // Tube tip measured from the sensor's threaded tip.
inlet_elbow_z = -gas_tube_outer_diameter / 2 + inlet_floor_overlap; // Derived elbow center merged into the floor.
exhaust_angles = [75, 145, 215, 285]; // Four outlets clear of the fixed inlet along positive X.

assert(sensor_thread_diameter > 0 && sensor_thread_pitch > 0 && thread_tolerance >= 0);
assert(holder_wall > 0 && gas_tube_protrusion > 0);
assert(sensor_thread_engagement > 0 && floor_thickness > 0);
assert(floor_edge_chamfer >= 0 && floor_edge_chamfer < min(floor_thickness, holder_wall),
    "Floor chamfer must leave material beneath the sensor seat and inside the outer wall.");
assert(gas_tube_inner_diameter > 0 && gas_tube_inner_diameter < gas_tube_outer_diameter);
assert(inlet_horizontal_length > gas_tube_outer_diameter / 2);
assert(inlet_floor_overlap > 0
    && inlet_floor_overlap <= (gas_tube_outer_diameter - gas_tube_inner_diameter) / 2,
    "Inlet overlap must join the floor without raising the horizontal bore into it.");
assert(inlet_hose_grip_length > 0, "The exposed hose grip length must be positive.");
assert(gas_tube_outer_diameter < sensor_thread_diameter - 2 * sensor_thread_pitch);
assert(exhaust_hole_diameter > 0
    && exhaust_center_radius - exhaust_hole_diameter / 2 >= gas_tube_outer_diameter / 2
    && exhaust_center_radius + exhaust_hole_diameter / 2 <= (sensor_thread_diameter - 2 * sensor_thread_pitch) / 2,
    "Floor exhaust holes must clear the inlet tube and stay inside the thread's clear bore.");
assert(exhaust_center_radius * sin(75) > (gas_tube_outer_diameter + exhaust_hole_diameter) / 2,
    "Floor exhaust holes must clear the horizontal inlet exterior.");
assert(2 * exhaust_center_radius * sin(35) > exhaust_hole_diameter,
    "Adjacent floor exhaust holes must not touch or overlap.");

module threaded_shell() {
    ScrewHole(sensor_thread_diameter, sensor_thread_engagement,
              position = [0, 0, sensor_tip_z], pitch = sensor_thread_pitch,
              tolerance = thread_tolerance)
        union() {
            if (floor_edge_chamfer > 0)
                cylinder(h = floor_edge_chamfer,
                         d1 = holder_outer_diameter - 2 * floor_edge_chamfer,
                         d2 = holder_outer_diameter, $fn = 96);
            translate([0, 0, floor_edge_chamfer])
                cylinder(h = holder_height - floor_edge_chamfer,
                         d = holder_outer_diameter, $fn = 96);
        }
}

module inlet_body() {
    translate([0, 0, inlet_elbow_z])
        cylinder(h = gas_tube_tip_z - inlet_elbow_z,
                 d = gas_tube_outer_diameter, $fn = 64);

    translate([0, 0, inlet_elbow_z]) {
        sphere(d = gas_tube_outer_diameter, $fn = 64);
        rotate([0, 90, 0])
            cylinder(h = inlet_horizontal_length, d = gas_tube_outer_diameter, $fn = 64);
    }
}

module inlet_bore() {
    translate([0, 0, inlet_elbow_z])
        cylinder(h = gas_tube_tip_z - inlet_elbow_z + 0.1,
                 d = gas_tube_inner_diameter, $fn = 64);

    translate([0, 0, inlet_elbow_z]) {
        sphere(d = gas_tube_inner_diameter, $fn = 64);
        rotate([0, 90, 0])
            cylinder(h = inlet_horizontal_length + 0.1, d = gas_tube_inner_diameter, $fn = 64);
    }
}

module floor_exhausts() {
    for (exhaust_angle = exhaust_angles)
        rotate([0, 0, exhaust_angle])
            translate([exhaust_center_radius, 0, -0.1])
                cylinder(h = floor_thickness + 0.2,
                         d = exhaust_hole_diameter, $fn = 32);
}

module inlet_floor_support() {
    translate([0, 0, inlet_elbow_z])
        intersection() {
            cylinder(h = gas_tube_outer_diameter / 2, d = holder_outer_diameter, $fn = 96);
            translate([0, -gas_tube_outer_diameter / 2, 0])
                cube([holder_outer_diameter / 2,
                      gas_tube_outer_diameter, gas_tube_outer_diameter / 2]);
        }
}

module o2_sensor_holder() {
    difference() {
        union() {
            threaded_shell();
            inlet_body();
            inlet_floor_support();
        }
        inlet_bore();
        floor_exhausts();
    }
}

o2_sensor_holder();