// Select "assembly" for preview, or "bottom" / "lid" for individual STL exports.
part = ""; // Which enclosure piece to display or export.

// Dimensions are millimeters; enclosure size is set independently of PCB placement.
outer_width = 110; // Manually set enclosure footprint width along X.
outer_height = 70; // Manually set enclosure footprint height along Y.
outer_corner_radius = 3; // Radius of the vertical exterior corners; zero keeps square corners.
wall = 2.4; // Enclosure wall thickness.

floor_thickness = 2.4; // Base floor thickness.
base_height = 34; // Vertical base height along Z, including the floor.
lid_thickness = 2.4; // Flat lid plate thickness.
lip_depth = 2.5; // Depth of the lid's locating rails into the base.
lip_width = 1.5; // Thickness of the lid's locating rails.
lid_support_depth = (base_height - floor_thickness) / 2; // Lid screw-boss depth; paired with equal-height base posts.
fit_clearance = 0.3; // Side clearance between the lid rails and base interior.

pcb_offset_x = wall + 8; // PCB lower-left X position from the enclosure exterior origin.
pcb_offset_y = wall + 0; // PCB lower-left Y position from the enclosure exterior origin.
pcb_support_positions = [[7, 52 ], [40, 52], [16, 8]]; // Support-hole centers from the PCB lower-left corner.
pcb_support_diameter = 5; // Outside diameter of each PCB support.
pcb_support_hole_diameter = 2; // Screw-hole diameter in each PCB support.
pcb_to_screen_top = 20.5; // Measured distance from PCB underside to screen top.
pcb_support_height = base_height + lid_thickness - floor_thickness - pcb_to_screen_top; // Automatic support height for a screen flush with the closed lid top.

// Corner screws enter from below through the base and thread into the lid bosses.
screw_offset = 5; // Each corner screw center's distance from the nearest enclosure edges.
screw_boss_radius = 3.6; // Radius of the base posts and lid screw bosses.
screw_hole_diameter = 3.5; // Clearance-hole diameter through the base.
screw_head_diameter = 6.5; // Diameter of the underside screw-head counterbore.
screw_head_depth = 1.8; // Depth of the underside screw-head counterbore.
lid_screw_pilot_diameter = 2.5; // Pilot-hole diameter in the lid bosses.
lid_top_cap = 1.2; // Solid material left above each blind lid pilot hole.

inner_width = outer_width - 2 * wall; // Derived interior footprint width along X.
inner_height = outer_height - 2 * wall; // Derived interior footprint height along Y.
pcb_z = floor_thickness + pcb_support_height; // PCB underside height above the enclosure exterior bottom.

assert(pcb_to_screen_top > 0 && pcb_support_height > 0,
    "PCB-to-screen height must be positive and leave room for PCB supports above the floor.");

// Provisional LilyGo openings; adjust dimensions and centers to match the installed board.
screen_window_width = 32.2; // Display opening width.
screen_window_height = 55.4; // Display opening height along Y.
screen_corner_radius = 5; // Radius of the display opening's corners.
screen_rear_lid_thickness = 1.2; // Lid material remaining above the rear underside recess.
screen_rear_clearance = 2; // Recess extension beyond the screen opening toward the rear (opposite USB-C).
screen_center_x = 13; // Display center X relative to the PCB lower-left corner.
screen_center_y = 32.5; // Display center Y relative to the PCB lower-left corner.
wake_button_diameter = 4; // Diameter of the single wake-button opening.
wake_button_flange_clearance_diameter = 5.4; // Lip relief around the push-pin's underside flange.
wake_button_center_x = 22; // Wake-button center X relative to the PCB lower-left corner.
wake_button_center_y = 2; // Wake-button center Y relative to the PCB lower-left corner.

lid_logo_width = 40; // Logo width along Y when viewed below the screen.
lid_logo_depth = 0.5; // Engraving depth into the lid's exterior face.
lid_logo_center_x = (pcb_offset_x + screen_center_x + screen_window_width / 2 + outer_width) / 2; // Center between the screen edge and the opposite lid edge.
lid_logo_center_y = outer_height / 2; // Center across the lid.

