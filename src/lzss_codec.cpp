#include "lzss_codec.h"

#include "tools.h"
#include <array>
#include <list>
#include <numeric>
#include <algorithm>
#include "../sais/sais.hxx"

#include <iostream>

//#define DEBUG_OUTPUT 1


const uint8_t LZSS::CodecIdentifier = 70;

uint8_t LZSS::codecIdentifier() const
{
	return CodecIdentifier;
}

std::string LZSS::codecName() const
{
	return "LZSS";
}

LZSS * LZSS::Create()
{
	return new LZSS();
}

void LZSS::setCompressionParameters(const uint32_t dictionaryBits, const uint32_t matchLengthBits)
{
	m_dictionaryBits = dictionaryBits;
	if (m_dictionaryBits < 4)
	{
		m_dictionaryBits = 4;
	}
	else if (m_dictionaryBits > 20)
	{
		m_dictionaryBits = 20;
	}
	m_dictionarySize = (1 << m_dictionaryBits);
	m_lookAheadSize = (1 << (m_dictionaryBits - 3)); //look-ahead buffer size is 1/8 of dictionary size
	m_matchLengthBits = matchLengthBits;
	if (m_matchLengthBits < 3)
	{
		m_matchLengthBits = 3;
	}
	else if (m_matchLengthBits > 8)
	{
		m_matchLengthBits = 8;
	}
	m_matchLengthMin = ((m_dictionaryBits + m_matchLengthBits + 7) / 8) + 1;
	m_matchLengthMax = ((1 << m_matchLengthBits) - 1) + m_matchLengthMin;
}


void LZSS::outputVerbatim(uint8_t symbol, std::vector<uint8_t> & dest, uint32_t & destIndex, uint32_t & buffer, uint32_t & bufferBits) const
{
	// add leading 0 to bits
	buffer |= ((uint32_t)symbol) << (bufferBits - 9);
	bufferBits -= 9;
	Tools::outputBits(dest, destIndex, buffer, bufferBits);
}

void LZSS::outputMatch(int32_t index, int32_t length, std::vector<uint8_t> & dest, uint32_t & destIndex, uint32_t & buffer, uint32_t & bufferBits) const
{
	// add leading 1 to bits
	buffer |= 1 << (bufferBits - 1);
	bufferBits -= 1;
	// store match index in buffer
	buffer |= index << (bufferBits - m_dictionaryBits);
	bufferBits -= m_dictionaryBits;
	// store match length in buffer
	buffer |= (length - m_matchLengthMin) << (bufferBits - m_matchLengthBits);
	bufferBits -= m_matchLengthBits;
	Tools::outputBits(dest, destIndex, buffer, bufferBits);
}

