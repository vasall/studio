#!/usr/bin/env python3
"""
Generate simple placeholder icons for Vasall Studio.

Every icon is a white shape with an alpha channel (the UI tints it), written
as <name>_16.png and <name>_32.png into assets/icons/. Only the standard
library is used. Replace the files with your own icon set at any time, the
names have to stay the same (see include/render/icons.h).

Usage: python3 tools/gen_placeholder_icons.py [output_dir]
"""

import math
import os
import struct
import sys
import zlib

SUPERSAMPLE = 4


# --- distance helpers (unit space: 0..16, y down) ----------------------------

def seg(px, py, ax, ay, bx, by):
    """Distance from point p to segment a-b."""
    dx, dy = bx - ax, by - ay
    l2 = dx * dx + dy * dy
    t = 0.0 if l2 == 0 else max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / l2))
    qx, qy = ax + t * dx, ay + t * dy
    return math.hypot(px - qx, py - qy)


def stroke(d, width):
    return d - width / 2.0


def circle(px, py, cx, cy, r):
    return math.hypot(px - cx, py - cy) - r


def box(px, py, x0, y0, x1, y1):
    """Signed distance to an axis-aligned box."""
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    hx, hy = (x1 - x0) / 2, (y1 - y0) / 2
    qx, qy = abs(px - cx) - hx, abs(py - cy) - hy
    out = math.hypot(max(qx, 0), max(qy, 0))
    return out + min(max(qx, qy), 0)


def outline_box(px, py, x0, y0, x1, y1, w=1.0):
    return abs(box(px, py, x0 + w / 2, y0 + w / 2, x1 - w / 2, y1 - w / 2)) - w / 2


def union(*ds):
    return min(ds)


def subtract(a, b):
    return max(a, -b)


W = 1.2  # default stroke width


# --- icons -------------------------------------------------------------------

def icon_close(x, y):
    return union(stroke(seg(x, y, 4, 4, 12, 12), W), stroke(seg(x, y, 12, 4, 4, 12), W))


def icon_dot(x, y):
    return circle(x, y, 8, 8, 3.5)


def icon_chevron_right(x, y):
    return union(stroke(seg(x, y, 6, 4, 10, 8), W), stroke(seg(x, y, 10, 8, 6, 12), W))


def icon_chevron_down(x, y):
    return union(stroke(seg(x, y, 4, 6, 8, 10), W), stroke(seg(x, y, 8, 10, 12, 6), W))


def icon_split_horizontal(x, y):
    return union(outline_box(x, y, 2, 3, 14, 13), stroke(seg(x, y, 8, 3, 8, 13), 1.0))


def icon_split_vertical(x, y):
    return union(outline_box(x, y, 2, 3, 14, 13), stroke(seg(x, y, 2, 8, 14, 8), 1.0))


def icon_files(x, y):
    front = outline_box(x, y, 5, 4, 13, 15)
    back = union(stroke(seg(x, y, 3, 2, 10, 2), 1.0), stroke(seg(x, y, 3, 2, 3, 12), 1.0))
    return union(front, back)


def icon_search(x, y):
    ring = abs(circle(x, y, 7, 7, 4.2)) - 0.6
    return union(ring, stroke(seg(x, y, 10, 10, 14, 14), 1.6))


def icon_gear(x, y):
    a = math.atan2(y - 8, x - 8)
    r = 5.2 + 1.1 * (1 if math.cos(a * 8) > 0.2 else 0)
    outer = math.hypot(x - 8, y - 8) - r
    return subtract(outer, circle(x, y, 8, 8, 2.2))


def icon_warning(x, y):
    tri = union(stroke(seg(x, y, 8, 2, 14.5, 14), W),
                stroke(seg(x, y, 14.5, 14, 1.5, 14), W),
                stroke(seg(x, y, 1.5, 14, 8, 2), W))
    mark = union(stroke(seg(x, y, 8, 6.5, 8, 10), W), circle(x, y, 8, 12, 0.8))
    return union(tri, mark)


