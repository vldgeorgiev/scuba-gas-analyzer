// Select "assembly" (or "") for preview, or "bottom" / "lid" for individual STL exports.
part = ""; // Which enclosure piece to display or export.
assembly_explode_gap = 40; // Vertical separation above the base in the assembly preview.

// Dimensions are millimeters; enclosure size is set independently of PCB placement.
outer_width = 107; // Manually set enclosure footprint width along X.
outer_height = 68; // Manually set enclosure footprint height along Y.
outer_corner_radius = 3; // Radius of the vertical exterior corners; zero keeps square corners.
outer_edge_chamfer = 1; // Exterior base-bottom and lid-top bevel; zero disables it.
wall = 2.4; // Enclosure wall thickness.

floor_thickness = 2.4; // Base floor thickness.
base_height = 33; // Vertical base height along Z, including the floor.
lid_thickness = 2.4; // Flat lid plate thickness.

lip_depth = 2.5; // Depth of the lid's locating rails into the base.
lip_width = 1.5; // Thickness of the lid's locating rails.
fit_clearance = 0.2; // Side clearance between the lid rails and base interior.

// Corner screws enter from below through the base and thread into the lid bosses.
screw_offset = 5; // Each corner screw center's distance from the nearest enclosure edges.
screw_boss_radius = 3.6; // Radius of the base posts and lid screw bosses.
screw_hole_diameter = 3.5; // Clearance-hole diameter through the base.
screw_head_diameter = 6.5; // Diameter of the underside screw-head counterbore.
screw_head_depth = 1.8; // Depth of the underside screw-head counterbore.
lid_screw_pilot_diameter = 2.5; // Pilot-hole diameter in the lid bosses.
lid_top_cap = 1.2; // Solid material left above each blind lid pilot hole.
post_clearance = 1; // Vertical gap between the base posts and lid bosses when closed.

pcb_offset_x = wall + 7; // PCB lower-left X position from the enclosure exterior origin.
pcb_offset_y = wall + 0; // PCB lower-left Y position from the enclosure exterior origin.
pcb_support_positions = [[7, 52], [40, 52], [16, 8]]; // Support-hole centers from the PCB lower-left corner.
pcb_support_diameter = 5; // Outside diameter of each PCB support.
pcb_support_hole_diameter = 2; // Screw-hole diameter in each PCB support.
pcb_to_screen_top = 20.5; // Measured distance from PCB underside to screen top.

// Provisional LilyGo openings; adjust dimensions and centers to match the installed board.
screen_window_width = 32.2; // Display opening width.
screen_window_height = 55.4; // Display opening height along Y.
screen_corner_radius = 5; // Radius of the display opening's corners.
screen_rear_lid_thickness = 1.2; // Lid material remaining above the rear underside recess.
screen_rear_clearance = 1.5; // Recess extension beyond the screen opening toward the rear (opposite USB-C).
screen_rear_clearance_width = 26; // Rear underside recess width, centered on the screen.
screen_center_x = 13; // Display center X relative to the PCB lower-left corner.
screen_center_y = 32.5; // Display center Y relative to the PCB lower-left corner.

wake_button_diameter = 4; // Diameter of the single wake-button opening.
wake_button_flange_clearance_diameter = 5.4; // Underside rail relief for the snap-in pin's barbs.
wake_button_center_x = 22; // Wake-button center X relative to the PCB lower-left corner.
wake_button_center_y = 2; // Wake-button center Y relative to the PCB lower-left corner.

// Approximate front-edge connector centers: [PCB-local X, Y, height above PCB underside].
usb_c_position = [13, 0, 16.5]; // USB-C opening center; access extends toward the front wall (Y = 0).
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

// Rear-wall gas inlet, opposite the front-wall USB-C; independent of PCB placement.
gas_inlet_center_x = 58; // Inlet center across the rear wall.
gas_inlet_center_z = base_height / 2 - 2; // Inlet center height above the exterior bottom.
gas_inlet_socket_diameter = 6; // External supply tube socket diameter.
gas_inlet_socket_depth = 5; // External supply tube insertion depth from the rear exterior face.
gas_inlet_boss_diameter = 10; // Outside diameter around the socket.
gas_inlet_shoulder_depth = 1.5; // Material beyond the socket forming the tube stop.
gas_inlet_spigot_diameter = 5; // Internal spigot outside diameter for flexible tubing.
gas_inlet_spigot_length = 6; // Internal spigot length beyond the socket boss.
gas_inlet_bore_diameter = 3.5; // Continuous gas passage from socket to internal spigot.

