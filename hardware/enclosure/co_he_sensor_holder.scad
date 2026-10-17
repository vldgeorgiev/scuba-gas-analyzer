// Dimensions are millimeters. CO inlet turns upward and its outlet joins He.
co_sensor_diameter = 20.15; // CO sensor fit diameter, including any desired clearance.
co_body_height = 10; // CO cup overall height.
co_floor_thickness = 3.7; // CO cup closed-end thickness.
co_wall_thickness = 1; // CO cup radial wall thickness.
co_port_height = 2.5; // CO bore center above the floor exterior; sets the shared passage height.
co_center_pad_height = 0.4; // Central sensor support height above the CO floor.
co_center_pad_diameter = 15; // Diameter of the central raised sensor support.

he_sensor_face_width = 9.7; // Sensor face width across X.
he_sensor_face_length = 19.3; // Sensor face length along Y.
he_sensor_body_clearance = 0; // Per-side clearance around the He sensor body.
he_sensor_seat_overlap = 0.5; // Per-side support ledge around the He flow chamber.
he_sensor_retainer_height = 6; // Collar height above the He chamber.
he_wall_thickness = 1; // He housing wall thickness.
he_floor_thickness = 0.75; // He floor thickness.
he_plenum_depth = 3.5; // Gas chamber depth beneath the He sensor face.
he_thermal_cutout_width = 7; // Temperature-sensor opening width along Y on the outlet side.
he_thermal_cutout_start_above_collar = 2; // Opening starts this far above the He chamber top.
he_separator_depth = 4; // Triangle length across X from its inlet-facing point toward the outlet.
he_separator_width = 8; // Triangle base width along Y; leave paths around both ends.
he_separator_offset_x = -1; // Divider center offset from the He housing center, toward CO.

joining_gap = 0; // Distance between housings; zero puts He directly against CO.
joining_web_width = 8; // Width of the solid joining web along Y.
joining_overlap = 1; // Web overlap into each housing to ensure a solid connection.

tube_bore_diameter = 3.5; // Shared gas passage and elbow bore diameter.
tube_spigot_diameter = 5; // Shared elbow and tube-fitting outside diameter.
spigot_overlap = 0.3; // Shared inlet/outlet elbow overlap into the housing walls.
inlet_elbow_standoff = 4; // Distance from the CO outer wall to the upward inlet axis.
outlet_elbow_standoff = 6; // Distance from the He outer wall to the upward outlet axis.
elbow_straight_length = 6; // Shared straight tube-fitting length above the rounded elbow joint.

elbow_reinforcement_side_thickness = 2; // Added thickness per side at the holder wall.
elbow_reinforcement_reach = 1; // Bend thickness scales by reach / standoff, capped at the full side thickness.

co_outer_diameter = co_sensor_diameter + 2 * co_wall_thickness; // Derived CO cup outside diameter.
he_outer_width = he_sensor_face_width + 2 * he_wall_thickness; // Derived He housing width.
he_outer_length = he_sensor_face_length + 2 * he_wall_thickness; // Derived He housing length.
he_chamber_width = he_sensor_face_width - 2 * he_sensor_seat_overlap; // Derived He flow-opening width.
he_chamber_length = he_sensor_face_length - 2 * he_sensor_seat_overlap; // Derived He flow-opening length.
he_cavity_width = he_sensor_face_width + 2 * he_sensor_body_clearance; // Derived collar opening width.
he_cavity_length = he_sensor_face_length + 2 * he_sensor_body_clearance; // Derived collar opening length.
he_origin_x = co_outer_diameter / 2 + joining_gap; // He left edge, with CO centered at the origin.
he_port_height = he_floor_thickness + he_plenum_depth / 2; // He port height before alignment with CO.
he_origin_z = co_port_height - he_port_height; // Align the He chamber ports with the CO passage.
he_top = he_floor_thickness + he_plenum_depth + he_sensor_retainer_height; // He overall height.
inlet_axis_x = -co_outer_diameter / 2 - inlet_elbow_standoff; // Derived upward inlet X position.
outlet_axis_x = he_outer_width + outlet_elbow_standoff; // Derived upward outlet X position.

