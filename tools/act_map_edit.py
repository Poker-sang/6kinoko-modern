"""Bounded, lossless placement editor for self-described ACT1 map documents.

Supports map-only layers, no timelines, and chip resources. Other ACT variants
are rejected, never guessed. Unknown properties and scripts remain byte-exact.
"""
import struct

class Reader:
    def __init__(self, data):
        self.data = data; self.pos = 0; self.schemas = {}
    def take(self, size):
        if size < 0 or self.pos + size > len(self.data):
            raise ValueError("Truncated ACT stream")
        result = self.data[self.pos:self.pos + size]; self.pos += size
        return result
    def number(self):
        return struct.unpack("<I", self.take(4))[0]
    def count(self, limit=65536):
        result = self.number()
        if result > limit: raise ValueError("ACT count exceeds limit")
        return result
    def string(self, limit=1048576):
        return self.take(self.count(limit)).decode("cp932")
    def properties(self, kind):
        present = self.take(1)[0]
        if present not in (0, 1): raise ValueError("Invalid schema flag")
        if present:
            schema = {}
            for _ in range(self.count(1024)):
                name = self.string(4096); typ = self.number()
                if typ > 3 or name in schema: raise ValueError("Invalid property schema")
                schema[name] = typ
            self.schemas[kind] = schema
        if kind not in self.schemas:
            raise ValueError("First property schema must be explicit: " + kind)
        values, positions = {}, {}
        for name, typ in sorted(self.schemas[kind].items()):
            positions[name] = (self.pos, typ)
            values[name] = self.string() if typ == 3 else self.take(1)[0] if typ == 2 else struct.unpack("<f" if typ == 1 else "<i", self.take(4))[0]
        return values, positions
    def script(self):
        self.properties("script"); self.take(self.count(16777216))
    def expect(self, value):
        if self.number() != value: raise ValueError("Unsupported ACT object type")

class MapDocument:
    def __init__(self, data):
        self.data = bytes(data); r = Reader(self.data)
        if r.take(8) != b"ACT1\x01\0\0\0": raise ValueError("Expected ACT1 version 1")
        r.take(r.count(16777216))
        self.properties, self.positions = r.properties("document")
        r.script(); self.layers = {}
        for _ in range(r.count()):
            r.expect(0x2618cf18)
            props, _ = r.properties("layer"); name = props["stName"]
            if name in self.layers: raise ValueError("Duplicate layer name")
            keys = []
            for _ in range(r.count()):
                r.expect(0xd933304d); r.properties("key")
                if r.take(1) != b"\1": raise ValueError("Expected map key layout")
                r.expect(0xc9ca5c20)
                layout, positions = r.properties("map")
                start = r.pos; count = r.count(); size = r.number()
                if size != 12: raise ValueError("Only 12-byte placement records are editable")
                cells = [list(struct.unpack("<Iii", r.take(12))) for _ in range(count)]
                keys.append(dict(start=start, end=r.pos, cells=cells, properties=layout, positions=positions))
            if r.number() != 0: raise ValueError("Timelines are not supported")
            r.script(); self.layers[name] = dict(properties=props, keys=keys)
        for _ in range(r.count()):
            r.expect(0xfbaaf527); r.properties("chip")
        if r.pos != len(data): raise ValueError("Trailing ACT bytes")

    def edit(self, layers, width=None):
        changes = []
        def integer(positions, name, value):
            offset, typ = positions[name]
            if typ != 0 or type(value) is not int or not 0 <= value <= 2147483647:
                raise ValueError("Invalid integer property: " + name)
            changes.append((offset, offset+4, struct.pack("<i", value)))
        if width is not None: integer(self.positions, "screenWidth", width)
        for name, cells in layers.items():
            if name not in self.layers: raise ValueError("Unknown layer: " + name)
            keys = self.layers[name]["keys"]
            if len(keys) != 1: raise ValueError("Exactly one key is required for editing")
            if len(cells) > 65536: raise ValueError("Too many placements")
            for cell in cells:
                if len(cell) != 3 or any(type(v) is not int for v in cell): raise ValueError("Expected [chip, x, y]")
                if not 0 <= cell[0] <= 65535 or any(not 0 <= v <= 2147483647 for v in cell[1:]): raise ValueError("Placement out of range")
            # Native map queries rely on x/y ordering.
            ordered = sorted(cells, key=lambda c: (c[1], c[2]))
            key = keys[0]
            payload = struct.pack("<II", len(ordered), 12) + b"".join(struct.pack("<Iii", *c) for c in ordered)
            changes.append((key["start"], key["end"], payload))
            props = key["properties"]
            if ordered:
                bounds = {"mapChipLeft": min(c[1] for c in ordered), "mapChipTop": min(c[2] for c in ordered),
                          "mapChipRight": max(c[1] for c in ordered)+max(0, props["maxChipWidth"]),
                          "mapChipBottom": max(c[2] for c in ordered)+max(0, props["maxChipHeight"])}
            else: bounds = dict.fromkeys(("mapChipLeft", "mapChipTop", "mapChipRight", "mapChipBottom"), 0)
            for field, value in bounds.items(): integer(key["positions"], field, value)
        output = self.data
        for start, end, value in sorted(changes, reverse=True): output = output[:start] + value + output[end:]
        MapDocument(output)  # Validate before publishing any edited bytes.
        return output