def icon_error(x, y):
    ring = abs(circle(x, y, 8, 8, 6)) - 0.6
    cross = union(stroke(seg(x, y, 5.5, 5.5, 10.5, 10.5), W), stroke(seg(x, y, 10.5, 5.5, 5.5, 10.5), W))
    return union(ring, cross)


def icon_info(x, y):
    ring = abs(circle(x, y, 8, 8, 6)) - 0.6
    mark = union(stroke(seg(x, y, 8, 7.5, 8, 11), W), circle(x, y, 8, 5.2, 0.8))
    return union(ring, mark)


def icon_terminal(x, y):
    frame = outline_box(x, y, 1.5, 2.5, 14.5, 13.5)
    prompt = union(stroke(seg(x, y, 4, 6, 6.5, 8), 1.0), stroke(seg(x, y, 6.5, 8, 4, 10), 1.0))
    line = stroke(seg(x, y, 8, 10.5, 12, 10.5), 1.0)
    return union(frame, prompt, line)


def icon_more(x, y):
    return union(circle(x, y, 3.5, 8, 1.1), circle(x, y, 8, 8, 1.1), circle(x, y, 12.5, 8, 1.1))


def icon_layout_sidebar(x, y):
    return union(outline_box(x, y, 1.5, 2.5, 14.5, 13.5), box(x, y, 2, 3, 6, 13))


def icon_layout_panel(x, y):
    return union(outline_box(x, y, 1.5, 2.5, 14.5, 13.5), box(x, y, 2, 9, 14, 13))


def icon_add(x, y):
    return union(stroke(seg(x, y, 8, 3, 8, 13), W), stroke(seg(x, y, 3, 8, 13, 8), W))


def icon_cube(x, y):
    top = [(8, 2), (14, 5), (8, 8), (2, 5)]
    d = 1e9
    for i in range(4):
        a, b = top[i], top[(i + 1) % 4]
        d = min(d, seg(x, y, a[0], a[1], b[0], b[1]))
    d = min(d, seg(x, y, 2, 5, 2, 11), seg(x, y, 14, 5, 14, 11), seg(x, y, 8, 8, 8, 14),
            seg(x, y, 2, 11, 8, 14), seg(x, y, 14, 11, 8, 14))
    return stroke(d, 1.0)


ICONS = {
    "close": icon_close,
    "dot": icon_dot,
    "chevron_right": icon_chevron_right,
    "chevron_down": icon_chevron_down,
    "split_horizontal": icon_split_horizontal,
    "split_vertical": icon_split_vertical,
    "files": icon_files,
    "search": icon_search,
    "gear": icon_gear,
    "warning": icon_warning,
    "error": icon_error,
    "info": icon_info,
    "terminal": icon_terminal,
    "more": icon_more,
    "layout_sidebar": icon_layout_sidebar,
    "layout_panel": icon_layout_panel,
    "add": icon_add,
    "cube": icon_cube,
}


# --- rasterize + PNG ---------------------------------------------------------

def render(fn, size):
    """Return RGBA rows. Coverage from supersampled signed distance."""
    rows = []
    scale = 16.0 / size
    n = SUPERSAMPLE
    for py in range(size):
        row = bytearray()
        for px in range(size):
            cov = 0.0
            for sy in range(n):
                for sx in range(n):
                    x = (px + (sx + 0.5) / n) * scale
                    y = (py + (sy + 0.5) / n) * scale
                    if fn(x, y) <= 0:
                        cov += 1
            a = int(round(255 * cov / (n * n)))
            row += bytes((255, 255, 255, a))
        rows.append(bytes(row))
    return rows


def write_png(path, size, rows):
    raw = b"".join(b"\x00" + r for r in rows)

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "assets", "icons")
    os.makedirs(out, exist_ok=True)
    for name, fn in ICONS.items():
        for size in (16, 32):
            write_png(os.path.join(out, "%s_%d.png" % (name, size)), size, render(fn, size))
    print("wrote %d icons to %s" % (len(ICONS), os.path.normpath(out)))


if __name__ == "__main__":
    main()
