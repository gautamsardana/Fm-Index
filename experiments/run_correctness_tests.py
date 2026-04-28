"""
Here’s the same table with an added “What is being tested” column.
(Empty pattern excluded; assume unsupported.)

Test case T	Query P	Count	Locate	What is being tested
""	0	0	[]	Empty text
""	1	0	[]	Empty text
0	0	1	[0]	Single char match
0	1	0	[]	Single char mismatch
0	00	0	[]	Pattern longer than text
1	1	1	[0]	Single char match
1	0	0	[]	Single char mismatch
1	11	0	[]	Pattern longer than text
00000	0	5	[0,1,2,3,4]	All same char
00000	00	4	[0,1,2,3]	Overlapping matches
00000	000	3	[0,1,2]	Overlapping matches
00000	00000	1	[0]	Full match
00000	1	0	[]	Absent symbol
11111	1	5	[0,1,2,3,4]	All same char
11111	11	4	[0,1,2,3]	Overlapping matches
11111	111	3	[0,1,2]	Overlapping matches
11111	11111	1	[0]	Full match
11111	0	0	[]	Absent symbol
01010101	0	4	[0,2,4,6]	Alternating pattern
01010101	1	4	[1,3,5,7]	Alternating pattern
01010101	01	4	[0,2,4,6]	Regular structure
01010101	10	3	[1,3,5]	Shifted pattern
01010101	010	3	[0,2,4]	Multi-step backward search
01010101	101	3	[1,3,5]	Multi-step backward search
01010101	01010101	1	[0]	Full match
01010101	00	0	[]	No consecutive zeros
01010101	11	0	[]	No consecutive ones
00110011	0	4	[0,1,4,5]	Mixed blocks
00110011	1	4	[2,3,6,7]	Mixed blocks
00110011	00	2	[0,4]	Block boundaries
00110011	11	2	[2,6]	Block boundaries
00110011	01	2	[1,5]	Transition correctness
00110011	10	1	[3]	Transition correctness
00110011	0011	2	[0,4]	Multi-block pattern
00110011	1100	1	[2]	Cross-boundary match
000111000	0	6	[0,1,2,6,7,8]	Two runs
000111000	1	3	[3,4,5]	Two runs
000111000	000	2	[0,6]	Separate runs
000111000	111	1	[3]	Middle run
000111000	01	1	[2]	Run boundary
000111000	10	1	[5]	Run boundary
000111000	01110	1	[2]	Complex match
000111000	1111	0	[]	Pattern exceeds run
0000000	000	5	[0,1,2,3,4]	Heavy overlap
0000000	0000	4	[0,1,2,3]	Heavy overlap
0000000	0000000	1	[0]	Full match
0000000	00000000	0	[]	Pattern too long
10001	1	2	[0,4]	Boundary matches
10001	0	3	[1,2,3]	Middle region
10001	10	1	[0]	Prefix match
10001	01	1	[3]	Suffix match
10001	000	1	[1]	Middle run
10001	10001	1	[0]	Full match
10001	11	0	[]	Non-existent
010101	111	0	[]	Absent pattern
010101	000	0	[]	Absent pattern
010101	1010	1	[1]	Partial overlap
011010011001	0	6	[0,3,5,6,9,10]	Random structure
011010011001	1	6	[1,2,4,7,8,11]	Random structure
011010011001	01	4	[0,3,6,10]	General correctness
011010011001	10	3	[2,4,8]	General correctness
011010011001	110	2	[1,7]	Multi-step LF
011010011001	001	2	[5,9]	Multi-step LF
011010011001	0110	2	[0,6]	Longer pattern
011010011001	111	0	[]	Absent
011010011001	full string	1	[0]	Full match
🔒 Always verify (for ALL rows)
count == locate.size()
positions valid
substrings match
sorted + no duplicates


✅ Add these test cases
Test case T	Query P	Count	Locate	What is being tested
0101	101	0	[]	No wrap-around across sentinel
001	01	1	[1]	Match ending at last index
100	10	1	[0]	Match starting at index 0
0001000	1	1	[3]	Single isolated occurrence
00101	101	1	[2]	Match at suffix (not prefix)
✅ Add rank edge test (not table-based, but REQUIRED)

Pick any BWT (example):

BWT = "01011010"

Add checks:

Query	Expected
rank(0)	0
rank(1)	BWT[0]
rank(n-1)	correct prefix sum
rank(n)	total # of 1s
✅ Add LF-mapping test (one-time check)

For any test string:

Start at sentinel_row
Apply LF repeatedly n times
Reconstruct string
Must equal original T


Test determinism:
run queries multiple times → same result
Random fuzz:
generate random T and P → compare with brute
"""