#include "tools.h"

#include <inttypes.h>


namespace Tools
{

	/// similar to: http://graphics.stanford.edu/~seander/bithacks.html#IntegerLog
	uint32_t highestBitSet(uint32_t value)
	{
		register uint32_t result;
		register uint32_t shift;
		result = (value > 0xFFFF) << 4; value >>= result;
		shift = (value > 0x00FF) << 3; value >>= shift; result |= shift;
		shift = (value > 0x000F) << 2; value >>= shift; result |= shift;
		shift = (value > 0x0003) << 1; value >>= shift; result |= shift;
		result |= (value >> 1);
		return result;
	}

	/// See: https://en.wikipedia.org/wiki/Adler-32
	uint32_t calculateAdler32(const std::vector<uint8_t> & data, uint32_t adler)
	{
		uint32_t s1 = adler & 0xffff;
		uint32_t s2 = (adler >> 16) & 0xffff;
		//calculate checksum for buffer
		for (uint32_t n = 0; n < data.size(); ++n)
		{
			s1 = (s1 + data[n]) % 65521;
			s2 = (s2 + s1) % 65521;
		}
		//build final checksum
		return (s2 << 16) + s1;
	}

	void outputBits(std::vector<uint8_t> & dest, uint32_t & index, uint32_t & buffer, uint32_t & bufferBits, bool dumpRemaining)
	{
		//if the buffer has a short or byte available, output it
		while (bufferBits <= 24)
		{
			dest[index++] = (uint8_t)(buffer >> 24);
			buffer <<= 8;
			bufferBits += 8;
		}
		//now if we still have remaining bits, dump buffer byte, which automatically adds the bits plus trailing zero bits
		if (dumpRemaining && bufferBits < 32)
		{
			dest[index++] = (uint8_t)(buffer >> 24);
		}
	}

}