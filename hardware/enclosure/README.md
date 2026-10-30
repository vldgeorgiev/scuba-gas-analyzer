# Analyzer Enclosure

## Design Considerations

- Set enclosure dimensions independently, allowing space for the battery, sensors, wiring, and flexible gas tubing. Keep the battery clear of screw tips and sharp edges.
- PCB supports and screen, wake-button, USB-C, switch, coaxial jack, and potentiometer openings follow PCB placement. Support height is calculated from the measured PCB-underside-to-screen-top distance to keep the screen flush with the closed lid.
- Corner screws enter from underneath and engage blind lid bosses. Locating rails align the lid; the assembly preview is exploded, not the closed position.
- The gas inlet is opposite USB-C, with an external tube socket and an internal spigot. Supply only regulated, low-pressure sample gas, never cylinder pressure directly.
- The removable mushroom insert has a tip orifice and no O-ring groove. Tune its push fit with a test print; a ringless fit is not a guaranteed seal. Hand-held sampling next to a valve must remain unsealed and at ambient pressure: this insert is not a pressure regulator or tank-valve fitting, and does not establish a known flow rate. Avoid exposing it to a direct high-pressure jet; use an appropriate sampling arrangement and verify flow and readings.
- The combined holder routes CO to He, with long-side He ports, a triangular flow divider, and an outlet-side temperature-sensor cutout.
- The O2 holder screws onto the sensor and uses a central delivery tube with floor exhaust holes. Verify that the seated sensor does not obstruct the exhausts or contact the tube tip.
- Printed gas paths and sensor seats need leak checks, appropriate sealing, and oxygen-compatible materials. This enclosure is not designed as waterproof or pressure-rated. Validate sensor response and readings at the intended flow before relying on measurements.

## Screws

Suggested thread-forming screws for printed plastic:

| Location | Quantity | Suggested Size |
| --- | --- | --- |
| Lid corner bosses | 4 | 2.5 mm diameter x 25 mm long, thread-forming screws |
| PCB supports | 3 | 2.5 mm diameter x 6-8 mm long, thread-forming screws |

Check pilot-hole fit, head diameter and height against the underside counterbores, and screw engagement. Match screw length to the support and enclosure dimensions; never let screws pierce the lid, floor, or battery space. The printed holes are not modeled metric threads.

## Wake Button

[wake_button_pin.scad](wake_button_pin.scad) defines a snap-in pin for the 4 mm lid opening. Print it with its external head flat on the bed, as exported, then insert it from outside the lid with the barbs facing left/right.

The external head stops inward escape; two split-stem barbs catch beneath the lid to stop outward escape. Clearance beneath the head allows pressing, and the onboard switch supplies the return force. The slot extends into the head so the two halves can flex, while a continuous face holds the pin together. The lid's underside relief provides clearance for the barbs.

PETG is preferable to brittle filament for the flexible stem.

## O2 Holder

A small 45-degree bevel softens the outer bottom edge of the holder floor while leaving the sensor seat flat.
The compact underside merges the 90-degree inlet into the floor with a small overlap. Beneath the floor, its support matches the inlet diameter in width, fills only the floor-facing upper half, and is clipped to the holder footprint. The outward-facing lower half retains the tube's circular profile, blending into the fully round exposed hose connection. Four fixed exhaust holes surround the central tube, leaving a wider gap around the inlet so their full vertical paths clear its exterior. There are no separate support ribs. The sensor thread, seat, and central delivery tube retain their original dimensions.
The exposed inlet stays round and extends beyond the holder perimeter for hose access. Its total reach combines the holder radius and the exposed hose-grip length. The inlet and its reinforcement share the same width, while the four exhausts remain clear of both.
Print with the threaded opening facing up. Local supports may still be needed under the exposed round inlet and floor overhangs; keep support out of the inlet bore, exhausts, and threads. The redesign reduces the below-floor gap but does not establish a support-free print.

## Export

Run `sh hardware/enclosure/export_parts.sh` from the repository root. It exports the base, lid, one-layer lid logo, combined CO/He holder, O2 holder, mushroom insert, and wake-button pin into `exports/`; generated STLs are ignored by Git. It also packages the lid and logo into `lid_inlay.3mf` using [package_lid_inlay.py](package_lid_inlay.py), preserving their CAD coordinates as two independently printable objects without recentering. OpenSCAD and Python 3 are required; override the OpenSCAD executable path with `OPENSCAD`. Pass an output directory as the first argument.

