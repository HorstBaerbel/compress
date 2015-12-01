#include "rle0_codec.h"

#include <vector>


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
		std::vector<uint8_t> dest(srcSize + srcSize / 2 + 4);
		uint32_t destIndex = 0;
		//output source size
		*((uint32_t *)&dest[destIndex]) = srcSize;
		destIndex += 4;
		//compress data
		uint32_t srcIndex = 0;
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
				uint32_t length = 0;
				while (srcIndex < (srcSize - 1) && length < 255 && source[srcIndex] == 0) { ++length; ++srcIndex; }
				//store run length
				dest[destIndex++] = length;
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
		//decompress data
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
				uint32_t length = source[srcIndex++];
				for (uint32_t i = 0; i < length; ++i)
				{
					dest[destIndex++] = 0;
				}
			}
		}
		return dest;
	}
	return std::vector<uint8_t>();
}
