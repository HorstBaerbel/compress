#include "bwt_codec.h"

#include <vector>
#include <numeric>
#include <algorithm>
#include <array>


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
	m_blockSize = blockSize;
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
		std::vector<uint32_t> indices(m_blockSize);
		//allocate array for block data storage and easier access
		std::vector<uint8_t> block(2 * m_blockSize);
		//loop through blocks
		uint32_t srcIndex = 0;
		while (srcIndex < srcSize)
		{
			//clamp block size so we don't read past the end of source
			const uint32_t size = (srcIndex + m_blockSize) > srcSize ? (srcSize - srcIndex) : m_blockSize;
			//copy data into block twice so can read it easier when we'd need to wrap around...
			std::copy(std::next(source.cbegin(), srcIndex), std::next(source.cbegin(), srcIndex + size), block.begin());
			std::copy(std::next(source.cbegin(), srcIndex), std::next(source.cbegin(), srcIndex + size), std::next(block.begin(), size));
			//initialize index array with consecutive indices
			std::iota(indices.begin(), std::next(indices.begin(), size), 0);
			//sort rotated block data and thus index data lexicographically
			std::stable_sort(indices.begin(), std::next(indices.begin(), size),
								  [&block, &size](const uint32_t & a, const uint32_t & b) {
								  return std::memcmp(&block[a], &block[b], size) < 0; });
// 				return std::lexicographical_compare(std::next(block.cbegin(), a), std::next(block.cbegin(), a + size), 
// 																std::next(block.cbegin(), b), std::next(block.cbegin(), b + size)); });
			//find start of initial string in sorted array by finding the index value 0 in the array
			const uint32_t startIndex = static_cast<uint32_t>(std::distance(indices.cbegin(), std::find(indices.cbegin(), std::next(indices.cbegin(), size), 0)));
			//output start index
			*((uint32_t *)&dest[destIndex]) = startIndex;
			destIndex += 4;
			//output last byte of all sorted strings
			for (uint32_t i = 0; i < size; ++i)
			{
				//store last character of rotated string
				const uint32_t index = indices[i] + (size - 1);
				dest[destIndex++] = block[index];
			}
			srcIndex += size;
		}
		dest.resize(destIndex);
		return dest;
	}
	return std::vector<uint8_t>();
}

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
		//array for storing how many symbols lexicographically less than symbol exist
		std::vector<uint32_t> predecessors(256);
		//array for storing how often a symbol source[index] occurs before index in source
		std::vector<uint32_t> occurrenceBefore(blockSize);
		//loop through blocks
		while (srcIndex < srcSize)
		{
			//read start index from data first
			uint32_t index = *((uint32_t *)&source[srcIndex]);
			srcIndex += 4;
			//clamp block size so we don't read past the end of source
			const uint32_t size = (srcIndex + blockSize) > srcSize ? (srcSize - srcIndex) : blockSize;
			//set up pointer to start of data
			const uint8_t * blockData = &source[srcIndex];
			//clear arrays storing symbol occurrence information
			std::fill(predecessors.begin(), predecessors.end(), 0);
			//count symbol occurrences and store number of symbol occurrences before position
			for (uint32_t i = 0; i < size; ++i)
			{
				const uint8_t symbol = blockData[i];
				occurrenceBefore[i] = predecessors[symbol];
				predecessors[symbol]++;
			}
			//store number of symbols lexicographically less than symbol
			uint32_t currentSum = 0;
			for (uint32_t symbol = 0; symbol < 256; ++symbol)
			{
				uint32_t symbolCount = predecessors[symbol];
				predecessors[symbol] = currentSum;
				currentSum += symbolCount;
			}
			//move destination index to end of array, the BWT is reconstructed in reverse order...
			destIndex += size;
			//undo the BWT by reading symbol, then looking up next index through transform arrays
			for (uint32_t i = 0; i < size; ++i)
			{
				const uint8_t symbol = blockData[index];
				dest[--destIndex] = symbol;
				index = occurrenceBefore[index] + predecessors[symbol];
			}
			srcIndex += size;
			destIndex += size;
		}
		return dest;
	}
	return std::vector<uint8_t>();
}