// Shallow flow chamber for the flat face of the helium sensor.
// The sensor face closes the open chamber side; gas flows across the whole face.
he_sensor_face_width = 9.7; // Sensor face width.
he_sensor_face_length = 19.3; // Sensor face length.
he_sensor_corner_diameter = he_sensor_face_width; // Diameter defining the rounded ends/corners.
sensor_body_clearance = 0; // Side clearance between the sensor body and retaining collar if needed
sensor_seat_overlap = 0.5; // Per-side ledge supporting the sensor face around the flow opening.
sensor_retainer_height = 6; // Collar height above the chamber that grips the sensor sides.
thermal_sensor_cutout_width = 7; // Opening width along the holder's long side.
thermal_sensor_cutout_start_above_collar = 2; // Opening starts this far above the collar base.

holder_wall = 1; // Wall thickness around the sensor face.
closed_end_thickness = 0.75; // Thickness of the chamber's closed back.
plenum_depth = 3.5; // Gap between the sensor face and the chamber back.

// Smooth spigots are provisional; size them to the flexible tubing.
tube_bore_diameter = 3.5; // Inside diameter of the tube ports and spigots.
tube_spigot_diameter = 5; // Outside diameter of each tube spigot.
tube_spigot_length = 6; // Length of each spigot beyond the holder end.
spigot_overlap = 0.3; // Amount each spigot overlaps the holder for a strong union.

chamber_width = he_sensor_face_width - 2 * sensor_seat_overlap; // Derived flow-opening width.
chamber_length = he_sensor_face_length - 2 * sensor_seat_overlap; // Derived flow-opening length.
chamber_corner_diameter = he_sensor_corner_diameter - 2 * sensor_seat_overlap; // Derived opening corner size.
sensor_cavity_width = he_sensor_face_width + 2 * sensor_body_clearance; // Derived collar opening width.
sensor_cavity_length = he_sensor_face_length + 2 * sensor_body_clearance; // Derived collar opening length.
sensor_cavity_corner_diameter = he_sensor_corner_diameter + 2 * sensor_body_clearance; // Derived collar opening corner size.
outer_width = he_sensor_face_width + 2 * holder_wall; // Derived holder outside width.
outer_length = he_sensor_face_length + 2 * holder_wall; // Derived holder outside length.
outer_corner_diameter = he_sensor_corner_diameter + 2 * holder_wall; // Derived holder corner size.
port_center_z = closed_end_thickness + plenum_depth / 2; // Tube-bore center height within the plenum.

module rounded_rectangle_2d(width, length, corner_diameter) {
    corner_radius = corner_diameter / 2;
    hull()
        for (y = [corner_radius, length - corner_radius])
            translate([width / 2, y]) circle(r = corner_radius, $fn = 48);
}

module rounded_rectangle_prism(width, length, corner_diameter, height) {
    linear_extrude(height = height)
        rounded_rectangle_2d(width, length, corner_diameter);
}

module he_sensor_holder() {
    difference() {
        union() {
            rounded_rectangle_prism(outer_width, outer_length, outer_corner_diameter,
                                    closed_end_thickness + plenum_depth);

            // Raised collar surrounds the sensor body above the gas chamber.
            translate([0, 0, closed_end_thickness + plenum_depth])
                rounded_rectangle_prism(outer_width, outer_length, outer_corner_diameter,
                                        sensor_retainer_height);

            // Inlet and outlet connect to opposite ends of the shallow chamber.
            for (side = [-1, 1])
                translate([outer_width / 2,
                           side < 0
                               ? -tube_spigot_length / 2 + spigot_overlap / 2
                               : outer_length + tube_spigot_length / 2 - spigot_overlap / 2,
                           port_center_z])
                    rotate([90, 0, 0])
                        cylinder(h = tube_spigot_length + spigot_overlap,
                                 d = tube_spigot_diameter, center = true, $fn = 48);
        }

        // Full-face plenum; the sensor face closes this opening when installed.
        translate([(outer_width - chamber_width) / 2,
               (outer_length - chamber_length) / 2,
                   closed_end_thickness])
            rounded_rectangle_prism(chamber_width, chamber_length,
                                    chamber_corner_diameter, plenum_depth + 0.1);

            // Sensor body slides through this collar opening and rests on the chamber ledge.
            translate([(outer_width - sensor_cavity_width) / 2,
                   (outer_length - sensor_cavity_length) / 2,
                   closed_end_thickness + plenum_depth])
                rounded_rectangle_prism(sensor_cavity_width, sensor_cavity_length,
                            sensor_cavity_corner_diameter, sensor_retainer_height + 0.1);

              // Side window lets a glued thermal sensor contact the helium sensor body.
              thermal_cutout_inner_x = (outer_width + sensor_cavity_width) / 2 - 0.1;
              translate([thermal_cutout_inner_x,
                     (outer_length - thermal_sensor_cutout_width) / 2,
                     closed_end_thickness + plenum_depth + thermal_sensor_cutout_start_above_collar])
                cube([outer_width - thermal_cutout_inner_x + 0.2,
                    thermal_sensor_cutout_width, sensor_retainer_height]);

        // Continuous round bore links both tube spigots across the sensor face.
        translate([outer_width / 2, outer_length / 2, port_center_z])
            rotate([90, 0, 0])
                cylinder(h = outer_length + 2 * tube_spigot_length + 1,
                         d = tube_bore_diameter, center = true, $fn = 48);
    }
}

he_sensor_holder();
