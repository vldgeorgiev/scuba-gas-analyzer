#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output_dir=${1:-"$script_dir/exports"}
openscad=${OPENSCAD:-/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD}
model="$script_dir/enclosure.scad"
co_he_holder="$script_dir/co_he_sensor_holder.scad"
o2_holder="$script_dir/o2_sensor_holder.scad"
flow_limiter="$script_dir/flow_limiter.scad"
button_pin="$script_dir/wake_button_pin.scad"

mkdir -p "$output_dir"
output_dir=$(CDPATH= cd -- "$output_dir" && pwd)
"$openscad" -D 'part="bottom"' -o "$output_dir/bottom.stl" "$model"
"$openscad" -D 'part="lid"' -D 'lid_face_down=true' -o "$output_dir/lid.stl" "$model"
"$openscad" -D 'part="lid_logo"' -D 'lid_face_down=true' -o "$output_dir/lid_logo.stl" "$model"
python3 "$script_dir/package_lid_inlay.py" "$output_dir/lid.stl" "$output_dir/lid_logo.stl"
"$openscad" -o "$output_dir/co_he_sensor_holder.stl" "$co_he_holder"
"$openscad" -o "$output_dir/o2_sensor_holder.stl" "$o2_holder"
"$openscad" -o "$output_dir/flow_limiter.stl" "$flow_limiter"
"$openscad" -o "$output_dir/wake_button_pin.stl" "$button_pin"