// Approximate front-edge connector centers: [PCB-local X, Y, height above PCB underside].
usb_c_position = [12.5, 0, 16.5]; // USB-C opening center; access extends toward the front wall (Y = 0).
usb_c_width = 10; // USB-C opening width along X.
usb_c_height = 6; // USB-C opening height along Z.
on_off_position = [20, 0, 2.2]; // Hardware switch opening center relative to the PCB.
on_off_width = 6; // Hardware switch opening width along X.
on_off_height = 3; // Hardware switch opening height along Z.
coax_position = [31.5, 0, 3.5]; // Coaxial jack opening center relative to the PCB.
coax_hole_diameter = 6; // Coaxial jack access-hole diameter.
potentiometer_position = [42, 0, 10.5]; // Provisional potentiometer opening center relative to the PCB.
potentiometer_hole_diameter = 3; // Round potentiometer access-hole diameter.

side_label_size = 3; // Engraved text size in millimeters.
side_label_depth = 0.5; // Engraving depth into the front wall.
side_label_gap = 1; // Space between the opening bottom and the label top.
side_label_font = "Liberation Sans:style=Bold"; // Font for printable side labels.

assert(side_label_size > 0 && side_label_depth > 0 && side_label_depth < wall && side_label_gap >= 0);

// Rear-wall gas inlet, opposite the front-wall USB-C; independent of PCB placement.
gas_inlet_center_x = outer_width - 49; // Inlet center across the rear wall.
gas_inlet_center_z = base_height / 2; // Inlet center height above the exterior bottom.
gas_inlet_socket_diameter = 6; // External supply tube socket diameter.
gas_inlet_socket_depth = 5; // External supply tube insertion depth from the rear exterior face.
gas_inlet_boss_diameter = 10; // Outside diameter around the socket.
gas_inlet_shoulder_depth = 1.5; // Material beyond the socket forming the tube stop.
gas_inlet_spigot_diameter = 5; // Internal spigot outside diameter for flexible tubing.
gas_inlet_spigot_length = 6; // Internal spigot length beyond the socket boss.
gas_inlet_bore_diameter = 3.5; // Continuous gas passage from socket to internal spigot.
gas_inlet_boss_depth = max(wall, gas_inlet_socket_depth + gas_inlet_shoulder_depth); // Derived inward boss depth.

assert(gas_inlet_bore_diameter > 0
       && gas_inlet_bore_diameter < gas_inlet_spigot_diameter
       && gas_inlet_bore_diameter < gas_inlet_socket_diameter
       && gas_inlet_socket_diameter < gas_inlet_boss_diameter);
assert(gas_inlet_socket_depth > 0 && gas_inlet_shoulder_depth > 0 && gas_inlet_spigot_length > 0);
assert(gas_inlet_center_x - gas_inlet_boss_diameter / 2 > wall
       && gas_inlet_center_x + gas_inlet_boss_diameter / 2 < outer_width - wall
       && gas_inlet_center_z - gas_inlet_boss_diameter / 2 > floor_thickness
       && gas_inlet_center_z + gas_inlet_boss_diameter / 2 < base_height - lip_depth,
       "Gas inlet boss must fit inside the rear wall below the lid rails.");

module gas_inlet_body() {
    translate([gas_inlet_center_x, outer_height, gas_inlet_center_z])
        rotate([90, 0, 0]) {
            cylinder(h = gas_inlet_boss_depth, d = gas_inlet_boss_diameter, $fn = 64);
            translate([0, 0, gas_inlet_boss_depth - 0.1])
                cylinder(h = gas_inlet_spigot_length + 0.1,
                         d = gas_inlet_spigot_diameter, $fn = 48);
        }
}

module gas_inlet_openings() {
    translate([gas_inlet_center_x, outer_height, gas_inlet_center_z])
        rotate([90, 0, 0]) {
            translate([0, 0, -0.1])
                cylinder(h = gas_inlet_socket_depth + 0.1,
                         d = gas_inlet_socket_diameter, $fn = 64);
            translate([0, 0, -0.1])
                cylinder(h = gas_inlet_boss_depth + gas_inlet_spigot_length + 0.2,
                         d = gas_inlet_bore_diameter, $fn = 48);
        }
}

