import subprocess
import json
import sys

BINARY = "./build/fm_index"

with open("experiments/expected.json") as f:
    expected = json.load(f)

passed = 0
failed = 0

for name, data in expected.items():
    input_file = f"experiments/inputs/{name}.txt"

    for q in data["queries"]:
        pattern = "".join(str(b) for b in q["pattern"])
        expected_count = q["count"]
        expected_positions = sorted(q["positions"])

        result = subprocess.run(
            [BINARY, "--convert-and-query", input_file, pattern],
            stdout=subprocess.PIPE, text=True
        )

        if result.returncode != 0:
            print(f"FAIL [{name}] pattern={pattern}: binary error")
            failed += 1
            continue

        output = result.stdout.strip()
        actual_count = int(output.split("count=")[1].split()[0])
        pos_str = output.split("positions=")[1].strip()
        actual_positions = sorted(int(p) for p in pos_str.split() if p)

        count_ok = actual_count == expected_count
        positions_ok = actual_positions == expected_positions

        if count_ok and positions_ok:
            print(f"PASS [{name}] pattern={pattern}: count={actual_count} positions={actual_positions}")
            passed += 1
        else:
            if not count_ok:
                print(f"FAIL [{name}] pattern={pattern}: expected count={expected_count} got={actual_count}")
            if not positions_ok:
                print(f"FAIL [{name}] pattern={pattern}: expected positions={expected_positions} got={actual_positions}")
            failed += 1

print(f"\n{passed} passed, {failed} failed")
sys.exit(0 if failed == 0 else 1)
