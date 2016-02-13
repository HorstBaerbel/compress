#include "bwt_codec.h"

#include <vector>
#include <numeric>
#include <algorithm>
#include <array>
#include <iostream>
#include <fstream>
#include <iosfwd>
#include "../sais/sais.hxx"


const uint8_t Bwt::CodecIdentifier = 40;

uint8_t Bwt::codecIdentifier() const
{
	return CodecIdentifier;
}

std::string Bwt::codecName() const
{
	return "Burrows-Wheeler transform";
}

Bwt * Bwt::Create()
{
	return new Bwt();
}

void Bwt::setBlockSizeForCompression(const uint32_t blockSize)
{
	//clamp to 16MB - 1
	m_blockSize = blockSize > 16 * 1024 * 1024 - 1 ? 16 * 1024 * 1024 - 1 : blockSize;
}

std::vector<uint8_t> Bwt::encode(const std::vector<uint8_t> & source) const
{
	const uint32_t srcSize = static_cast<uint32_t>(source.size());
	if (srcSize > 0 && m_blockSize > 0)
	{
		//allocate destination data
		std::vector<uint8_t> dest(srcSize + 4 + 4 + 4 * ((srcSize / m_blockSize) + 1));
		uint32_t destIndex = 0;
		//output source size
		*((uint32_t *)&dest[destIndex]) = srcSize;
		destIndex += 4;
		//output BWT block size
		*((uint32_t *)&dest[destIndex]) = m_blockSize;
		destIndex += 4;
		//build array of indices into source array
		std::vector<int32_t> indices(2 * m_blockSize);
		//allocate array for block data storage and easier access
		std::vector<uint8_t> block(2 * m_blockSize);
		//loop through blocks
		uint32_t srcIndex = 0;
		while (srcIndex < srcSize)
		{
			//clamp block size so we don't read past the end of source
			const uint32_t size = (srcIndex + m_blockSize) > srcSize ? (srcSize - srcIndex) : m_blockSize;
			//copy data into block in reverse order and duplicate it. this makes it possible to write the data 
			//with increasing indices when decoding. also the suffix array algorithm will generate un-decodable data
			//when using only a single block of data. the generated indices are screwed up.
			//If you know the correct way to sort the string without needing to duplicate indices, please let me know!
			std::reverse_copy(std::next(source.cbegin(), srcIndex), std::next(source.cbegin(), srcIndex + size), block.begin());
			std::reverse_copy(std::next(source.cbegin(), srcIndex), std::next(source.cbegin(), srcIndex + size), std::next(block.begin(), size));
			//build suffix array from data
 			saisxx<uint8_t *, int32_t *, int32_t>(block.data(), indices.data(), 2 * size, 256);
			//store reference to start index for writing it later
			uint32_t & startIndex = ((uint32_t &)dest[destIndex]);
			destIndex += 4;
			//encode data to output while looking for start index
			uint32_t count = 0;
			for (uint32_t i = 0; i < 2 * size; ++i)
			{
				uint32_t index = static_cast<uint32_t>(indices[i]);
				//we have duplicated the input data, thus we only need to use indices that are "from the first half"
				if (index < size)
				{
					//check if we've found the start index
					if (index == 0)
					{
						//yes. store it and copy last input symbol to output
						startIndex = count;
						index = size;
					}
					//store symbol from input data to output data
					dest[destIndex++] = block[index - 1];
					count++;
				}
			}
			//move to next block of input data
			srcIndex += size;
		}
		dest.resize(destIndex);
		return dest;
	}
	return std::vector<uint8_t>();
}

/// @brief Three BWT-inversion algorithms are described very well in this paper: Space-Time trade-offs in the Burrows-Wheeler transform
/// Here, a variation of the algorithm "bw94" is used, while the input data has been reversed in the encoder
/// and thus the output can be written front-to-back in contrary to the original algorithm.
std::vector<uint8_t> Bwt::decode(const std::vector<uint8_t> & source) const
{
	const uint32_t srcSize = static_cast<uint32_t>(source.size());
	if (srcSize > 0)
	{
		//read result size
		uint32_t srcIndex = 0;
		const uint32_t destSize = *((uint32_t *)&source[srcIndex]);
		srcIndex += 4;
		//allocate destination data
		std::vector<uint8_t> dest(destSize);
		uint32_t destIndex = 0;
		//read BWT block size
		const uint32_t blockSize = *((uint32_t *)&source[srcIndex]);
		srcIndex += 4;
		//array for storing accumulated character frequencies
		std::vector<uint32_t> C(256);
		//inverse transform array
		std::vector<uint32_t> T(blockSize);
		//loop through blocks
		while (srcIndex < srcSize)
		{
			//read start index from data first
			const uint32_t startIndex = *((uint32_t *)&source[srcIndex]);
			srcIndex += 4;
			//clamp block size so we don't read past the end of source
			const uint32_t size = (srcIndex + blockSize) > srcSize ? (srcSize - srcIndex) : blockSize;
			//set up pointer to start of data
			const uint8_t * blockData = &source[srcIndex];
			//clear array storing symbol frequency information
			std::fill(C.begin(), C.end(), 0);
			//count symbol counts and build inverse transform array
			for (uint32_t i = 0; i < size; ++i)
			{
				T[i] = C[blockData[i]]++;
			}
			//sum counts
			for (uint32_t symbol = 1; symbol < 256; ++symbol)
			{
				C[symbol] += C[symbol - 1];
			}
			//shit counts right
			for (uint32_t symbol = 255; symbol > 0; --symbol)
			{
				C[symbol] = C[symbol - 1];
			}
			C[0] = 0;
			//undo the BWT by reading symbol, then looking up next index through transform arrays
			uint32_t index = startIndex;
			for (uint32_t i = 0; i < size; ++i)
			{
				const uint8_t symbol = blockData[index];
				dest[destIndex++] = symbol;
				index = T[index] + C[symbol];
			}
			srcIndex += size;
		}
		return dest;
	}
	return std::vector<uint8_t>();
}