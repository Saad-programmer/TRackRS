import os
import re
import xml.etree.ElementTree as ET

maps_dir = "/home/alan/Downloads/Dev_Game/trackRS/data"

levels = []

for root, dirs, files in os.walk(maps_dir):
    for file in files:
        if file.endswith(".level"):
            filepath = os.path.join(root, file)
            try:
                tree = ET.parse(filepath)
                level_elem = tree.getroot()
                level_name = level_elem.get("name")
                
                race_elem = level_elem.find("race")
                if race_elem is not None:
                    targettime = float(race_elem.get("targettime", 0))
                    coordscale = race_elem.get("coordscale", "1, 1")
                    # parse coordscale
                    parts = [float(p.strip()) for p in coordscale.split(",")]
                    scale_x = parts[0] if len(parts) > 0 else 1.0
                    scale_y = parts[1] if len(parts) > 1 else 1.0
                    
                    checkpoints = race_elem.findall("checkpoint")
                    
                    # Calculate total track distance by summing distance between consecutive checkpoints
                    pts = []
                    # Start position is the starting point
                    start_elem = race_elem.find("startposition")
                    if start_elem is not None:
                        pos = start_elem.get("pos", "0, 0")
                        pt_parts = [float(p.strip()) for p in pos.split(",")]
                        pts.append((pt_parts[0] * scale_x, pt_parts[1] * scale_y))
                    
                    for cp in checkpoints:
                        coords = cp.get("coords", "0, 0")
                        pt_parts = [float(p.strip()) for p in coords.split(",")]
                        pts.append((pt_parts[0] * scale_x, pt_parts[1] * scale_y))
                        
                    distance = 0.0
                    for i in range(len(pts) - 1):
                        p1 = pts[i]
                        p2 = pts[i+1]
                        dist = ((p1[0] - p2[0])**2 + (p1[1] - p2[1])**2)**0.5
                        distance += dist
                    
                    levels.append({
                        "name": level_name,
                        "path": filepath,
                        "targettime": targettime,
                        "distance": distance,
                        "num_checkpoints": len(checkpoints)
                    })
            except Exception as e:
                print(f"Error parsing {filepath}: {e}")

# Sort by targettime and distance
levels_by_time = sorted(levels, key=lambda x: x["targettime"], reverse=True)
levels_by_dist = sorted(levels, key=lambda x: x["distance"], reverse=True)

print("--- TOP 10 BY TARGET TIME ---")
for i, l in enumerate(levels_by_time[:10]):
    print(f"{i+1}. {l['name']} ({l['path']}) - targettime: {l['targettime']}s, distance: {l['distance']:.2f}m, checkpoints: {l['num_checkpoints']}")

print("\n--- TOP 10 BY DISTANCE ---")
for i, l in enumerate(levels_by_dist[:10]):
    print(f"{i+1}. {l['name']} ({l['path']}) - targettime: {l['targettime']}s, distance: {l['distance']:.2f}m, checkpoints: {l['num_checkpoints']}")
