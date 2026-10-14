#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output_dir=${1:-"$script_dir/exports"}
openscad=${OPENSCAD:-/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD}
model="$script_dir/enclosure.scad"
co_holder="$script_dir/co_sensor_holder.scad"
he_holder="$script_dir/he_sensor_holder.scad"
o2_holder="$script_dir/o2_sensor_holder.scad"
flow_limiter="$script_dir/flow_limiter.scad"
button_pin="$script_dir/wake_button_pin.scad"

mkdir -p "$output_dir"
"$openscad" -D 'part="bottom"' -o "$output_dir/bottom.stl" "$model"
"$openscad" -D 'part="lid"' -o "$output_dir/lid.stl" "$model"
"$openscad" -o "$output_dir/co_sensor_holder.stl" "$co_holder"
"$openscad" -o "$output_dir/he_sensor_holder.stl" "$he_holder"
"$openscad" -o "$output_dir/o2_sensor_holder.stl" "$o2_holder"
"$openscad" -o "$output_dir/flow_limiter.stl" "$flow_limiter"
"$openscad" -o "$output_dir/wake_button_pin.stl" "$button_pin"
