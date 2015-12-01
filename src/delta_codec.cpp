#include "delta_codec.h"


const uint8_t Delta::CodecIdentifier = 20;

uint8_t Delta::codecIdentifier() const
{
	return CodecIdentifier;
}

std::string Delta::codecName() const
{
	return "Delta";
}

I_Codec * Delta::Create()
{
	return new Delta();
}

std::vector<uint8_t> Delta::encode(const std::vector<uint8_t> & source) const
{
	const uint32_t srcSize = static_cast<uint32_t>(source.size());
	if (srcSize > 0)
	{
		std::vector<uint8_t> dest(srcSize);
		uint32_t destIndex = 0;
		//output first symbol verbatim
		dest[destIndex++] = source[0];
		int16_t lastSymbol = source[0];
		//output deltas
		for (uint32_t i = 1; i < srcSize; ++i)
		{
			const int16_t symbol = source[i];
			//calculate delta and wrap around absolute values higher that 128
			int16_t delta = (int8_t)((lastSymbol - symbol) ^ 256);
			//zig-zag encode delta value
			uint8_t zigZag = (uint8_t)((delta << 1) ^ (delta >> 16));
			dest[destIndex++] = zigZag;
			lastSymbol = symbol;
		}
		return dest;
	}
	return std::vector<uint8_t>();
}

std::vector<uint8_t> Delta::decode(const std::vector<uint8_t> & source) const
{
	const uint32_t srcSize = static_cast<uint32_t>(source.size());
	if (srcSize > 0)
	{
		//read result size
		uint32_t srcIndex = 0;
		//allocate destination data
		std::vector<uint8_t> dest(srcSize);
		uint32_t destIndex = 0;
		//read first symbol verbatim
		uint8_t lastSymbol = source[0];
		dest[destIndex++] = lastSymbol;
		//output deltas
		for (uint32_t i = 1; i < srcSize; ++i)
		{
			const uint8_t zigZag = source[i];
			//reverse zig-zag encoding
			int16_t delta = ((uint16_t)zigZag >> 1) ^ (-((uint16_t)zigZag & 1));
			//calculate value from last value and delta
			uint8_t value = (uint8_t)(lastSymbol - delta);
			dest[destIndex++] = value;
			lastSymbol = value;
		}
		return dest;
	}
	return std::vector<uint8_t>();
}