module pcb_xy() {
    translate([pcb_offset_x, pcb_offset_y, 0]) children();
}

module front_rectangular_opening(position, width, height) {
    access_depth = pcb_offset_y + position[1] + wall + 0.2;
    pcb_xy()
        translate([position[0] - width / 2, position[1] - access_depth,
                   pcb_z + position[2] - height / 2])
            cube([width, access_depth + 0.1, height]);
}

module front_round_opening(position, diameter) {
    pcb_xy()
        translate([position[0], position[1] + 0.1, pcb_z + position[2]])
            rotate([90, 0, 0])
                cylinder(h = pcb_offset_y + position[1] + wall + 0.3,
                         d = diameter, $fn = 48);
}

module pcb_side_openings() {
    front_rectangular_opening(usb_c_position, usb_c_width, usb_c_height);
    front_rectangular_opening(on_off_position, on_off_width, on_off_height);
    front_round_opening(coax_position, coax_hole_diameter);
    front_round_opening(potentiometer_position, potentiometer_hole_diameter);
}

module front_label(value, position, opening_height) {
    translate([pcb_offset_x + position[0], side_label_depth,
               pcb_z + position[2] - opening_height / 2 - side_label_gap])
        rotate([90, 0, 0])
            linear_extrude(height = side_label_depth + 0.1)
                text(value, size = side_label_size, font = side_label_font,
                     halign = "center", valign = "top", $fn = 32);
}

module pcb_side_labels() {
    front_label("On/Off    ", on_off_position, on_off_height);
    front_label("He", potentiometer_position, potentiometer_hole_diameter);
    front_label("    Ext O2", coax_position, coax_hole_diameter);
}

// Shared corner locations keep the base posts and lid bosses aligned.
module screw_positions() {
    for (x = [screw_offset, outer_width - screw_offset])
        for (y = [screw_offset, outer_height - screw_offset])
            translate([x, y]) children();
}

// Support positions are measured from the PCB's lower-left corner.
module pcb_supports() {
        pcb_xy()
            for (position = pcb_support_positions)
                translate([position[0], position[1], floor_thickness])
            difference() {
                cylinder(h = pcb_support_height, d = pcb_support_diameter, $fn = 48);
                translate([0, 0, -0.1])
                    cylinder(h = pcb_support_height + 0.2,
                             d = pcb_support_hole_diameter, $fn = 48);
            }
}

module rounded_enclosure_body(height) {
    assert(outer_corner_radius >= 0 && 2 * outer_corner_radius < min(outer_width, outer_height));
    if (outer_corner_radius == 0) {
        cube([outer_width, outer_height, height]);
    } else {
        linear_extrude(height = height)
            translate([outer_corner_radius, outer_corner_radius])
                offset(r = outer_corner_radius, $fn = 48)
                    square([outer_width - 2 * outer_corner_radius,
                            outer_height - 2 * outer_corner_radius]);
    }
}

module bottom() {
    difference() {
        union() {
            difference() {
                rounded_enclosure_body(base_height);
                translate([wall, wall, floor_thickness])
                    cube([inner_width, inner_height, base_height]);
            }

            screw_positions()
                translate([0, 0, floor_thickness])
                    cylinder(h = base_height - floor_thickness - lid_support_depth,
                             r = screw_boss_radius, $fn = 48);

            pcb_supports();
            gas_inlet_body();
        }

        screw_positions() {
            translate([0, 0, -0.1])
                cylinder(h = base_height + 0.2, d = screw_hole_diameter, $fn = 48);
            // Counterbores are on the bottom face so screw heads sit underneath the enclosure.
            translate([0, 0, -0.1])
                cylinder(h = screw_head_depth + 0.1, d = screw_head_diameter, $fn = 48);
        }

        pcb_side_openings();
        pcb_side_labels();
        gas_inlet_openings();
    }
}

lilygo_lip_clearance_width = screen_window_width; // Front rail gap width; adjust to the LilyGo body width.

