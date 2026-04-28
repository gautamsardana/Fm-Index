import os
import json

os.makedirs("experiments/inputs", exist_ok=True)

def write_txt(filepath, values):
    with open(filepath, "w") as f:
        f.write("".join(str(v) for v in values))

def count_occurrences(values, pattern):
    count = 0
    for i in range(len(values) - len(pattern) + 1):
        if values[i:i+len(pattern)] == pattern:
            count += 1
    return count

def find_occurrences(values, pattern):
    positions = []
    for i in range(len(values) - len(pattern) + 1):
        if values[i:i+len(pattern)] == pattern:
            positions.append(i)
    return positions

# --- Test cases ---
test_cases = [
    {
        "name": "simple",
        "values": [1, 0, 1, 1, 0, 0, 1, 0],
        "queries": [[1, 0], [1, 1], [0, 0], [1]],
    },
    {
        "name": "all_zeros",
        "values": [0] * 16,
        "queries": [[0], [0, 0], [1], [0, 0, 0]],
    },
    {
        "name": "all_ones",
        "values": [1] * 16,
        "queries": [[1], [1, 1], [0], [1, 1, 1]],
    },
    {
        "name": "alternating",
        "values": [0, 1] * 16,
        "queries": [[0, 1], [1, 0], [0, 0], [1, 1]],
    },
]

expected = {}
for tc in test_cases:
    path = f"experiments/inputs/{tc['name']}.txt"
    write_txt(path, tc["values"])
    expected[tc["name"]] = {
        "n_bits": len(tc["values"]),
        "queries": []
    }
    for q in tc["queries"]:
        expected[tc["name"]]["queries"].append({
            "pattern": q,
            "count": count_occurrences(tc["values"], q),
            "positions": find_occurrences(tc["values"], q),
        })
    print(f"Generated {path} ({len(tc['values'])} bits)")

with open("experiments/expected.json", "w") as f:
    json.dump(expected, f, indent=2)
print("Wrote experiments/expected.json")