std::vector<uint8_t> LZSS::encode(const std::vector<uint8_t> & source) const
{
	const int32_t srcSize = static_cast<int32_t>(source.size());
	if (srcSize > 0)
	{
		std::vector<uint8_t> dest(srcSize + 6 + srcSize / 8);
		uint32_t destIndex = 0;
		//output source size
		*((uint32_t *)&dest[destIndex]) = srcSize;
		destIndex += 4;
		//store dictionary bits and match length bits
		dest[destIndex++] = static_cast<uint8_t>(m_dictionaryBits);
		dest[destIndex++] = static_cast<uint8_t>(m_matchLengthBits);
		//check if source size is bigger that look-ahead buffer size
		if (srcSize > m_lookAheadSize)
		{
			int32_t dictStart = 0;
			int32_t labStart = 0;
			//allocate and clear left index array
			std::array<int32_t, 256> LI;
			std::array<int32_t, 256> RI;
			//allocate space for dictionary suffix arrays
			std::vector<int32_t> P(m_dictionarySize, -1);
			//push m_lookAheadSize bytes straight to output and move look-ahead buffer to next block
			std::copy(source.begin(), std::next(source.begin(), m_lookAheadSize), std::next(dest.begin(), destIndex));
			destIndex += m_lookAheadSize;
			labStart += m_lookAheadSize;
			// build initial suffix array
			saisxx<uint8_t *, int32_t *, int32_t>((uint8_t *)source.data(), P.data(), m_lookAheadSize);
			//do LZSS encoding
			uint32_t buffer = 0; //bit buffer holding encoded data
			uint32_t availableBits = 32; //number of available bits in buffer we can fill with data
			while (labStart < srcSize)
			{
#ifdef DEBUG_OUTPUT
				std::string dictString(std::next(source.cbegin(), dictStart), std::next(source.cbegin(), labStart));
				std::cout << "DICT: " << dictString;
#endif
				// ---------- Build LI and RI arrays ----------
				// clear left and right index array
				std::fill(LI.begin(), LI.end(), -1);
				std::fill(RI.begin(), RI.end(), -1);
				//set up dictionary size
				const int32_t dictSize = labStart - dictStart;
				//scan suffix array P and update left-index LI and right-index RI
				uint8_t previousSymbol = source[dictStart + P[0]];
				//store index for first symbol
				LI[previousSymbol] = 0;
				for (int32_t pIndex = 1; pIndex < dictSize; ++pIndex)
				{
					//store index for previous symbol if the current symbol is different
					const uint8_t currentSymbol = source[dictStart + P[pIndex]];
					if (currentSymbol != previousSymbol)
					{
						RI[previousSymbol] = pIndex - 1;
						LI[currentSymbol] = LI[currentSymbol] < 0 ? pIndex : LI[currentSymbol];
						previousSymbol = currentSymbol;
					}
				}
				//store index for last symbol
				RI[previousSymbol] = (uint32_t)dictSize - 1;
				// ---------- Encode symbols ----------
				//calculate next look-ahead buffer size. this is usually m_lookAheadSize, but can be less at the end of the file.
				const int32_t labSize = labStart + m_lookAheadSize > srcSize ? srcSize - labStart : m_lookAheadSize;
#ifdef DEBUG_OUTPUT
				std::string labString(std::next(source.cbegin(), labStart), std::next(source.cbegin(), labStart + labSize));
				std::cout << " LAB: " << labString << std::endl;
				std::cout << "P [";
				std::for_each(P.cbegin(), P.cend(), [](const int32_t & p){ std::cout << p << ","; });
				std::cout << "]" << std::endl;
#endif
				//encode symbols from look-ahead buffer
				int32_t labEncoded = 0;
				// try to find symbol in dictionary (through LI array)
				const int32_t leftIndex = LI[source[labStart + labEncoded]];
				if (leftIndex < 0)
				{
					//symbol not in dictionary. encode as single symbol
					outputVerbatim(source[labStart + labEncoded], dest, destIndex, buffer, availableBits);
#ifdef DEBUG_OUTPUT
					std::cout << source[labStart + labEncoded] << " -> not in dict" << std::endl;
#endif
					labEncoded++;
				}
				else
				{
					//symbol in dictionary, find right end of suffixes
					const int32_t rightIndex = RI[source[labStart + labEncoded]];
					//locate longest match for best compression
					int32_t matchLength = 1;
					int32_t matchIndex = 0;
					for (int32_t pIndex = leftIndex; pIndex <= rightIndex; ++pIndex)
					{
						//clamp search length to end of LAB, end of dictionary and maximum encodable length
						const int32_t maxCmpLength = std::min(std::min(dictSize - P[pIndex], labSize - labEncoded), m_matchLengthMax);
						if (maxCmpLength > m_matchLengthMin && maxCmpLength > matchLength)
						{
							//compare strings
							auto compareStart = std::next(source.cbegin(), labStart + labEncoded);
							const auto mismatches = std::mismatch(compareStart, std::next(compareStart, maxCmpLength), std::next(source.cbegin(), dictStart + P[pIndex]));
							const int32_t length = std::distance(compareStart, mismatches.first);
							if (length > matchLength)
							{
								matchLength = length;
								matchIndex = pIndex;
								//if we have reached the maximum encodable length, we don't need to search anymore
								if (matchLength == m_matchLengthMax)
								{
									break;
								}
							}
						}
					}
					//we have found a match now, check if long enough
					if (matchLength >= m_matchLengthMin)
					{
						// match found. output it
						outputMatch(P[matchIndex], matchLength, dest, destIndex, buffer, availableBits);
#ifdef DEBUG_OUTPUT
						std::string match(std::next(source.cbegin(), dictStart + P[matchIndex]), std::next(source.cbegin(), dictStart + P[matchIndex] + matchLength));
						std::cout << match << " -> match index " << P[matchIndex] << ", length " << matchLength << std::endl;
#endif
						labEncoded += matchLength;
					}
					else
					{
						//no. just encode bytes of match verbatim
						for (int32_t i = 0; i < matchLength; ++i)
						{
							outputVerbatim(source[labStart + labEncoded], dest, destIndex, buffer, availableBits);
#ifdef DEBUG_OUTPUT
							std::cout << source[labStart + labEncoded] << " -> match too short" << std::endl;
#endif
							labEncoded++;
						}
					}
				}
				// advance dictionary and LAB
				labStart += labEncoded;
				dictStart = labStart <= m_dictionarySize ? 0 : (labStart - m_dictionarySize);
				// rebuild suffix array
				saisxx<uint8_t *, int32_t *, int32_t>((uint8_t *)source.data() + dictStart, P.data(), labStart - dictStart);
			}
			//now if we still have remaining bits, dump buffer byte, which automatically adds the bits plus trailing zero bits
			Tools::outputBits(dest, destIndex, buffer, availableBits, true);
		}
		else
		{
			//source size is too small, do simple copy
			std::copy(source.cbegin(), source.cend(), std::next(dest.begin(), destIndex));
			destIndex += srcSize;
		}
		dest.resize(destIndex);
		return dest;
	}
	return std::vector<uint8_t>();
}

