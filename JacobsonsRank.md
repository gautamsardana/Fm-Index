# Implementation behind Jacobson's Rank

Build(BWT, Size)

 - BWT is the bitvector which represents the BWT of the original input
 - Size is the size of the BWT (Number of bits)

## CHUNKS

chunk_size = (log2(size)) ^ 2
num_chunks = ceil(n/chunk_size) -> (chunk_size + n - 1)/chunk_size
num_bits_per_chunk = (logn)^2

uint64_t chunk_rank[num_chunks] = zero this out;
ie we should be able to get chunk_rank[i], this will return the rank of the first index in the ith chunk (not including)
get_cumulative_rank will do this 

This will take 64 * num_chunks bits to store the chunk_rank array: 64* (n/(log2(size))^2)

Now we need to set up the sub chunk arrays

## SUB-CHUNKS

sub_chunk_size = 1/2log(size)
num_sub_chunks = 2log(size)
num_bits_per_sub_chunk_rank = log((logn)^2) // Derived from the size of a chunk being (logn)^2, thus the num of digits needed is log of that

* Multiplying these yields (log(size))^2 which is the chunk size!

Now we need a get_relative_rank(chunk_idx, chunk_offset, sub_chunk_offset)

Finally we need a get_sub_chunk_rank(sub_chunk) which can be done via a population_count
 - We have a subchunk of size 1/2log(size)
 - if we have a size of 1000000000000 this only requires 40 bits to represent
 - Thus we can store these in chains of uint8_t, so we must divy up the original bwt bits into uint8_t sections
 - Then we can address the specific run we need via
get_rank(offset)
  which_chunk = (offset/chunk_size)
  which_subchunk = (offset - which_chunk * chunk_size)/sub_chunk_size
  subchunk_start = which_chunk * chunk_size + which_subchunk * sub_chunk_size
  subchunk_bits = get_bits(bwt, subchunk_start, offset) // should get the bits in that range and pack into uint64_t
  return get_cumulative_rank(which_chunk) + get_relative_rank(which_subchunk) + subchunk_bits

