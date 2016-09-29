CoMPres5
========
(short cmp5) is a collection of lossless compression algorithms and meant as a testbed mainly for trying out lossless image compression techniques for [NerDisco](https://github.com/HorstBaerbel/NerDisco) and [res2h](https://github.com/HorstBaerbel/res2h). It includes delta encoding, Burrows-Wheeler transform, move-to-front encoding, zero run-length encoding, a static huffman and LZSS entropy encoder. I plan to add code for adaptive Huffman, LZ4 and to try out a inter-frame compression technique for images.  
Compression ratios are in the range of bzip2 (as-in: not really stellar). The algorithms were tested with the [Canterbury corpus](http://corpus.canterbury.ac.nz/descriptions/#cantrbry) and the [Silesia corpus](http://sun.aei.polsl.pl/~sdeor/index.php?page=silesia). The results for the [Canterbury corpus](http://corpus.canterbury.ac.nz/descriptions/#cantrbry):  

Method  | text | fax  | Csrc | Excl | SPRC | tech | poem | html | list | man  | play
--------|------|------|------|------|------|------|------|------|------|------|------
bzip2-9 | 2.27 | 0.78 | 2.18 | 1.01 | 2.70 | 2.02 | 2.42 | 2.48 | 2.79 | 3.33 | 2.53
bzip2-1 | 2.42 | 0.78 | 2.18 | 0.96 | 2.70 | 2.34 | 2.72 | 2.48 | 2.79 | 3.33 | 2.65
cmp5<sup>1</sup>    | 2.37 | 0.79 | 2.28 | 1.43 | 2.80 | 2.25 | 2.66 | 2.56 | 2.90 | 3.44 | 2.66
cmp5<sup>2</sup>   | 3.99 | 1.68 | 2.92 | 2.78 | 3.89 | 3.89 | 4.56 | 3.77 | 3.88 | 4.45 | 4.36
<sup>1</sup> options: "-t -bwt65535 -mtf1 -rle0 -huffman", which is more or less equal to bzip2 with the "-1" (fast) option.  
<sup>2</sup> options: "-t -lzss32768" (32k dictionary, 4k look-ahead buffer)

License
========
[BSD-2-Clause](http://opensource.org/licenses/BSD-2-Clause), see [LICENSE.md](LICENSE.md).  
[SAIS-lite](https://sites.google.com/site/yuta256/sais) is used for suffix array generation. See [sais/COPYING](sais/COPYING).

Building
========
**Use CMake**

<pre>
cd compress
cmake .
make
</pre>

G++ 4.7 / VS2013 or higher (for C++11) will be needed to compile CoMPres5. Support for std::filesystem (via std::tr2::filesystem) is needed. For installing G++ 4.7 see [here](http://lektiondestages.blogspot.de/2013/05/installing-and-switching-gccg-versions.html).

Usage
========

<pre>
cmp5 [-c, -d, -t] [options] infile [outfile]
</pre>

**Available options (you must specify -c, -d or -t):**
Option | Description |
--------|------|
**-c** | Compress data from **infile** to **outfile**
**-d** | Decompress data from **infile** to **outfile**
**-t** | Test routines by compressing/decompressing data from **infile** in memory
**-v** | Be verbose
**-b** | Benchmark compression and decompression
**"random"** | use for **infile** to generate random input data

**Available pre-processing options (optional):**
Option | Description |
--------|------|
**-rgbSplit** | Split R8G8B8 data into RRR...GGG...BBB... color planes (size must be divisible by 3)
**-delta** | Apply delta-encoding on consecutive bytes
**-bwt[block size]** | Apply Burrows-Wheeler transform. Block size in bytes is optional, e.g. **"-bwt1024"** (Default is 256kB, max. is 16MB)
**-mtf1** | Apply move-to-front-1 encoding
**-rle0** | Apply zero run-length encoding

**Available entropy coders (optional):**
Option | Description |
--------|------|
**-huffman** | Use static Huffman entropy coder
**-lzss** | Use LZSS entropy encoder. Dictionary size is optional, e.g. **"-lzss16384" (Default is 4k, look-ahead buffer size is 1/8 of dictionary size)

**Examples:**  
Compress single file:
<pre>cmp5 -c -huffman ./canterbury/alice29.txt ./alice29.cmp5</pre>
Decompress single file:
<pre>cmp5 -d ./alice29.cmp5 ./canterbury/alice29_2.txt</pre>  
Test routines:
<pre>cmp5 -t -bwt -huffman ./canterbury/alice29.txt</pre> 
Compress all files in directory:
<pre>cmp5 -c -huffman ./canterbury ./compressed</pre>  
Compress all files matching wildcards:
<pre>cmp5 -c -huffman ./test/*.txt</pre>
Test random generated data and be verbose:
<pre>cmp5 -t -v -huffman random</pre>

I found a bug or have a suggestion
========

The best way to report a bug or suggest something is to post an issue on GitHub. Try to make it simple, but descriptive and add ALL the information needed to REPRODUCE the bug. **"Does not work" is not enough!** If you can not compile, please state your system, compiler version, etc! You can also contact me via email if you want to.
