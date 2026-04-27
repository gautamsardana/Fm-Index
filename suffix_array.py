def build_suffix_array(s):
    n = len(s)
    suffixes = sorted(range(n), key=lambda i: s[i:])
    return suffixes

def build_suffix_array_fast(s):
    n = len(s)
    # initial ranking based on single characters
    sa = sorted(range(n), key=lambda i: s[i])
    rank = [0] * n
    rank[sa[0]] = 0
    for i in range(1, n):
        rank[sa[i]] = rank[sa[i-1]]
        if s[sa[i]] != s[sa[i-1]]:
            rank[sa[i]] += 1
    
    k = 1
    while k < n:
        # sort by (rank[i], rank[i+k])
        def sort_key(i):
            return (rank[i], rank[i+k] if i+k < n else -1)
        sa = sorted(range(n), key=sort_key)
        
        # update ranks
        new_rank = [0] * n
        new_rank[sa[0]] = 0
        for i in range(1, n):
            new_rank[sa[i]] = new_rank[sa[i-1]]
            if sort_key(sa[i]) != sort_key(sa[i-1]):
                new_rank[sa[i]] += 1
        rank = new_rank
        
        if rank[sa[n-1]] == n-1:
            break  # all ranks unique, done
        k *= 2
    
    return sa