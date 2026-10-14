// CO sensor cup with opposed side spigots for flexible inlet and outlet tubes.
// Replace the provisional sensor diameter with the measured sensor body diameter.
co_sensor_diameter = 20; // Sensor body diameter
sensor_clearance = 0.05; // Radial extra clearance around the sensor body if needed

holder_height = 10; // Overall cup height.
holder_wall = 1; // Radial wall thickness around the sensor.
closed_end_thickness = 4; // Thickness of the closed end that stops the sensor.

// Smooth spigots are sized for flexible tubing; adjust to the tube's inside diameter.
tube_bore_diameter = 3.5; // Inside diameter of both gas ports and spigots.
tube_spigot_diameter = 5; // Outside diameter of the tube-attachment spigots.
tube_spigot_length = 6; // Length of each tube spigot outside the cup.
spigot_overlap = 0.6; // Amount each spigot overlaps the cup wall for a strong union.
gas_passage_overlap = 0.25; // How far the round passage rises above the closed-end surface.

sensor_cavity_diameter = co_sensor_diameter + 2 * sensor_clearance; // Derived sensor-cavity diameter.
holder_outer_diameter = sensor_cavity_diameter + 2 * holder_wall; // Derived cup outside diameter.
port_center_z = closed_end_thickness - tube_bore_diameter / 2 + gas_passage_overlap; // Height of tube bores above the floor.

module co_sensor_holder() {
    difference() {
        union() {
            cylinder(h = holder_height, d = holder_outer_diameter, $fn = 96);

            // Opposed hollow spigots open into the cup just above its closed end.
            for (side = [-1, 1])
                translate([side * (holder_outer_diameter / 2 + tube_spigot_length / 2 - spigot_overlap / 2),
                           0, port_center_z])
                    rotate([0, 90, 0])
                        cylinder(h = tube_spigot_length + spigot_overlap,
                                 d = tube_spigot_diameter, center = true, $fn = 48);
        }

        // Sensor inserts through the top; the bottom thickness remains closed.
        translate([0, 0, closed_end_thickness])
            cylinder(h = holder_height - closed_end_thickness + 0.1,
                     d = sensor_cavity_diameter, $fn = 96);

                // One continuous round passage connects both spigots beneath the seated sensor.
        translate([0, 0, port_center_z])
            rotate([0, 90, 0])
                cylinder(h = holder_outer_diameter + 2 * tube_spigot_length + 1,
                         d = tube_bore_diameter, center = true, $fn = 48);
    }
}

co_sensor_holder();