module lid_logo_engraving() {
    assert(lid_logo_width > 0 && lid_logo_depth > 0 && lid_logo_depth < lid_thickness);
    translate([lid_logo_center_x, lid_logo_center_y, lid_thickness - lid_logo_depth])
        linear_extrude(height = lid_logo_depth + 0.1)
            rotate(90)
                resize([lid_logo_width, 0], auto = true)
                    import("logo.svg", center = true);
}

module lid() {
    difference() {
        union() {
            rounded_enclosure_body(lid_thickness);

            // Four short rails fit inside the base, leaving clearance around the corner supports.
            rail_start = screw_offset + screw_boss_radius + fit_clearance;
            rail_width = outer_width - 2 * rail_start;
            rail_height = outer_height - 2 * rail_start;
            rail_edge = wall + fit_clearance;
            // Omit the front rail across the LilyGo width without cutting the lid plate or bosses.
            difference() {
                translate([rail_start, rail_edge, -lip_depth])
                    cube([rail_width, lip_width, lip_depth]);
                translate([pcb_offset_x + screen_center_x - lilygo_lip_clearance_width / 2,
                           rail_edge - 0.1, -lip_depth - 0.1])
                    cube([lilygo_lip_clearance_width, lip_width + 0.2, lip_depth + 0.2]);
            }
            translate([rail_start, outer_height - rail_edge - lip_width, -lip_depth])
                cube([rail_width, lip_width, lip_depth]);
            translate([rail_edge, rail_start, -lip_depth])
                cube([lip_width, rail_height, lip_depth]);
            translate([outer_width - rail_edge - lip_width, rail_start, -lip_depth])
                cube([lip_width, rail_height, lip_depth]);

            // Trim boss sides to clear the base walls when the lid is seated.
            intersection() {
                screw_positions()
                    translate([0, 0, -lid_support_depth])
                        cylinder(h = lid_support_depth, r = screw_boss_radius, $fn = 48);
                translate([wall + fit_clearance, wall + fit_clearance, -lid_support_depth])
                    cube([inner_width - 2 * fit_clearance,
                          inner_height - 2 * fit_clearance, lid_support_depth]);
            }
        }

        screw_positions()
            translate([0, 0, -lid_support_depth - 0.1])
                cylinder(h = lid_support_depth + lid_thickness - lid_top_cap + 0.1,
                         d = lid_screw_pilot_diameter, $fn = 48);

        // Rounded screen window through the lid plate.
                pcb_xy()
                    translate([screen_center_x - screen_window_width / 2 + screen_corner_radius,
                   screen_center_y - screen_window_height / 2 + screen_corner_radius, -0.1])
            linear_extrude(height = lid_thickness + 0.2)
                offset(r = screen_corner_radius)
                    square([screen_window_width - 2 * screen_corner_radius,
                            screen_window_height - 2 * screen_corner_radius]);

        // Rear screen clearance removes underside material without changing the exterior face.
        assert(screen_rear_lid_thickness > 0 && screen_rear_lid_thickness <= lid_thickness
             && screen_rear_clearance > 0);
         pcb_xy()
             translate([screen_center_x - screen_window_width / 2,
                  screen_center_y + screen_window_height / 2 - screen_corner_radius,
                  -lip_depth - 0.1])
              cube([screen_window_width, screen_corner_radius + screen_rear_clearance,
                 lip_depth + 0.1 + lid_thickness - screen_rear_lid_thickness]);

         // Clear the locating lip as well as the plate beneath the wake button.
        pcb_xy()
            translate([wake_button_center_x, wake_button_center_y, -lip_depth - 0.1])
                cylinder(h = lip_depth + lid_thickness + 0.2, d = wake_button_diameter, $fn = 48);

        // Relieve only the lip so the wider flange can reach the lid underside.
        pcb_xy()
            translate([wake_button_center_x, wake_button_center_y, -lip_depth - 0.1])
                cylinder(h = lip_depth + 0.1, d = wake_button_flange_clearance_diameter, $fn = 48);

        lid_logo_engraving();
    }
}

if (part == "bottom") {
    bottom();
} else if (part == "lid") {
    lid();
} else {
    bottom();
    translate([0, 0, base_height + 20]) lid();
}