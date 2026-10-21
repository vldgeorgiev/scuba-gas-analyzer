// Insert from outside the lid with the split-stem barbs facing left/right; no keeper or O-ring is needed.
// Dimensions are millimeters; print with the external head flat on the bed at Z = 0.
lid_thickness = 2.4; // Match the enclosure lid plate thickness.
lid_hole_diameter = 4; // Match the wake-button opening in the lid.
lid_relief_diameter = 5.4; // Match the underside clearance around the opening.
button_recess = 4; // Distance from lid exterior to the resting onboard switch top.
button_rest_gap = 0.2; // Clearance between the plunger tip and resting switch.
button_travel = 0.5; // Head-to-lid clearance at rest and maximum press stroke.

head_diameter = 5.2; // External head diameter; overlaps the hole to stop inward movement.
head_thickness = 1.2; // Head height, including the solid flexure root.
head_edge_chamfer = 0.2; // Bevel around the exposed head edge.
stem_diameter = 3.6; // Sliding stem diameter; smaller than the lid hole.
flex_slot_width = 1.2; // Gap between the two flexible halves, allowing them to squeeze together.
barb_diameter = 4.6; // Retaining width across X; slightly larger than the lid hole.
barb_ramp_height = 1; // Taper from the retaining shoulder toward the insertion tip.
tip_chamfer = 0.2; // Small bevel on the two switch-contact tips.

barb_shoulder_z = head_thickness + lid_thickness + button_travel; // Retaining shoulders in head-down print coordinates.
pin_height = head_thickness + button_travel + button_recess - button_rest_gap; // Tip position in head-down print coordinates.

assert(lid_thickness > 0 && button_recess > lid_thickness);
assert(button_rest_gap >= 0 && button_travel > button_rest_gap);
assert(head_diameter > lid_hole_diameter);
assert(head_edge_chamfer > 0 && head_edge_chamfer < head_thickness);
assert(stem_diameter > 0 && stem_diameter < lid_hole_diameter);
assert(flex_slot_width > barb_diameter - lid_hole_diameter
    && flex_slot_width < stem_diameter);
assert(barb_diameter > lid_hole_diameter && barb_diameter < lid_relief_diameter);
assert(barb_ramp_height > 0 && tip_chamfer > 0
    && barb_shoulder_z + barb_ramp_height < pin_height - tip_chamfer,
    "Barb ramps must finish before the switch-contact tips.");

module wake_button_pin() {
    difference() {
        union() {
            rotate_extrude($fn = 64)
                polygon([[0, 0],
                         [head_diameter / 2 - head_edge_chamfer, 0],
                         [head_diameter / 2, head_edge_chamfer],
                         [head_diameter / 2, head_thickness],
                         [stem_diameter / 2, head_thickness],
                         [stem_diameter / 2, pin_height - tip_chamfer],
                         [stem_diameter / 2 - tip_chamfer, pin_height],
                         [0, pin_height]]);
            intersection() {
                translate([0, 0, barb_shoulder_z])
                    cylinder(h = barb_ramp_height, d1 = barb_diameter,
                             d2 = stem_diameter - 0.1, $fn = 64);
                translate([-barb_diameter / 2, -stem_diameter / 2, barb_shoulder_z])
                    cube([barb_diameter, stem_diameter, barb_ramp_height]);
            }
        }
        translate([-flex_slot_width / 2, -head_diameter / 2 - 0.1, head_thickness])
            cube([flex_slot_width, head_diameter + 0.2, pin_height]);
    }
}

wake_button_pin();