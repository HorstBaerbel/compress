#include "huffman_codes.h"

#include <algorithm>


bool operator==(const HuffmanCode & a, const HuffmanCode & b)
{
	return (a.code == b.code && a.length == b.length) && ((a.symbol == b.symbol) || (a.code == 0 && a.length == 0));
}

bool operator!=(const HuffmanCode & a, const HuffmanCode & b)
{
	return !(a == b);
}

bool sortByLengthFirstThenSymbolAscending(const HuffmanCode & a, const HuffmanCode & b)
{
	return (a.length < b.length) || (a.length == b.length && a.symbol < b.symbol);
}

HuffmanCodes convertToCanonicalCodes(const HuffmanCodes & codes)
{
	HuffmanCodes canonical(256);
	std::copy(codes.cbegin(), codes.cend(), canonical.begin());
	//sort codes by length first, then by alphabet value
	std::sort(canonical.begin(), canonical.end(), sortByLengthFirstThenSymbolAscending);
	//now replace the codes, keeping the length
	uint16_t currentCode = 0;
	uint8_t currentLength = canonical.at(0).length;
	HuffmanCodes::iterator iter = canonical.begin();
	while (iter != canonical.end())
	{
		if (iter->length < 255)
		{
			//if we've reached a longer code word length, shift the current code left to account for that
			currentCode = (currentLength < iter->length) ? (currentCode << (iter->length - currentLength)) : currentCode;
			//store new code and increase current code word
			iter->code = currentCode++;
			currentLength = iter->length;
		}
		else
		{
			iter->code = 0;
			iter->length = 0;
		}
		++iter;
	}
	return canonical;
}