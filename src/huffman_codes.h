#pragma once

#include <inttypes.h>
#include <vector>


/// @brief Huffman code structure.
struct HuffmanCode
{
	uint8_t symbol;
	uint32_t code;
	uint8_t length;
};

/// @brief Array of Huffman codes for symbols.
typedef std::vector<HuffmanCode> HuffmanCodes; 

/// @brief Huffman code equality operator.
/// @param a First Huffman code.
/// @param b Second Huffman code.
/// @return Returns true if the codes are equal.
bool operator==(const HuffmanCode & a, const HuffmanCode & b);

/// @brief Huffman code inequality operator.
/// @param a First Huffman code.
/// @param b Second Huffman code.
/// @return Returns true if the codes are not equal.
bool operator!=(const HuffmanCode & a, const HuffmanCode & b);

/// @brief Covert Huffman codes to canonical codes.
/// @param codes Huffman codes to convert.
/// @return Returns the codes converted to canonical form.
HuffmanCodes convertToCanonicalCodes(const HuffmanCodes & codes);