lid_logo_width = 40; // Logo width along Y when viewed below the screen.
lid_logo_depth = 0.5; // Engraving depth into the lid's exterior face.

inner_width = outer_width - 2 * wall; // Derived interior footprint width along X.
inner_height = outer_height - 2 * wall; // Derived interior footprint height along Y.
lid_support_depth = (base_height - floor_thickness) / 2; // Lid boss depth before allowing clearance at the base posts.
base_post_height = base_height - floor_thickness - lid_support_depth - post_clearance; // Base post height below the lid bosses.
pcb_support_height = base_height + lid_thickness - floor_thickness - pcb_to_screen_top; // Support height for a screen flush with the lid top.
pcb_z = floor_thickness + pcb_support_height; // PCB underside height above the enclosure exterior bottom.
lid_rail_start = screw_offset + screw_boss_radius + fit_clearance; // Rail ends clear the corner bosses.
lid_rail_width = outer_width - 2 * lid_rail_start; // Front and rear rail length.
lid_rail_height = outer_height - 2 * lid_rail_start; // Side rail length.
lid_rail_edge = wall + fit_clearance; // Rail offset from the enclosure exterior.
lilygo_lip_clearance_width = screen_window_width; // Front rail gap width; adjust to the LilyGo body width.
gas_inlet_boss_depth = max(wall, gas_inlet_socket_depth + gas_inlet_shoulder_depth); // Derived inward boss depth.
lid_logo_center_x = (pcb_offset_x + screen_center_x + screen_window_width / 2 + outer_width) / 2; // Center between the screen and opposite lid edge.
lid_logo_center_y = outer_height / 2; // Center across the lid.

assert(part == "" || part == "assembly" || part == "bottom" || part == "lid",
    "Part must be assembly, bottom, lid, or an empty string for assembly.");
assert(assembly_explode_gap >= 0);
assert(wall > 0 && inner_width > 0 && inner_height > 0);
assert(floor_thickness > 0 && base_height > floor_thickness && lid_thickness > 0);
assert(outer_edge_chamfer >= 0 && outer_edge_chamfer < min(wall, floor_thickness, lid_thickness));
assert(lip_depth > 0 && lip_width > 0 && fit_clearance >= 0);
assert(lid_rail_width > 0 && lid_rail_height > 0
    && inner_width > 2 * (fit_clearance + lip_width)
    && inner_height > 2 * (fit_clearance + lip_width));
assert(post_clearance >= 0 && base_post_height > 0 && lid_support_depth > 0);
assert(lid_top_cap > 0 && lid_top_cap < lid_thickness);
assert(pcb_to_screen_top > 0 && pcb_support_height > 0,
    "PCB-to-screen height must be positive and leave room for PCB supports above the floor.");
assert(pcb_support_hole_diameter > 0 && pcb_support_hole_diameter < pcb_support_diameter);
assert(screen_window_width > 0 && screen_window_height > 0 && screen_corner_radius >= 0
    && 2 * screen_corner_radius < min(screen_window_width, screen_window_height));
assert(screen_rear_lid_thickness > 0 && screen_rear_lid_thickness <= lid_thickness
    && screen_rear_clearance > 0 && screen_rear_clearance_width > 0);
assert(lilygo_lip_clearance_width > 0);
assert(wake_button_diameter > 0 && wake_button_flange_clearance_diameter >= wake_button_diameter);
assert(side_label_size > 0 && side_label_depth > 0 && side_label_depth < wall && side_label_gap >= 0);
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
assert(lid_logo_width > 0 && lid_logo_depth > 0 && lid_logo_depth < lid_thickness);

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
    front_label("Off/On    ", on_off_position, on_off_height);
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

module enclosure_outline(inset = 0) {
    assert(outer_corner_radius >= 0 && 2 * outer_corner_radius < min(outer_width, outer_height));
    offset(delta = -inset)
        if (outer_corner_radius == 0) {
            square([outer_width, outer_height]);
        } else {
            translate([outer_corner_radius, outer_corner_radius])
                offset(r = outer_corner_radius, $fn = 48)
                    square([outer_width - 2 * outer_corner_radius,
                            outer_height - 2 * outer_corner_radius]);
        }
}

