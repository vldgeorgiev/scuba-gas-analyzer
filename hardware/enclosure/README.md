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

## Export

Run `sh hardware/enclosure/export_parts.sh` from the repository root. It exports the base, lid, combined CO/He holder, O2 holder, mushroom insert, and snap-in wake-button pin into `exports/`; generated STLs are ignored by Git. Pass an output directory as the first argument or override the executable with `OPENSCAD`.
