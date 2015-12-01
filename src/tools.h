#pragma once

#include <inttypes.h>
#include <vector>


namespace Tools
{

	/// @brief Create Adler-32 checksum from data.
	/// @param data Data to create checksum for.
	/// @param adler Optional. Adler checksum from last run if you're combining data.
	/// @return Returns the Adler-32 checksum for the data or the initial checksum upon failure.
	/// @note Based on the sample code here: https://tools.ietf.org/html/rfc1950. 
	/// This is not as safe as CRC-32 (see here: https://en.wikipedia.org/wiki/Adler-32), but should be totally sufficient for us.
	uint32_t calculateAdler32(const std::vector<uint8_t> & dest, uint32_t adler = 1);

	/// @brief Highest bit position set to 1 in the value.
	/// @param value Input value.
	/// @return Return the highest bit set to 1 (0-31).
	uint32_t highestBitSet(uint32_t value);

	/// @brief Write bits from buffer into dest starting at index.
	/// @param dest Point to destination.
	/// @param index Index in dest array for start of output.
	/// @param buffer Data to output.
	/// @param availableBits Free bits available in the buffer. Output will occur if avaiableBits <= 24.
	/// @param dumpRemaining Pass true to dump all remaining bits to output by dumping a full byte if needed.
	/// The excess bits might contain random data.
	void outputBits(std::vector<uint8_t> & dest, uint32_t & index, uint32_t & buffer, uint32_t & availableBits, bool dumpRemaining = false);

}