# Thread Library

`threads.scad` is Ryan A. Colyer's threads-scad library (CC0), downloaded unchanged from:

https://github.com/rcolyer/threads-scad/blob/4ae9aeb3b136f9858200f77a304b909a000ce3b4/threads.scad

The O2 holder imports it with `use` to suppress the library's demo geometry.
Its `ScrewHole` tolerance includes the library's diameter corrections, not simply
a radial clearance. Verify M16x1 fit with the actual sensor and printer before use.