std::vector<uint8_t> LZSS::decode(const std::vector<uint8_t> & source) const
{
	const uint32_t srcSize = static_cast<uint32_t>(source.size());
	if (srcSize > 0)
	{
		// read result size
		uint32_t srcIndex = 0;
		const uint32_t destSize = *((uint32_t *)&source[srcIndex]);
		srcIndex += 4;
		// allocate destination data
		std::vector<uint8_t> dest(destSize);
		uint32_t destIndex = 0;
		// read match dictionary bits and length bits
		const int32_t dictionaryBits = source[srcIndex++];
		const int32_t matchLengthBits = source[srcIndex++];
		// calculate dictionary and look-ahead-buffer size
		const int32_t dictionarySize = (1 << dictionaryBits);
		const int32_t lookAheadSize = (1 << (dictionaryBits - 3)); //look-ahead buffer size is 1/8 of dictionary size
		// check if the source size is bigger than the look-ahead-buffer size
		if (destSize > lookAheadSize)
		{
			// calculate the minimum match. we need to add this to the encoded match length
			const int32_t matchLengthMin = ((dictionaryBits + matchLengthBits + 7) / 8) + 1;
			// check what the minimum amount of bits per message is (encoded or un-encoded)
			const int32_t minCodeLength = 1 + std::min(dictionaryBits + matchLengthBits, 8);
			// copy lookAheadSize symbols straight to the output / dictionary
			std::copy(std::next(source.begin(), srcIndex), std::next(source.begin(), srcIndex + lookAheadSize), dest.begin());
			destIndex += lookAheadSize;
			srcIndex += lookAheadSize;
			//apply reverse LZSS encoding
			int32_t dictStart = 0;
			uint32_t buffer = 0;
			uint8_t availableBits = 32;
			while ((destIndex < destSize) && (availableBits >= minCodeLength))
			{
				// fill buffer from input
				while ((availableBits >= 8) && (srcIndex < srcSize))
				{
					buffer |= source[srcIndex++] << (availableBits - 8);
					availableBits -= 8;
				}
				const bool isEncoded = (bool)(buffer >> 31);
				buffer <<= 1;
				availableBits += 1;
				// check first bit (encoded / un-encoded)
				if (isEncoded)
				{
					// bit is 1. get string index and length bits from source
					const uint32_t stringIndex = buffer >> (32 - dictionaryBits);
					buffer <<= dictionaryBits;
					availableBits += dictionaryBits;
					const uint32_t stringLength = (buffer >> (32 - matchLengthBits)) + matchLengthMin;
					buffer <<= matchLengthBits;
					availableBits += matchLengthBits;
					// copy symbol string from dictionary
					std::copy(std::next(dest.begin(), dictStart + stringIndex), std::next(dest.begin(), dictStart + stringIndex + stringLength), std::next(dest.begin(), destIndex));
					destIndex += stringLength;
					//std::string match(std::next(dest.begin(), dictStart + stringIndex), std::next(dest.begin(), dictStart + stringIndex + stringLength));
					//std::cout << match << " -> match index " << stringIndex << ", length " << stringLength << std::endl;
				}
				else
				{
					// bit is 0. copy following symbol bits verbatim
					//std::cout << (uint8_t)(buffer >> 24) << " -> verbatim" << std::endl;
					dest[destIndex++] = buffer >> 24;
					buffer <<= 8;
					availableBits += 8;
				}
				// advance dictionary
				dictStart = (destIndex - dictStart) <= dictionarySize ? 0 : (destIndex - dictionarySize);
			}
		}
		else
		{
			// source size is too small, do simple copy
			std::copy(std::next(source.cbegin(), srcIndex), source.cend(), dest.begin());
			//destIndex += destSize;
		}
		return dest;
	}
	return std::vector<uint8_t>();
}