use <co_sensor_holder.scad>

// Dimensions are millimeters. CO cup is unchanged; its inlet turns upward and outlet joins He.
co_outer_diameter = 22.1; // Match the current CO cup's outside diameter when its source dimensions change.
co_body_height = 10; // Match the current CO cup height.
co_port_height = 2.5; // Match the current CO bore center above its floor exterior.
joining_gap = 0; // Distance between housings; zero puts He directly against CO.
joining_web_width = 8; // Width of the solid joining web along Y.
joining_overlap = 1; // Web overlap into each housing to ensure a solid connection.

he_sensor_face_width = 9.7; // Sensor face width across X.
he_sensor_face_length = 19.3; // Sensor face length along Y.
sensor_body_clearance = 0; // Per-side clearance around the He sensor body.
sensor_seat_overlap = 0.5; // Per-side support ledge around the He flow chamber.
sensor_retainer_height = 6; // Collar height above the He chamber.
holder_wall = 1; // He housing wall thickness.
closed_end_thickness = 0.75; // He floor thickness.
plenum_depth = 3.5; // Gas chamber depth beneath the He sensor face.
thermal_sensor_cutout_width = 7; // Temperature-sensor opening width along Y on the outlet side.
thermal_sensor_cutout_start_above_collar = 2; // Opening starts this far above the He chamber top.

tube_bore_diameter = 3.5; // Connecting passage and He outlet bore diameter; match CO bore.
tube_spigot_diameter = 5; // He outlet spigot outside diameter.
tube_spigot_length = 6; // He outlet spigot length outside its housing.
spigot_overlap = 0.3; // Shared inlet/outlet elbow overlap into the housing walls.
inlet_elbow_standoff = 4; // Distance from the CO outer wall to the upward inlet axis.
outlet_elbow_standoff = 6; // Distance from the He outer wall to the upward outlet axis.
inlet_vertical_length = 6; // Straight tube-fitting length above the rounded elbow joint.
inlet_axis_x = -co_outer_diameter / 2 - inlet_elbow_standoff; // Derived upward inlet X position.
separator_depth = 4; // Triangle length across X from its inlet-facing point toward the outlet.
separator_width = 8; // Triangle base width along Y; leave paths around both ends.

he_outer_width = he_sensor_face_width + 2 * holder_wall; // Derived He housing width.
he_outer_length = he_sensor_face_length + 2 * holder_wall; // Derived He housing length.
chamber_width = he_sensor_face_width - 2 * sensor_seat_overlap; // Derived He flow-opening width.
chamber_length = he_sensor_face_length - 2 * sensor_seat_overlap; // Derived He flow-opening length.
sensor_cavity_width = he_sensor_face_width + 2 * sensor_body_clearance; // Derived collar opening width.
sensor_cavity_length = he_sensor_face_length + 2 * sensor_body_clearance; // Derived collar opening length.
he_origin_x = co_outer_diameter / 2 + joining_gap; // He left edge, with CO centered at the origin.
he_port_height = closed_end_thickness + plenum_depth / 2; // He port height before alignment with CO.
he_origin_z = co_port_height - he_port_height; // Align the He chamber ports with the CO passage.
he_top = closed_end_thickness + plenum_depth + sensor_retainer_height; // He overall height.
outlet_axis_x = he_outer_width + outlet_elbow_standoff; // Derived upward outlet X position.

assert(joining_gap >= 0 && joining_overlap > 0);
assert(separator_depth > 0 && separator_depth < chamber_width);
assert(separator_width > 0 && separator_width < chamber_length);
assert(tube_bore_diameter <= plenum_depth && tube_bore_diameter < tube_spigot_diameter);
assert(inlet_elbow_standoff > tube_spigot_diameter / 2 && inlet_vertical_length > 0);

module inlet_elbow_body() {
    translate([inlet_axis_x, 0, co_port_height]) {
        sphere(d = tube_spigot_diameter, $fn = 64);
        rotate([0, 90, 0])
            cylinder(h = inlet_elbow_standoff + spigot_overlap,
                     d = tube_spigot_diameter, $fn = 48);
        cylinder(h = tube_spigot_diameter / 2 + inlet_vertical_length,
                 d = tube_spigot_diameter, $fn = 48);
    }
}