### Two-Color Lid Without AMS

The geometry packager overwrites an existing `lid_inlay.3mf`, including saved Bambu project settings and plate placement, with the generated geometry-only file. Save a tuned project under a different name if you want to keep it.

Open a configured 3MF as a project, not geometry-only, to load its process settings. The tuned inlay process uses 20 mm/s for both initial-layer and initial-layer infill speeds. Bambu's project configuration must record these keys in the process entry of `different_settings_to_system` and its base preset in `inherits_group`; changing only the numeric values can let GUI preset reconciliation restore the stock speeds.

The lid uses a flush, face-down inlay for the A1 mini without AMS: print the logo in one color first, leave it on the bed, then print the lid around and over it in another color. The logo is one 0.20 mm layer thick, with a matching recess in the lid. Both parts use the same nominal outline, with no press-fit clearance. Use compatible filaments of the same material, such as white PLA for the logo and black PLA for the lid.

[export_parts.sh](export_parts.sh) exports `lid.stl` and `lid_logo.stl` in the same coordinate system and exterior-face-down orientation: the exterior is at Z = 0, the logo occupies Z = 0 to 0.20 mm, and the lid covers its back above Z = 0.20 mm. Do not mirror the logo separately. The default CAD preview remains in assembly orientation; manual exports must use the same face-down orientation for both parts.

**Use [lid_inlay.3mf](exports/lid_inlay.3mf) for import into Bambu Studio**, rather than opening the two STLs separately. It contains two top-level objects, `lid` and `lid_logo`, with independent printable checkboxes and unchanged CAD coordinates. Do not combine them into a multipart object if prompted. Separate STL imports can be independently centered or arranged, losing alignment despite the files sharing CAD coordinates. The generated 3MF is a geometry container, not a printer/material profile or sliced print job; Bambu may report "invalid config, load geometry data only" because it has no Bambu project configuration. Load the geometry and select the A1 mini, your actual nozzle, plate, and filament settings, then save a Bambu project.

The shared logo outline expands by 0.01 mm to join point-touching traced contours into finite overlaps; the lid recess uses the identical outline. This is a mesh-topology repair, not a printable-detail thickening pass. The packager rejects meshes with open or non-manifold edges instead of exporting them into the 3MF.

Prepare two single-filament jobs from one aligned layout:

1. Import `lid_inlay.3mf` as two separate objects. If repositioning is needed, select both and move them together once. Do not independently center or auto-arrange them, or split the logo into individual islands.
2. Set **first-layer height and normal layer height to 0.20 mm** in both jobs. Disable adaptive layer height and raft. Keep the same plate origin, object placement, scale, and rotation in both jobs.
3. For the logo-only job, uncheck `lid` and leave `lid_logo` checked. For the lid-only job, check `lid` and uncheck `lid_logo`. Toggle printability without deleting, moving, or arranging either object. Confirm in the sliced preview that the first job contains exactly one layer, the second job's first layer leaves the logo footprint empty, and its second layer covers the logo's back.
4. Print the logo-only job, then change to the lid filament without removing the plate or logo. Maintain bed adhesion between jobs.
5. Print the lid-only job from its first layer at Z = 0.20 mm; do not start it at the second layer. Its first layer prints around the existing logo, then the following layers bond over it.

This is not an ordinary pause-at-layer color change: both colors occupy the first layer. Before running either job, inspect the A1 mini startup/end sequences and the second job's first-layer travels for contact with the existing logo. Repeated probing, nozzle wiping/purging, a prime line, or low travel moves must not cross or disturb it. Avoid a brim that overlaps the other part's footprint. The export script produces geometry only; it does not create or validate printer-specific G-code or automate filament changes.

The older `lid_black_white.3mf` is obsolete for this workflow: it contains the previous two-filament geometry and predates both the button clips and the 0.20 mm inlay. Use the regenerated `lid_inlay.3mf` instead.

Print-fit and gas-flow tests remain necessary even when rendering succeeds.`