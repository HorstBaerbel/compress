CoMPres5
========
(short cmp5) is a collection of lossless compression algorithms and meant as a testbed mainly for trying out lossless image compression techniques for [NerDisco](https://github.com/HorstBaerbel/NerDisco). It includes delta encoding, Burrows-Wheeler transform, move-to-front encoding, zero run-length encoding and a static huffman encoder. I plan to improve rle0 compression (better algorithm) and BWT transform speed (using suffix arrays), add code for adaptive Huffman, LZSS, LZ4 and to try out a inter-frame compression technique for images.  
Compression ratios are in the range of bzip2 (as-in: not really stellar). The results for the [Canterbury corpus](http://corpus.canterbury.ac.nz/descriptions/#cantrbry):  

Method  | text | fax  | Csrc | Excl | SPRC | tech | poem | html | list | man  | play
--------|------|------|------|------|------|------|------|------|------|------|------
bzip2-9 | 2.27 | 0.78 | 2.18 | 1.01 | 2.70 | 2.02 | 2.42 | 2.48 | 2.79 | 3.33 | 2.53
bzip2-1 | 2.42 | 0.78 | 2.18 | 0.96 | 2.70 | 2.34 | 2.72 | 2.48 | 2.79 | 3.33 | 2.65
cmp5*    | 2.41 | 0.81 | 2.34 | 1.42 | 2.80 | 2.31 | 2.73 | 2.60 | 2.96 | 3.50 | 2.71
*= options: (-bwt65535 -mtf1 -rle0 -huffman)

License
========
[BSD-2-Clause](http://opensource.org/licenses/BSD-2-Clause), see [LICENSE.md](LICENSE.md).  

Building
========
**Use CMake**

<pre>
cd compress
cmake .
make
cmp5
</pre>

G++ 4.7 / VS2013 or higher (for C++11) will be needed to compile CoMPres5. Support for std::filesystem is needed. For installing G++ 4.7 see [here](http://lektiondestages.blogspot.de/2013/05/installing-and-switching-gccg-versions.html).

I found a bug or have a suggestion
========

The best way to report a bug or suggest something is to post an issue on GitHub. Try to make it simple, but descriptive and add ALL the information needed to REPRODUCE the bug. **"Does not work" is not enough!** If you can not compile, please state your system, compiler version, etc! You can also contact me via email if you want to.
