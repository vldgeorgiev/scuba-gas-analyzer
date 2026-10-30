"""Package lid STLs as independently printable 3MF objects without recentering."""

from collections import Counter
import math
from pathlib import Path
import struct
import sys
from xml.etree import ElementTree as ET
from zipfile import ZIP_DEFLATED, ZipFile


CORE = "http://schemas.microsoft.com/3dmanufacturing/core/2015/02"
ET.register_namespace("", CORE)


def element(parent, tag, **attributes):
    return ET.SubElement(parent, f"{{{CORE}}}{tag}", attributes)


def read_vertices(path):
    data = path.read_bytes()
    if len(data) >= 84 and len(data) == 84 + 50 * struct.unpack_from("<I", data, 80)[0]:
        return [
            tuple(row[i:i + 3])
            for row in struct.iter_unpack("<12fH", data[84:])
            for i in (3, 6, 9)
        ]
    return [
        tuple(map(float, line.split()[1:]))
        for line in data.decode("ascii").splitlines()
        if line.strip().startswith("vertex ")
    ]


def package(lid, logo):
    output = lid.with_name("lid_inlay.3mf")
    if output.exists():
        with ZipFile(output) as archive:
            if "Metadata/project_settings.config" in archive.namelist():
                raise ValueError(
                    f"Refusing to overwrite saved Bambu settings in {output}. "
                    "Export geometry to a different directory and update the saved project."
                )
    model = ET.Element(f"{{{CORE}}}model", unit="millimeter")
    resources = element(model, "resources")
    for object_id, path in enumerate((lid, logo), 1):
        points = read_vertices(path)
        if (not points or len(points) % 3
                or any(len(p) != 3 or not all(map(math.isfinite, p)) for p in points)):
            raise ValueError(f"Invalid STL triangles: {path}")
        obj = element(resources, "object", id=str(object_id), type="model", name=path.stem)
        mesh = element(obj, "mesh")
        vertices = element(mesh, "vertices")
        triangles = element(mesh, "triangles")
        indices = {}
        for point in points:
            if point not in indices:
                indices[point] = len(indices)
                element(vertices, "vertex", **dict(zip(("x", "y", "z"), map(str, point))))
        edges = Counter()
        for start in range(0, len(points), 3):
            face = [indices[p] for p in points[start:start + 3]]
            if len(set(face)) != 3:
                raise ValueError(f"Degenerate STL triangle: {path}")
            for a, b in zip(face, face[1:] + face[:1]):
                edges[tuple(sorted((a, b)))] += 1
            element(triangles, "triangle", **{
                f"v{i + 1}": str(face[i]) for i in range(3)
            })
        invalid_edges = sum(count != 2 for count in edges.values())
        if invalid_edges:
            raise ValueError(f"STL has {invalid_edges} open or non-manifold edges: {path}")
    build = element(model, "build")
    for object_id in ("1", "2"):
        element(build, "item", objectid=object_id)
    with ZipFile(output, "w", compression=ZIP_DEFLATED) as archive:
        archive.writestr("[Content_Types].xml", (
            '<?xml version="1.0" encoding="UTF-8"?>'
            '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
            '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
            '<Default Extension="model" ContentType="application/vnd.ms-package.3dmanufacturing-3dmodel+xml"/>'
            '</Types>'
        ))
        archive.writestr("_rels/.rels", (
            '<?xml version="1.0" encoding="UTF-8"?>'
            '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
            '<Relationship Id="model" Target="/3D/3dmodel.model" '
            'Type="http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"/>'
            '</Relationships>'
        ))
        archive.writestr("3D/3dmodel.model", ET.tostring(model, encoding="utf-8", xml_declaration=True))
    print(f"Exported independently printable, aligned lid objects: {output}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("Usage: package_lid_inlay.py lid.stl lid_logo.stl")
    package(Path(sys.argv[1]), Path(sys.argv[2]))