assert(co_wall_thickness > 0 && co_sensor_diameter > 0);
assert(co_floor_thickness > 0 && co_center_pad_height >= 0
    && co_floor_thickness + co_center_pad_height < co_body_height);
assert(co_center_pad_diameter > 0 && co_center_pad_diameter < co_sensor_diameter);
assert(co_floor_thickness + co_center_pad_height < co_port_height + tube_bore_diameter / 2,
    "The round CO channel must open through the raised pad.");
assert(he_wall_thickness > 0 && he_floor_thickness > 0 && he_sensor_retainer_height > 0);
assert(he_sensor_body_clearance >= 0 && he_sensor_body_clearance < he_wall_thickness);
assert(he_sensor_seat_overlap > 0 && he_chamber_width > 0 && he_chamber_length > 0);
assert(joining_gap >= 0 && joining_overlap > 0);
assert(he_separator_depth > 0
    && abs(he_separator_offset_x) + he_separator_depth / 2 < he_chamber_width / 2);
assert(he_separator_width > 0 && he_separator_width < he_chamber_length);
assert(tube_bore_diameter > 0 && tube_bore_diameter <= he_plenum_depth
    && tube_bore_diameter < tube_spigot_diameter);
assert(inlet_elbow_standoff > tube_spigot_diameter / 2
    && outlet_elbow_standoff > tube_spigot_diameter / 2 && elbow_straight_length > 0);
assert(spigot_overlap > 0 && joining_web_width > 0);
assert(elbow_reinforcement_side_thickness >= 0 && elbow_reinforcement_reach > 0);

module elbow(diameter, horizontal_length, vertical_length, direction = 1) {
    sphere(d = diameter, $fn = 64);
    rotate([0, direction * 90, 0])
        cylinder(h = horizontal_length, d = diameter, $fn = 48);
    cylinder(h = vertical_length, d = diameter, $fn = 48);
}

module port_root_reinforcement(standoff, wall_overlap) {
    tube_radius = tube_spigot_diameter / 2;
    elbow_side_extra = elbow_reinforcement_side_thickness * min(1, elbow_reinforcement_reach / standoff);
    difference() {
        intersection() {
            hull() {
                translate([-wall_overlap, 0, 0])
                    rotate([0, 90, 0])
                        scale([1, (tube_radius + elbow_reinforcement_side_thickness) / tube_radius, 1])
                            cylinder(h = 0.01, r = tube_radius, $fn = 64);
                translate([standoff, 0, 0])
                    scale([1, (tube_radius + elbow_side_extra) / tube_radius, 1])
                        sphere(r = tube_radius, $fn = 64);
                translate([standoff, 0, tube_radius - 0.01])
                    cylinder(h = 0.01, r = tube_radius, $fn = 64);
            }
            translate([-wall_overlap, -tube_radius - elbow_reinforcement_side_thickness, -tube_radius])
                cube([wall_overlap + standoff + tube_radius,
                      2 * (tube_radius + elbow_reinforcement_side_thickness), tube_spigot_diameter]);
        }
        translate([-wall_overlap - 0.1, 0, 0])
            rotate([0, 90, 0])
                cylinder(h = wall_overlap + standoff + 0.2,
                         d = tube_bore_diameter, $fn = 48);
        translate([standoff, 0, 0]) {
            sphere(d = tube_bore_diameter, $fn = 64);
            cylinder(h = tube_radius + 0.1, d = tube_bore_diameter, $fn = 48);
        }
    }
}

module inlet_elbow_body() {
    translate([-co_outer_diameter / 2, 0, co_port_height])
        rotate([0, 0, 180])
            port_root_reinforcement(inlet_elbow_standoff, co_wall_thickness);
    translate([inlet_axis_x, 0, co_port_height])
        elbow(tube_spigot_diameter, inlet_elbow_standoff + spigot_overlap,
              tube_spigot_diameter / 2 + elbow_straight_length);
}

module inlet_elbow_bore() {
    translate([inlet_axis_x, 0, co_port_height])
        elbow(tube_bore_diameter, inlet_elbow_standoff + spigot_overlap + 0.1,
              tube_spigot_diameter / 2 + elbow_straight_length + 0.1);
}

