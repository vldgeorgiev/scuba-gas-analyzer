// Insert from beneath the lid, then fit an O-ring from above to retain the pin.
// Dimensions are millimeters; print with the switch-contact end at Z = 0.
lid_thickness = 2.4; // Match the enclosure lid plate thickness.
lid_hole_diameter = 4; // Match the wake-button opening in the lid.
button_recess = 4; // Distance from the lid top to the onboard button's resting top.
button_rest_gap = 0.2; // Clearance above the switch when the flange rests against the lid underside.
stem_diameter = 3.5; // Sliding stem diameter; smaller than the lid hole.
flange_diameter = 5; // Underside retaining flange diameter.
flange_thickness = 0.6; // Flange thickness below its lid-contact surface.
flange_front_extent = 1.5; // Flange reach toward negative Y; flat edge clears the front enclosure wall.
tip_above_lid = 2; // Pin projection above the lid at rest, including the groove and tip.
retainer_clearance = 0.4; // Distance from lid top to the groove's lower edge at rest.
oring_inner_diameter = 2.5; // Provisional retaining O-ring inside diameter.
oring_cross_section = 1; // Provisional retaining O-ring cord diameter.
groove_root_diameter = 2.6; // Diameter beneath the ring; slightly stretches the O-ring.
groove_width = 1.2; // Groove width along Z.
tip_chamfer = 0.2; // Small bevel at the exposed pin end.

flange_top_z = button_recess - lid_thickness - button_rest_gap; // Flange rests at the lid underside.
groove_start_z = flange_top_z + lid_thickness + retainer_clearance; // Groove lower edge in print coordinates.
pin_height = button_recess - button_rest_gap + tip_above_lid; // Overall pin length.

assert(button_rest_gap >= 0 && flange_top_z > flange_thickness);
assert(stem_diameter < lid_hole_diameter && flange_diameter > lid_hole_diameter);
assert(flange_front_extent > 0 && flange_front_extent <= flange_diameter / 2);
assert(oring_inner_diameter > 0 && oring_cross_section > 0);
assert(groove_root_diameter >= oring_inner_diameter && groove_root_diameter < stem_diameter);
assert(groove_root_diameter + 2 * oring_cross_section > lid_hole_diameter);
assert(retainer_clearance > 0 && groove_width >= oring_cross_section);
assert(tip_chamfer > 0 && groove_start_z + groove_width < pin_height - tip_chamfer);

module wake_button_pin() {
    difference() {
        union() {
            cylinder(h = pin_height - tip_chamfer, d = stem_diameter, $fn = 64);
            translate([0, 0, pin_height - tip_chamfer])
                cylinder(h = tip_chamfer, d1 = stem_diameter,
                         d2 = stem_diameter - 2 * tip_chamfer, $fn = 64);
            // Flat faces negative Y when installed, clearing the front wall beneath the lid.
            intersection() {
                translate([0, 0, flange_top_z - flange_thickness])
                    cylinder(h = flange_thickness, d = flange_diameter, $fn = 64);
                translate([-flange_diameter / 2, -flange_front_extent, 0])
                    cube([flange_diameter, flange_diameter, pin_height]);
            }
        }
        translate([0, 0, groove_start_z])
            difference() {
                cylinder(h = groove_width, d = stem_diameter + 1, $fn = 64);
                translate([0, 0, -0.1])
                    cylinder(h = groove_width + 0.2, d = groove_root_diameter, $fn = 64);
            }
    }
}

wake_button_pin();