module inlet_elbow_bore() {
    translate([inlet_axis_x, 0, co_port_height]) {
        sphere(d = tube_bore_diameter, $fn = 64);
        rotate([0, 90, 0])
            cylinder(h = inlet_elbow_standoff + spigot_overlap + 0.1,
                     d = tube_bore_diameter, $fn = 48);
        cylinder(h = tube_spigot_diameter / 2 + inlet_vertical_length + 0.1,
                 d = tube_bore_diameter, $fn = 48);
    }
}

module stadium(width, length, height) {
    linear_extrude(height = height)
        hull()
            for (center_y = [-(length - width) / 2, (length - width) / 2])
                translate([0, center_y]) circle(d = width, $fn = 96);
}

module he_outlet_elbow_body() {
    translate([outlet_axis_x, 0, he_port_height]) {
        sphere(d = tube_spigot_diameter, $fn = 64);
        rotate([0, -90, 0])
            cylinder(h = outlet_elbow_standoff + spigot_overlap,
                     d = tube_spigot_diameter, $fn = 48);
        cylinder(h = tube_spigot_diameter / 2 + inlet_vertical_length,
                 d = tube_spigot_diameter, $fn = 48);
    }
}

module he_outlet_elbow_bore() {
    bore_start_x = he_outer_width - holder_wall - sensor_seat_overlap - 0.1;
    translate([outlet_axis_x, 0, he_port_height]) {
        sphere(d = tube_bore_diameter, $fn = 64);
        rotate([0, -90, 0])
            cylinder(h = outlet_axis_x - bore_start_x + 0.2,
                     d = tube_bore_diameter, $fn = 48);
        cylinder(h = tube_spigot_diameter / 2 + inlet_vertical_length + 0.1,
                 d = tube_bore_diameter, $fn = 48);
    }
}

module he_housing() {
    difference() {
        union() {
            translate([he_outer_width / 2, 0, 0])
                stadium(he_outer_width, he_outer_length, he_top);
            he_outlet_elbow_body();
        }
        translate([he_outer_width / 2, 0, closed_end_thickness])
            stadium(chamber_width, chamber_length, plenum_depth + 0.1);
        translate([he_outer_width / 2, 0, closed_end_thickness + plenum_depth])
            stadium(sensor_cavity_width, sensor_cavity_length, sensor_retainer_height + 0.1);
        translate([(he_outer_width + sensor_cavity_width) / 2 - 0.1,
                   -thermal_sensor_cutout_width / 2,
                   closed_end_thickness + plenum_depth + thermal_sensor_cutout_start_above_collar])
            cube([holder_wall + 0.2, thermal_sensor_cutout_width, sensor_retainer_height + 0.1]);

        // Turn the outlet upward outside the housing without boring through the divider.
        he_outlet_elbow_bore();
    }
}

module combined_housings() {
    difference() {
        union() {
            // Trim both original spigots, retaining the CO cup and its internal passage.
            intersection() {
                co_sensor_holder();
                translate([-co_outer_diameter / 2, -co_outer_diameter, -1])
                    cube([co_outer_diameter, 2 * co_outer_diameter, co_body_height + 2]);
            }
            inlet_elbow_body();
            translate([he_origin_x, 0, he_origin_z]) he_housing();
            translate([co_outer_diameter / 2 - joining_overlap, -joining_web_width / 2, 0])
                cube([joining_gap + 2 * joining_overlap, joining_web_width, co_body_height]);
        }
        inlet_elbow_bore();
        // Continue the CO passage through the solid web and the He inlet wall.
        translate([co_outer_diameter / 2 - joining_overlap - 0.1, 0, co_port_height])
            rotate([0, 90, 0])
                cylinder(h = joining_overlap + joining_gap + holder_wall + sensor_seat_overlap + 0.2,
                         d = tube_bore_diameter, $fn = 48);
    }
}

module co_he_sensor_holder() {
    union() {
        combined_housings();
        // Full-depth divider meets the sensor face: incoming gas must pass around either side.
        translate([he_origin_x + he_outer_width / 2 - 1 , 0,
                   he_origin_z + closed_end_thickness - 0.1])
            linear_extrude(height = plenum_depth + 0.1)
                polygon([[-separator_depth / 2, 0],
                         [separator_depth / 2, -separator_width / 2],
                         [separator_depth / 2, separator_width / 2]]);
    }
}

co_he_sensor_holder();