module stadium(width, length, height) {
    linear_extrude(height = height)
        hull()
            for (center_y = [-(length - width) / 2, (length - width) / 2])
                translate([0, center_y]) circle(d = width, $fn = 96);
}

module he_outlet_elbow_body() {
    translate([he_outer_width, 0, he_port_height])
        port_root_reinforcement(outlet_elbow_standoff, he_wall_thickness);
    translate([outlet_axis_x, 0, he_port_height])
        elbow(tube_spigot_diameter, outlet_elbow_standoff + spigot_overlap,
              tube_spigot_diameter / 2 + elbow_straight_length, direction = -1);
}

module he_outlet_elbow_bore() {
    bore_start_x = he_outer_width - he_wall_thickness - he_sensor_seat_overlap - 0.1;
    translate([outlet_axis_x, 0, he_port_height])
        elbow(tube_bore_diameter, outlet_axis_x - bore_start_x + 0.2,
              tube_spigot_diameter / 2 + elbow_straight_length + 0.1, direction = -1);
}

module he_housing() {
    difference() {
        union() {
            translate([he_outer_width / 2, 0, 0])
                stadium(he_outer_width, he_outer_length, he_top);
            he_outlet_elbow_body();
        }
        translate([he_outer_width / 2, 0, he_floor_thickness])
            stadium(he_chamber_width, he_chamber_length, he_plenum_depth + 0.1);
        translate([he_outer_width / 2, 0, he_floor_thickness + he_plenum_depth])
            stadium(he_cavity_width, he_cavity_length, he_sensor_retainer_height + 0.1);
        translate([(he_outer_width + he_cavity_width) / 2 - 0.1,
                   -he_thermal_cutout_width / 2,
                   he_floor_thickness + he_plenum_depth + he_thermal_cutout_start_above_collar])
            cube([he_wall_thickness + 0.2, he_thermal_cutout_width, he_sensor_retainer_height + 0.1]);

        // Turn the outlet upward outside the housing without boring through the divider.
        he_outlet_elbow_bore();
    }
}

module co_housing() {
    difference() {
        union() {
            difference() {
                cylinder(h = co_body_height, d = co_outer_diameter, $fn = 96);
                translate([0, 0, co_floor_thickness])
                    cylinder(h = co_body_height - co_floor_thickness + 0.1,
                             d = co_sensor_diameter, $fn = 96);
            }
            translate([0, 0, co_floor_thickness - 0.1])
                cylinder(h = co_center_pad_height + 0.1,
                         d = co_center_pad_diameter, $fn = 96);
        }
        translate([0, 0, co_port_height])
            rotate([0, 90, 0])
                cylinder(h = co_outer_diameter + 0.2,
                         d = tube_bore_diameter, center = true, $fn = 48);
    }
}

module combined_housings() {
    difference() {
        union() {
            co_housing();
            inlet_elbow_body();
            translate([he_origin_x, 0, he_origin_z]) he_housing();
            translate([co_outer_diameter / 2 - joining_overlap, -joining_web_width / 2, 0])
                cube([joining_gap + 2 * joining_overlap, joining_web_width, co_body_height]);
        }
        inlet_elbow_bore();
        // Continue the CO passage through the solid web and the He inlet wall.
        translate([co_outer_diameter / 2 - joining_overlap - 0.1, 0, co_port_height])
            rotate([0, 90, 0])
                cylinder(h = joining_overlap + joining_gap + he_wall_thickness + he_sensor_seat_overlap + 0.2,
                         d = tube_bore_diameter, $fn = 48);
    }
}

module he_separator() {
    translate([he_origin_x + he_outer_width / 2 + he_separator_offset_x, 0,
               he_origin_z + he_floor_thickness - 0.1])
        linear_extrude(height = he_plenum_depth + 0.1)
            polygon([[-he_separator_depth / 2, 0],
                     [he_separator_depth / 2, -he_separator_width / 2],
                     [he_separator_depth / 2, he_separator_width / 2]]);
}

module co_he_sensor_holder() {
    union() {
        combined_housings();
        // Full-depth divider meets the sensor face: incoming gas must pass around either side.
        he_separator();
    }
}

co_he_sensor_holder();