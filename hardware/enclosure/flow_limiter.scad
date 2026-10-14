// Removable mushroom inlet insert. No O-ring groove or pressure-rated valve connection.
// Dimensions are millimeters; Z = 0 is the plug end inserted into the enclosure.
socket_diameter = 6; // Match the enclosure inlet socket diameter.
plug_fit_adjustment = 0; // Diametral fit adjustment: positive tightens, negative loosens.
plug_length = 5; // Insertion length before the mushroom's flat underside meets the enclosure wall.
plug_chamfer_height = 0.5; // Lead-in chamfer length at the plug end.
plug_chamfer_reduction = 0.4; // Diameter reduction at the chamfer tip.
mushroom_diameter = 26; // Maximum mushroom head diameter.
mushroom_height = 10; // Height of the elliptical dome above its flat underside.
gas_bore_diameter = 3; // Main passage through the plug and most of the head.
restriction_diameter = 2; // Only intentional flow restriction, at the mushroom tip.
restriction_length = 1.5; // Axial length of the small tip bore.

plug_diameter = socket_diameter + plug_fit_adjustment; // Derived push-fit stem diameter.
head_z = plug_length; // Flat head underside seats directly against the enclosure wall.
total_height = head_z + mushroom_height; // Tip height above the plug end.

assert(plug_chamfer_height > 0 && plug_chamfer_height < plug_length);
assert(plug_chamfer_reduction > 0 && plug_diameter - plug_chamfer_reduction > gas_bore_diameter);
assert(mushroom_diameter > plug_diameter && mushroom_height > 0);
assert(restriction_diameter > 0 && restriction_diameter < gas_bore_diameter);
assert(restriction_length > 0 && restriction_length < mushroom_height);

module flow_limiter() {
    difference() {
        union() {
            cylinder(h = plug_chamfer_height,
                     d1 = plug_diameter - plug_chamfer_reduction, d2 = plug_diameter, $fn = 96);
            translate([0, 0, plug_chamfer_height])
                cylinder(h = plug_length - plug_chamfer_height, d = plug_diameter, $fn = 96);

            // Revolved elliptical cap gives the mushroom a rounded nose and flat underside.
            translate([0, 0, head_z])
                rotate_extrude($fn = 128)
                    polygon(concat([[0, 0]],
                                   [for (angle = [0 : 2 : 90])
                                       [mushroom_diameter / 2 * cos(angle),
                                        mushroom_height * sin(angle)]]));
        }

        // A short tip restriction feeds a larger, continuous passage toward the enclosure.
        translate([0, 0, -0.1])
            cylinder(h = total_height - restriction_length + 0.1,
                     d = gas_bore_diameter, $fn = 64);
        translate([0, 0, total_height - restriction_length - 0.1])
            cylinder(h = restriction_length + 0.2, d = restriction_diameter, $fn = 64);
    }
}

flow_limiter();