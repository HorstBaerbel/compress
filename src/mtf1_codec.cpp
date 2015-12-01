#include "mtf1_codec.h"

#include <array>
#include <numeric>


const uint8_t Mtf1::CodecIdentifier = 50;

uint8_t Mtf1::codecIdentifier() const
{
	return CodecIdentifier;
}

std::string Mtf1::codecName() const
{
	return "Move-to-front-1";
}

I_Codec * Mtf1::Create()
{
	return new Mtf1();
}

std::vector<uint8_t> Mtf1::encode(const std::vector<uint8_t> & source) const
{
	const uint32_t srcSize = static_cast<uint32_t>(source.size());
	if (srcSize > 0)
	{
		std::vector<uint8_t> dest(srcSize);
		uint32_t destIndex = 0;
		//set up symbol table
		std::array<uint8_t, 256> symbols;
		std::iota(symbols.begin(), symbols.end(), 0);
		//apply MTF encoding
		for (uint32_t i = 0; i < srcSize; ++i)
		{
			uint8_t symbol = source[i];
			//find symbol in source
			uint32_t index = 0;
			while (symbols[index] != symbol) { ++index; }
			//output index
			dest[i] = index;
			//move symbol to position 0 if it was already at 1, else move it to 1
			uint32_t newIndex = (index <= 1) ? 0 : 1;
			while (newIndex < index) { symbols[index] = symbols[index - 1]; --index; }
			symbols[newIndex] = symbol;
		}
		return dest;
	}
	return std::vector<uint8_t>();
}

std::vector<uint8_t> Mtf1::decode(const std::vector<uint8_t> & source) const
{
	const uint32_t srcSize = static_cast<uint32_t>(source.size());
	if (srcSize > 0)
	{
		uint32_t srcIndex = 0;
		//allocate destination data
		std::vector<uint8_t> dest(srcSize);
		uint32_t destIndex = 0;
		//set up symbol table
		std::array<uint8_t, 256> symbols;
		std::iota(symbols.begin(), symbols.end(), 0);
		//apply MTF encoding
		for (uint32_t i = 0; i < srcSize; ++i)
		{
			uint32_t index = source[i];
			uint8_t symbol = symbols[index];
			//output symbol
			dest[i] = symbol;
			//move symbol to position 0 if it was already at 1, else move it to 1
			uint32_t newIndex = (index <= 1) ? 0 : 1;
			while (newIndex < index) { symbols[index] = symbols[index - 1]; --index; }
			symbols[newIndex] = symbol;
		}
		return dest;
	}
	return std::vector<uint8_t>();
}