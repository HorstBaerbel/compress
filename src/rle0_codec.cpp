#include "rle0_codec.h"

#include "tools.h"

#include <vector>
#include <array>
#include <iostream>


const uint8_t Rle0::CodecIdentifier = 55;

uint8_t Rle0::codecIdentifier() const
{
	return CodecIdentifier;
}

std::string Rle0::codecName() const
{
	return "Zero run-length";
}

I_Codec * Rle0::Create()
{
	return new Rle0();
}

std::vector<uint8_t> Rle0::encode(const std::vector<uint8_t> & source) const
{
	const uint32_t srcSize = static_cast<uint32_t>(source.size());
	if (srcSize > 0)
	{
		std::vector<uint8_t> dest(srcSize + srcSize / 2 + 4 + 1 + 1);
		uint32_t destIndex = 0;
		uint32_t srcIndex = 0;
		//output source size
		*((uint32_t *)&dest[destIndex]) = srcSize;
		destIndex += 4;
		//check which symbols do not occur in data
		std::array<std::pair<uint8_t, uint32_t>, 256> frequencies;
		for (uint32_t i = 0; i < frequencies.size(); ++i)
		{
			frequencies[i].first = i;
			frequencies[i].second = 0;
		}
		for (auto symbol : source)
		{
			frequencies[symbol].second++;
		}
		//sort by count
		std::sort(frequencies.begin(), frequencies.end(), [](const std::pair<uint8_t, uint32_t> & a, const std::pair<uint8_t, uint32_t> & b)
																			 { return a.second > b.second; });
		//check if we have two free symbols to mark a run of zeros (0 and 1)
		//we add 1 to every byte in the data stream to free up 1, thus the one extra symbol needs to be free
		auto unneededSymbol = std::find_if(frequencies.cbegin(), frequencies.cend(), [](const std::pair<uint8_t, uint32_t> & n){ return n.first > 0 && n.second == 0; });
		if (unneededSymbol != frequencies.cend())
		{
			if (m_verbose) std::cout << "Using Wheeler zero run length encoding." << std::endl;
			//yes. do proper run-length encoding. store compression mode byte.
			dest[destIndex++] = 1;
			//store unneeded symbol
			const uint8_t symbolBorder = unneededSymbol->first;
			dest[destIndex++] = symbolBorder;
			//compress data
			while (srcIndex < srcSize)
			{
				//get current symbol from source
				const uint8_t symbol = source[srcIndex++];
				//check if it is a zero byte
				if (symbol == 0)
				{
					//yes. count run of zeros
					uint32_t count = 1;
					while (srcIndex < srcSize && count < INT_MAX && source[srcIndex] == 0) { ++count; ++srcIndex; }
					//increase count by one
					count++;
					//count bits needed to encode run length. subtract one, because the MSB is always 1 and we don't need to store it
					const int32_t nrOfbits = Tools::log2(count) - 1;
					//store run length by encoding a one bit as 1 and a zero bit as 0
					for (int32_t bit = nrOfbits; bit >= 0; --bit)
					{
						//store bits from MSB to LSB for easy decoding
						dest[destIndex++] = static_cast<uint8_t>((count >> bit) & 1);
					}
				}
				else
				{
					//no. store symbol + 1 if below symbol border, else store symbol
					dest[destIndex++] = symbol <= symbolBorder ? symbol + 1 : symbol;
				}
			}
		}
		else
		{
			if (m_verbose) std::cout << "Using naive zero run length encoding." << std::endl;
			//no. use less efficient run-length encoding. store compression mode byte.
			dest[destIndex++] = 0;
			//compress data
			while (srcIndex < srcSize)
			{
				//get current symbol from source
				const uint8_t symbol = source[srcIndex++];
				//store symbol
				dest[destIndex++] = symbol;
				//check if it is a zero byte
				if (symbol == 0)
				{
					//count zeros following
					uint32_t count = 0;
					while (srcIndex < (srcSize - 1) && count < 255 && source[srcIndex] == 0) { ++count; ++srcIndex; }
					//store run length
					dest[destIndex++] = count;
				}
			}
		}
		dest.resize(destIndex);
		return dest;
	}
	return std::vector<uint8_t>();
}

std::vector<uint8_t> Rle0::decode(const std::vector<uint8_t> & source) const
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
		//read compression mode used
		const uint8_t mode = source[srcIndex++];
		if (mode == 0)
		{
			//less efficient fallback compression mode. decompress data
			while (srcIndex < srcSize && destIndex < destSize)
			{
				//get current symbol from source
				const uint8_t symbol = source[srcIndex++];
				//store symbol
				dest[destIndex++] = symbol;
				//check if it is a zero byte
				if (symbol == 0)
				{
					//write run of zeros
					uint32_t count = source[srcIndex++];
					for (uint32_t i = 0; i < count; ++i)
					{
						dest[destIndex++] = 0;
					}
				}
			}
			return dest;
		}
		else if (mode == 1)
		{
			//proper zero run length encoding. read unneeded symbol
			const uint8_t symbolBorder = source[srcIndex++];
			//decompress data
			while (srcIndex < srcSize && destIndex < destSize)
			{
				//get current symbol from source
				uint8_t symbol = source[srcIndex++];
				//check if it a 0 or 1 symbol
				if (symbol < 2)
				{
					//yes. encoded zero run. restore one MSB and add first bit read from source
					uint32_t count = 2 | symbol;
					//read more bits from source
					while (srcIndex < srcSize && (symbol = source[srcIndex]) < 2)
					{
						count = (count << 1) | symbol;
						++srcIndex;
					}
					//now decrease count by 1
					count--;
					//write length zeros to destination
					for (uint32_t i = 0; i < count; ++i)
					{
						dest[destIndex++] = 0;
					}
				}
				else
				{
					//no. verbatim byte. store symbol - 1 or symbol depending where in relation to empty symbol
					dest[destIndex++] = symbol <= symbolBorder ? symbol - 1 : symbol;
				}
			}
			return dest;
		}
	}
	return std::vector<uint8_t>();
}
