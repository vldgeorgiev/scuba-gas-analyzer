# Analyzer Enclosure

## Design Considerations

- Set enclosure dimensions independently, allowing space for the battery, sensors, wiring, and flexible gas tubing. Keep the battery clear of screw tips and sharp edges.
- PCB supports and screen, wake-button, USB-C, switch, coaxial jack, and potentiometer openings follow PCB placement. Support height is calculated from the measured PCB-underside-to-screen-top distance to keep the screen flush with the closed lid.
- Corner screws enter from underneath and engage blind lid bosses. Locating rails align the lid; the assembly preview is exploded, not the closed position.
- The gas inlet is opposite USB-C, with an external tube socket and an internal spigot. Supply only regulated, low-pressure sample gas, never cylinder pressure directly.
- The removable mushroom insert has a tip orifice and no O-ring groove. Tune its push fit with a test print; a ringless fit is not a guaranteed seal. Hand-held sampling next to a valve must remain unsealed and at ambient pressure: this insert is not a pressure regulator or tank-valve fitting, and does not establish a known flow rate. Avoid exposing it to a direct high-pressure jet; use an appropriate sampling arrangement and verify flow and readings.
- Separate CO and He holders can be connected with flexible tubes. The combined holder routes CO to He, with long-side He ports, a triangular flow divider, and an outlet-side temperature-sensor cutout. The divider does not guarantee uniform flow.
- The O2 holder screws onto the sensor and uses a central delivery tube with floor exhaust holes. Verify that the seated sensor does not obstruct the exhausts or contact the tube tip.
- Printed gas paths and sensor seats need leak checks, appropriate sealing, and oxygen-compatible materials. This enclosure is not designed as waterproof or pressure-rated. Validate sensor response and readings at the intended flow before relying on measurements.

## Screws

Provisional choices for screws that form threads directly in printed plastic:

| Location | Quantity | Starting Choice |
| --- | --- | --- |
| Lid corner bosses | 4 | 2.5 mm diameter x 25 mm long, thread-forming screws |
| PCB supports | 3 | 2.5 mm diameter x 6-8 mm long, thread-forming screws |

These are fit-test choices, not confirmed hardware specifications. Check pilot-hole fit, head diameter and height against the underside counterbores, and engagement after printing. Adjust screw length when support or enclosure height changes; never let screws pierce the lid, floor, or battery space. The printed holes are not modeled metric threads.

## Wake Button

Print `wake_button_pin.scad` with its switch-contact end on the bed. Insert it from beneath the lid with the flange's flat edge toward the front wall, then fit a provisional 2.5 mm ID x 1 mm cord O-ring into the exposed groove. The flange and ring retain the pin; the onboard switch supplies the return action. Test for free movement and no continuous switch pressure before use. The lid lip is relieved for the flange. Keep the pin's lid thickness and hole diameter parameters matched to the enclosure; adjust its button recess, rest gap, and ring groove after a fit-test print.

## Export

Run `sh hardware/enclosure/export_parts.sh` from the repository root. It exports the base, lid, separate sensor holders, mushroom insert, and wake-button pin into `exports/`; generated STLs are ignored by Git. Pass an output directory as the first argument or override the executable with `OPENSCAD`.

The combined CO/He holder currently needs a separate export from OpenSCAD. Print-fit and gas-flow tests remain necessary even when rendering succeeds.