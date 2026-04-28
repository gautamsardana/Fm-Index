import random

values = [random.randint(0, 1) for _ in range(100)]

# pack 8 bits per byte, pad last byte with 0s
padded = values + [0] * (8 - len(values) % 8) % 8
bytes_out = []
for i in range(0, len(padded), 8):
    byte = 0
    for bit in padded[i:i+8]:
        byte = (byte << 1) | bit
    bytes_out.append(byte)

with open("sample.bin", "wb") as f:
    f.write(bytearray(bytes_out))

print(f"Generated sample.bin: {len(values)} bits packed into {len(bytes_out)} bytes")
print("Values:", values)