module rounded_enclosure_body(height, bottom_chamfer = 0, top_chamfer = 0) {
    assert(bottom_chamfer >= 0 && top_chamfer >= 0
        && bottom_chamfer + top_chamfer < height);
    if (bottom_chamfer == 0 && top_chamfer == 0) {
        linear_extrude(height = height) enclosure_outline();
    } else {
        slice_height = 0.001;
        hull() {
            linear_extrude(height = slice_height)
                enclosure_outline(bottom_chamfer);
            translate([0, 0, bottom_chamfer])
                linear_extrude(height = height - bottom_chamfer - top_chamfer)
                    enclosure_outline();
            translate([0, 0, height - slice_height])
                linear_extrude(height = slice_height)
                    enclosure_outline(top_chamfer);
        }
    }
}

module bottom() {
    difference() {
        union() {
            difference() {
                rounded_enclosure_body(base_height, bottom_chamfer = outer_edge_chamfer);
                translate([wall, wall, floor_thickness])
                    cube([inner_width, inner_height, base_height]);
            }

            screw_positions()
                translate([0, 0, floor_thickness])
                    cylinder(h = base_post_height,
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

module lid_logo_engraving() {
    translate([lid_logo_center_x, lid_logo_center_y, lid_thickness - lid_logo_depth])
        linear_extrude(height = lid_logo_depth + 0.1)
            rotate(90)
                resize([lid_logo_width, 0], auto = true)
                    import("logo.svg", center = true);
}

module lid_rails() {
    difference() {
        translate([lid_rail_start, lid_rail_edge, -lip_depth])
            cube([lid_rail_width, lip_width, lip_depth]);
        translate([pcb_offset_x + screen_center_x - lilygo_lip_clearance_width / 2,
                   lid_rail_edge - 0.1, -lip_depth - 0.1])
            cube([lilygo_lip_clearance_width, lip_width + 0.2, lip_depth + 0.2]);
    }
    translate([lid_rail_start, outer_height - lid_rail_edge - lip_width, -lip_depth])
        cube([lid_rail_width, lip_width, lip_depth]);
    translate([lid_rail_edge, lid_rail_start, -lip_depth])
        cube([lip_width, lid_rail_height, lip_depth]);
    translate([outer_width - lid_rail_edge - lip_width, lid_rail_start, -lip_depth])
        cube([lip_width, lid_rail_height, lip_depth]);
}

module lid_bosses() {
    intersection() {
        screw_positions()
            translate([0, 0, -lid_support_depth])
                cylinder(h = lid_support_depth, r = screw_boss_radius, $fn = 48);
        translate([wall + fit_clearance, wall + fit_clearance, -lid_support_depth])
            cube([inner_width - 2 * fit_clearance,
                  inner_height - 2 * fit_clearance, lid_support_depth]);
    }
}

module screen_opening() {
    pcb_xy()
        translate([screen_center_x - screen_window_width / 2 + screen_corner_radius,
                   screen_center_y - screen_window_height / 2 + screen_corner_radius, -0.1])
            linear_extrude(height = lid_thickness + 0.2)
                offset(r = screen_corner_radius)
                    square([screen_window_width - 2 * screen_corner_radius,
                            screen_window_height - 2 * screen_corner_radius]);
}

module screen_rear_recess() {
    pcb_xy()
        translate([screen_center_x - screen_rear_clearance_width / 2,
                   screen_center_y + screen_window_height / 2 - screen_corner_radius,
                   -lip_depth - 0.1])
            cube([screen_rear_clearance_width, screen_corner_radius + screen_rear_clearance,
                  lip_depth + 0.1 + lid_thickness - screen_rear_lid_thickness]);
}

module wake_button_openings() {
    pcb_xy()
        translate([wake_button_center_x, wake_button_center_y, -lip_depth - 0.1])
            cylinder(h = lip_depth + lid_thickness + 0.2, d = wake_button_diameter, $fn = 48);

    pcb_xy()
        translate([wake_button_center_x, wake_button_center_y, -lip_depth - 0.1])
            cylinder(h = lip_depth + 0.1, d = wake_button_flange_clearance_diameter, $fn = 48);
}

module lid() {
    difference() {
        union() {
            rounded_enclosure_body(lid_thickness, top_chamfer = outer_edge_chamfer);
            lid_rails();
            lid_bosses();
        }

        screw_positions()
            translate([0, 0, -lid_support_depth - 0.1])
                cylinder(h = lid_support_depth + lid_thickness - lid_top_cap + 0.1,
                         d = lid_screw_pilot_diameter, $fn = 48);

        screen_opening();
        screen_rear_recess();
        wake_button_openings();
        lid_logo_engraving();
    }
}

if (part == "bottom") {
    bottom();
} else if (part == "lid") {
    lid();
} else {
    bottom();
    translate([0, 0, base_height + assembly_explode_gap]) lid();
}