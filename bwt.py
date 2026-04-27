import sys

def build_bwt(s, sa):
    n = len(s)
    bwt = ''.join(s[i - 1] for i in sa)
    return bwt

def main():
    if len(sys.argv) < 2:
        print("Usage: python fm.py <input_file>")
        sys.exit(1)

    with open(sys.argv[1], 'r') as f:
        text = f.read().strip()

    # append sentinel
    text = text + '$'
    print(f"Text: {text}")

    sa = build_suffix_array_fast(text)
    print(f"Suffix Array: {sa}")

    bwt = build_bwt(text, sa)
    print(f"BWT: {bwt}")

if __name__ == "__main__":
    main()