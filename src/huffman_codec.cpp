#include "huffman_codec.h"

#include "tools.h"
#include <array>
#include <queue>
#include <algorithm>
#include <iostream>
#include <numeric>


const uint8_t StaticHuffman::CodecIdentifier = 60;

uint8_t StaticHuffman::codecIdentifier() const
{
	return CodecIdentifier;
}

std::string StaticHuffman::codecName() const
{
	return "Static Huffman";
}

I_Codec * StaticHuffman::Create()
{
	return new StaticHuffman();
}

void StaticHuffman::setDecodeMethod(uint32_t method)
{
	m_decodeMethod = method;
}

//-------------------------------------------------------------------------------------------------

StaticHuffman::Frequencies StaticHuffman::frequenciesFromData(const std::vector<uint8_t> & source) const
{
	//count frequencies in data
	Frequencies frequencies;
	std::fill(frequencies.begin(), frequencies.end(), 0);
	for (uint32_t i = 0; i < source.size(); ++i)
	{
		frequencies[source[i]]++;
	}
	//just halve frequencies
	uint32_t minimum = UINT32_MAX;
	for (uint32_t i = 0; i < 256; ++i)
	{
		if (frequencies[i] > 0)
		{
			minimum = (frequencies[i] < minimum) ? frequencies[i] : minimum;
		}
	}
	//find out what to divide by
	const uint32_t shift = minimum > 2 ? Tools::highestBitSet(minimum) : 0;
	if (shift > 0)
	{
		for (uint32_t i = 0; i < 256; ++i)
		{
			//adjust frequencies by dividing, but make sure frequencies > 0 stay > 0
			if (frequencies[i] > 0)
			{
				frequencies[i] = (frequencies[i] >> shift) | 1;
			}
		}
	}
	return frequencies;
}

//-------------------------------------------------------------------------------------------------

struct TreeNode
{
	TreeNode * parent; /// @brief Parent node in tree. Uninitialized nodes and the root node has parent == NULL.
	TreeNode * leftChild; /// @brief left side child index of this node. valid if != NULL.
	TreeNode * rightChild; /// @brief right side child index of this node. valid if != NULL.
	uint32_t weight; /// @brief The frequency or weight of the node.
	uint8_t symbol; /// @brief The actual symbol value if this is a leaf node (leftChild == 0 && rightChild == 0).
};

class SortByWeightAscending
{
public:
	bool operator()(const TreeNode * a, const TreeNode * b) const
	{
		return a->weight > b->weight;
	}
};

void buildCodesFromTree(HuffmanCodes & codes, TreeNode * node, uint16_t code = 0, uint8_t codeLength = 0)
{
	//check if it is a leaf node and not an intermediate one
	if (node->leftChild == nullptr && node->rightChild == nullptr)
	{
		//assign code to current node
		if (node->weight > 0)
		{
			codes[node->symbol].symbol = node->symbol;
			codes[node->symbol].code = code;
			codes[node->symbol].length = codeLength;
		}
		else
		{
			codes[node->symbol].symbol = node->symbol;
			codes[node->symbol].code = 0;
			codes[node->symbol].length = 255;
		}
	}
	//assign a 0 for a left node child, a 1 for a right node child
	if (node->leftChild != nullptr)
	{
		buildCodesFromTree(codes, node->leftChild, code << 1, codeLength + 1);
	}
	if (node->rightChild != nullptr)
	{
		buildCodesFromTree(codes, node->rightChild, (code << 1) | 1, codeLength + 1);
	}
}

void deleteTreeNodes(TreeNode * node)
{
	if (node->leftChild != nullptr)
	{
		deleteTreeNodes(node->leftChild);
	}
	if (node->rightChild != nullptr)
	{
		deleteTreeNodes(node->rightChild);
	}
	delete node;
}

HuffmanCodes StaticHuffman::codesFromFrequencies(StaticHuffman::Frequencies frequencies, uint8_t allowedCodeLength) const
{
	HuffmanCodes codes(256);
	//loop while the maximum code length has been exceeded
	uint8_t maxCodeLength = 0;
	do
	{
		std::vector<TreeNode*> nodes(256);
		std::priority_queue<TreeNode*, std::vector<TreeNode*>, SortByWeightAscending> queue;
		//initialize leaves from frequencies
		for (uint32_t i = 0; i < 256; ++i)
		{
			TreeNode * leaf = new TreeNode();
			leaf->parent = NULL;
			leaf->leftChild = NULL;
			leaf->rightChild = NULL;
			leaf->symbol = i;
			leaf->weight = frequencies[i];
			queue.push(leaf);
		}
		//now start building tree. when only the root node is left, we are done.
		while (queue.size() > 1)
		{
			//remove the two nodes of lowest probability
			TreeNode * left = queue.top(); queue.pop();
			TreeNode * right = queue.top(); queue.pop();
			//build new node combining frequencies
			TreeNode * combined = new TreeNode();
			combined->parent = NULL;
			combined->leftChild = left;
			combined->rightChild = right;
			combined->weight = left->weight + right->weight;
			combined->symbol = 0;
			left->parent = combined;
			right->parent = combined;
			queue.push(combined);
		}
		//now build codes from tree
		TreeNode * root = queue.top(); queue.pop();
		buildCodesFromTree(codes, root);
		//convert to canonical codes
		codes = convertToCanonicalCodes(codes);
		//free data
		deleteTreeNodes(root);
		//check if maximum code length is ok
		maxCodeLength = 0;
		auto citer = codes.cbegin();
		while (citer != codes.cend())
		{
			maxCodeLength = maxCodeLength < citer->length ? citer->length : maxCodeLength;
			++citer;
		}
		if (maxCodeLength > allowedCodeLength)
		{
			if (m_verbose) std::cout << "Maximum code length is " << (uint32_t)maxCodeLength << ". Reducing..." << std::endl;
			//reduce frequencies and build codes again
			auto fiter = frequencies.begin();
			while (fiter != frequencies.end())
			{
				*fiter = *fiter > 0 ? (*fiter >> 1) | 1 : *fiter;
				++fiter;
			}
		}
	} while (maxCodeLength > allowedCodeLength);
	if (m_verbose) std::cout << "Maximum code length is " << (uint32_t)maxCodeLength << "." << std::endl;
	//Dump codes
	/*std::cout << "Huffman codes:" << std::endl;
	for (uint16_t i = 0; i < 256; ++i)
	{
	std::cout << (uint32_t)codes[i].code << std::endl;
	}*/
	return codes;
}

//-------------------------------------------------------------------------------------------------

std::vector<uint8_t> StaticHuffman::encode(const std::vector<uint8_t> & source) const
{
	const uint32_t srcSize = static_cast<uint32_t>(source.size());
	if (srcSize > 0)
	{
		std::vector<uint8_t> dest(srcSize + 4 + 128 + (srcSize / 8));
		uint32_t destIndex = 0;
		//output source size
		*((uint32_t *)&dest[destIndex]) = srcSize;
		destIndex += 4;
		//build Huffman codes from data
		if (m_verbose) std::cout << "Generating Huffman codes... ";
		Frequencies frequencies = frequenciesFromData(source);
		HuffmanCodes codes = codesFromFrequencies(frequencies);
		if (m_verbose) std::cout << "Done." << std::endl;
		if (m_verbose) std::cout << "Compressing with static Huffman encoder... ";
		//the codes ascending by symbols for easier encoding
		std::sort(codes.begin(), codes.end(), [](const HuffmanCode & a, const HuffmanCode & b){ return a.symbol < b.symbol; });
		//output code lengths
		uint32_t buffer = 0; //bit buffer holding encoded data
		uint32_t availableBits = 32; //number of available bits in buffer we can fill with data
		for (uint32_t i = 0; i < 256; ++i)
		{
			const uint32_t length = (codes.at(i).length <= 15) ? codes.at(i).length : 0;
			//convert length to nibble and store in buffer
			buffer |= length << (availableBits - 4);
			availableBits -= 4;
			//if the buffer has a short or byte available, output it
			Tools::outputBits(dest, destIndex, buffer, availableBits);
		}
		//output compressed data
		buffer = 0;
		availableBits = 32;
		for (uint32_t i = 0; i < srcSize; ++i)
		{
			const uint8_t symbol = source[i];
			//get code for symbol from Huffman table and insert at current bit position in buffer
			buffer |= (uint32_t)codes.at(symbol).code << (availableBits - codes.at(symbol).length);
			availableBits -= codes.at(symbol).length;
			//if the buffer has a short or byte available, output it
			Tools::outputBits(dest, destIndex, buffer, availableBits);
		}
		//now if we still have remaining bits, dump buffer byte, which automatically adds the bits plus trailing zero bits
		Tools::outputBits(dest, destIndex, buffer, availableBits, true);
		if (m_verbose) std::cout << "Done." << std::endl;
		dest.resize(destIndex);
		return dest;
	}
	return std::vector<uint8_t>();
}

//------------------------------------------------------------------------------------------------

StaticHuffman::CodeLengths StaticHuffman::getCodeLengthsFromHeader(const std::vector<uint8_t> & source, uint32_t & index, uint8_t & minLength, uint8_t & maxLength) const
{
	//read code lengths from data
	CodeLengths codeLengths;
	for (uint16_t i = 0; i < 256;)
	{
		const uint8_t current = source[index++];
		codeLengths[i++] = current >> 4;
		codeLengths[i++] = current & 0x0F;
	}
	//find min/max code lengths
	minLength = 15;
	maxLength = 0;
	for (uint16_t i = 0; i < 256; ++i)
	{
		//find min/max code lengths, but don't take into account zero code lengths == invalid symbols
		minLength = codeLengths[i] > 0 && codeLengths[i] < minLength ? codeLengths[i] : minLength;
		maxLength = codeLengths[i] > 0 && codeLengths[i] > maxLength ? codeLengths[i] : maxLength;
	}
	return codeLengths;
}

HuffmanCodes StaticHuffman::getCodesFromHeader(const std::vector<uint8_t> & source) const
{
	HuffmanCodes codes(256);
	//check minimum data size (length size + lengths + uncompressed size)
	if (source.size() > 132)
	{
		uint32_t index = 0;
		//read code lengths from data
		uint8_t minCodeLength = 15;
		uint8_t maxCodeLength = 0;
		const CodeLengths codeLengths = getCodeLengthsFromHeader(source, index, minCodeLength, maxCodeLength);
		//clear codes and symbols
		for (uint16_t i = 0; i < 256; ++i)
		{
			codes[i].code = 0;
			codes[i].length = 0;
			codes[i].symbol = (uint8_t)i;
		}
		//build canonical codes from lengths. this also sorts codes by codeword length and then symbol
		uint16_t codeIndex = 0;
		uint16_t currentCode = 0;
		for (uint16_t i = minCodeLength; i <= maxCodeLength; ++i)
		{
			for (uint16_t j = 0; j < 256; ++j)
			{
				if (codeLengths[j] == i)
				{
					codes[codeIndex].symbol = (uint8_t)j;
					codes[codeIndex].length = (uint8_t)i;
					codes[codeIndex].code = currentCode++;
					codeIndex++;
				}
			}
			currentCode <<= 1;
		}
	}
	return codes;
}

//------------------------------------------------------------------------------------------------

std::vector<uint8_t> StaticHuffman::decode(const std::vector<uint8_t> & source) const
{
	//check minimum data size (length size + lengths + uncompressed size)
	if (source.size() > 132)
	{
		switch (m_decodeMethod)
		{
			case 1:
				return decode1(source);
				break;
			case 2:
				return decode2(source);
				break;
			case 3:
				return decode3(source);
				break;
			default:
				return decode0(source);
				break;
		}
	}
	return std::vector<uint8_t>();
}

std::vector<uint8_t> StaticHuffman::decode0(const std::vector<uint8_t> & source) const
{
	const uint32_t srcSize = static_cast<uint32_t>(source.size());
	if (srcSize > 132)
	{
		//read result size
		uint32_t srcIndex = 0;
		const uint32_t destSize = *((uint32_t *)&source[srcIndex]);
		srcIndex += 4;
		//allocate destination data
		std::vector<uint8_t> dest(destSize);
		uint32_t destIndex = 0;
		//read code lengths from data
		uint8_t minCodeLength = 15;
		uint8_t maxCodeLength = 0;
		const CodeLengths codeLengths = getCodeLengthsFromHeader(source, srcIndex, minCodeLength, maxCodeLength);
		//clear codes and symbols
		std::array<uint8_t, 256> symbols;
		std::iota(symbols.begin(), symbols.end(), 0);
		std::array<uint16_t, 256> codes;
		std::fill(codes.begin(), codes.end(), 0);
		//build codes for all lengths
		std::array<uint8_t, 16> codeLengthStarts;
		std::fill(codeLengthStarts.begin(), codeLengthStarts.end(), 0);
		std::array<uint8_t, 16> codeLengthEnds;
		std::fill(codeLengthEnds.begin(), codeLengthEnds.end(), 0);
		//build canonical codes from lengths. this also sorts codes by codeword length
		uint16_t codeIndex = 0;
		uint16_t currentCode = 0;
		for (uint16_t i = minCodeLength; i <= maxCodeLength; ++i)
		{
			codeLengthStarts[i] = (uint8_t)codeIndex;
			for (uint16_t j = 0; j < 256; ++j)
			{
				if (codeLengths[j] == i)
				{
					symbols[codeIndex] = (uint8_t)j;
					codes[codeIndex++] = currentCode++;
				}
			}
			currentCode <<= 1;
			codeLengthEnds[i] = codeIndex - 1;
		}
		//decode data by searching code table in increasing size of code lengths
		//we start with the minimum code length and work our way towards the higher ones...
		uint32_t buffer = 0;
		uint8_t bits = 32;
		while (destIndex < destSize && (srcIndex < srcSize || (32 - bits) >= minCodeLength))
		{
			//read byte from input
			buffer |= source[srcIndex++] << (bits - 8);
			bits -= 8;
			//try to match code
			int8_t codeLength = minCodeLength;
			while (codeLength <= (32 - bits) && codeLength <= maxCodeLength && destIndex < dest.size())
			{
				//read code from buffer and mask with code length
				const uint16_t codeWord = (uint16_t)(buffer >> (32 - codeLength)) & (0xFFFF >> (16 - codeLength));
				//search through all codes of current code length
				for (uint16_t i = codeLengthStarts[codeLength]; i <= codeLengthEnds[codeLength]; ++i)
				{
					if (codeWord == codes[i])
					{
						//code matches. store symbol and remove bits from buffer
						dest[destIndex++] = symbols[i];
						buffer <<= codeLength;
						bits += codeLength;
						//set current code length to minCodeLength - 1. this will be increased by 1 when leaving the loop
						//thus we start searching for a new symbol again if have enough bits left...
						codeLength = minCodeLength - 1;
						break;
					}
				}
				codeLength++;
			}
		}
		return dest;
	}
	return std::vector<uint8_t>();
}

std::vector<uint8_t> StaticHuffman::decode1(const std::vector<uint8_t> & source) const
{
	const uint32_t srcSize = static_cast<uint32_t>(source.size());
	if (srcSize > 132)
	{
		//read result size
		uint32_t srcIndex = 0;
		const uint32_t destSize = *((uint32_t *)&source[srcIndex]);
		srcIndex += 4;
		//allocate destination data
		std::vector<uint8_t> dest(destSize);
		uint32_t destIndex = 0;
		//read code lengths from data
		uint8_t minCodeLength = 15;
		uint8_t maxCodeLength = 0;
		const CodeLengths codeLengths = getCodeLengthsFromHeader(source, srcIndex, minCodeLength, maxCodeLength);
		//clear codes and symbols
		std::array<uint8_t, 256> symbols;
		std::iota(symbols.begin(), symbols.end(), 0);
		std::array<uint16_t, 256> codes;
		std::fill(codes.begin(), codes.end(), 0);
		//build codes for all lengths
		std::array<uint16_t, 16> firstCode;
		std::fill(firstCode.begin(), firstCode.end(), 0);
		std::array<uint8_t, 16> codeLengthStarts;
		std::fill(codeLengthStarts.begin(), codeLengthStarts.end(), 0);
		//build canonical codes from lengths. this also sorts codes by codeword length and then symbol index
		uint16_t codeIndex = 0;
		uint16_t currentCode = 0;
		for (uint16_t i = minCodeLength; i <= maxCodeLength; ++i)
		{
			firstCode[i] = currentCode;
			codeLengthStarts[i] = (uint8_t)codeIndex;
			for (uint16_t j = 0; j < 256; ++j)
			{
				if (codeLengths[j] == i)
				{
					symbols[codeIndex] = (uint8_t)j;
					codes[codeIndex++] = currentCode++;
				}
			}
			currentCode <<= 1;
		}
		//decode data by indexing into code table. we start with the minimum code length and work our way towards maximum, codelength...
		uint32_t buffer = 0;
		uint8_t bits = 32;
		while (destIndex < destSize && (srcIndex < srcSize || (32 - bits) >= minCodeLength))
		{
			//read byte from input
			buffer |= source[srcIndex++] << (bits - 8);
			bits -= 8;
			//try to match code
			int8_t codeLength = minCodeLength;
			while (codeLength <= maxCodeLength && codeLength <= (32 - bits))
			{
				//read code from buffer and mask with code length
				uint16_t codeWord = (uint16_t)(buffer >> (32 - codeLength));
				//calculate absolute index in code array
				uint16_t symbolIndex = ((uint16_t)codeLengthStarts[codeLength] + codeWord) - firstCode[codeLength];
				//if symbol index is in range check if code word matches
				if (codeWord == codes[symbolIndex])
				{
					//code matches. store symbol and remove bits from buffer
					dest[destIndex++] = symbols[symbolIndex];
					buffer <<= codeLength;
					bits += codeLength;
					//check if want to quit now, or break because there are not enough bits left anyway
					if (minCodeLength > bits || destIndex >= dest.size())
					{
						break;
					}
					//set current code length to minCodeLength - 1. this will be increased by 1 when leaving the loop
					//thus we start searching for a new symbol again if have enough bits left...
					codeLength = minCodeLength - 1;
				}
				codeLength++;
			}
		}
		return dest;
	}
	return std::vector<uint8_t>();
}

std::vector<uint8_t> StaticHuffman::decode2(const std::vector<uint8_t> & source) const
{
	const uint32_t srcSize = static_cast<uint32_t>(source.size());
	if (srcSize > 132)
	{
		//read result size
		uint32_t srcIndex = 0;
		const uint32_t destSize = *((uint32_t *)&source[srcIndex]);
		srcIndex += 4;
		//allocate destination data
		std::vector<uint8_t> dest(destSize);
		uint32_t destIndex = 0;
		//read code lengths from data
		uint8_t minCodeLength = 15;
		uint8_t maxCodeLength = 0;
		const CodeLengths codeLengths = getCodeLengthsFromHeader(source, srcIndex, minCodeLength, maxCodeLength);
		//clear codes and symbols and find min/max code lengths
		std::array<uint8_t, 256> symbols;
		std::iota(symbols.begin(), symbols.end(), 0);
		std::array<uint16_t, 256> codes;
		std::fill(codes.begin(), codes.end(), 0);
		//build codes for all lengths
		std::array<int16_t, 16> indexHelper;
		std::fill(indexHelper.begin(), indexHelper.end(), 0);
		//build canonical codes from lengths. this also sorts codes by codeword length and then symbol index
		uint16_t codeIndex = 0;
		uint16_t currentCode = 0;
		for (uint16_t i = minCodeLength; i <= maxCodeLength; ++i)
		{
			//calculate helper value consisting of index to start of code word and first code word
			//this is subtracted from every code word in the decode loop to generate and index into the code word table
			indexHelper[i] = currentCode - codeIndex;
			for (uint16_t j = 0; j < 256; ++j)
			{
				if (codeLengths[j] == i)
				{
					symbols[codeIndex] = (uint8_t)j;
					codes[codeIndex++] = currentCode++;
				}
			}
			currentCode <<= 1;
		}
		//decode data by indexing into code table. we start with the minimum code length and work our way towards maximum, codelength...
		uint32_t buffer = 0;
		uint8_t bits = 0;
		while (destIndex < destSize && (srcIndex < srcSize || bits >= minCodeLength))
		{
			//read byte from input if needed and available
			if (bits < maxCodeLength && srcIndex < srcSize)
			{
				buffer = (buffer << 8) | (uint32_t)source[srcIndex++];
				bits += 8;
			}
			//try to match code
			int8_t codeLength = minCodeLength;
			while (codeLength <= maxCodeLength && codeLength <= bits)
			{
				//read code from buffer and mask with code length
				uint16_t codeWord = (uint16_t)(buffer >> (bits - codeLength));
				//calculate absolute index in code array
				int16_t symbolIndex = (int16_t)codeWord - indexHelper[codeLength];
				//check if code word matches
				if (codeWord == codes[symbolIndex])
				{
					//code matches. store symbol and remove bits from buffer
					dest[destIndex++] = symbols[symbolIndex];
					//remove bits from buffer. mask out the bits we've just consumed
					bits -= codeLength;
					buffer &= ~(0xFFFFFFFF << bits);
					break;
				}
				codeLength++;
			}
		}
		return dest;
	}
	return std::vector<uint8_t>();
}

std::vector<uint8_t> StaticHuffman::decode3(const std::vector<uint8_t> & source) const
{
	const uint32_t srcSize = static_cast<uint32_t>(source.size());
	if (srcSize > 132)
	{
		//read result size
		uint32_t srcIndex = 0;
		const uint32_t destSize = *((uint32_t *)&source[srcIndex]);
		srcIndex += 4;
		//allocate destination data
		std::vector<uint8_t> dest(destSize);
		uint32_t destIndex = 0;
		//read code lengths from data
		uint8_t minCodeLength = 15;
		uint8_t maxCodeLength = 0;
		const CodeLengths codeLengths = getCodeLengthsFromHeader(source, srcIndex, minCodeLength, maxCodeLength);
		//build canonical codes from lengths and sort symbols by codeword length and then symbol index
		std::array<uint8_t, 16> codeLengthCount;
		std::iota(codeLengthCount.begin(), codeLengthCount.end(), 0);
		std::array<uint8_t, 256> symbols;
		std::iota(symbols.begin(), symbols.end(), 0);
		uint8_t codeIndex = 0;
		for (uint8_t i = minCodeLength; i <= maxCodeLength; ++i)
		{
			codeLengthCount[i] = 0;
			for (uint16_t j = 0; j < 256; ++j)
			{
				if (codeLengths[j] == i)
				{
					codeLengthCount[i]++;
					symbols[codeIndex++] = (uint8_t)j;
				}
			}
		}
		// 		std::copy(symbols.cbegin(), symbols.cend(), std::ostream_iterator<int>(std::cout, " "));
		// 		std::cout << std::endl;
		//decode data by straight indexing into symbol table. we start with the minimum code length and work our way towards maximum code length...
		uint32_t buffer = 0;
		uint8_t bits = 0;
		uint16_t currentCode = 0;
		uint8_t symbolStartIndex = 0;
		uint8_t currentCodeLength = minCodeLength;
		while (destIndex < destSize && (srcIndex < srcSize || bits >= minCodeLength))
		{
			//read byte from input if needed
			if (bits < maxCodeLength && srcIndex < srcSize)
			{
				buffer = (buffer << 8) | (uint32_t)source[srcIndex++];
				bits += 8;
			}
			//try to match code
			while (currentCodeLength <= maxCodeLength && currentCodeLength <= bits)
			{
				//read code from buffer and mask with code length
				const uint16_t codeWord = (uint16_t)(buffer >> (bits - currentCodeLength));
				//check if the code word can actually be one of the codes of our current code length
				if (codeWord >= currentCode)
				{
					//calculate absolute index in code array
					const uint8_t codeIndex = codeWord - currentCode;
					//check if code word matches
					if (codeIndex < codeLengthCount[currentCodeLength])
					{
						//code matches. store symbol and remove bits from buffer
						dest[destIndex++] = symbols[symbolStartIndex + codeIndex];
						//remove bits from buffer. mask out the bits we've just consumed
						bits -= currentCodeLength;
						buffer &= ~(0xFFFFFFFF << bits);
						currentCode = 0;
						symbolStartIndex = 0;
						currentCodeLength = minCodeLength;
						break;
					}
				}
				currentCode = (currentCode + codeLengthCount[currentCodeLength]) << 1;
				symbolStartIndex += codeLengthCount[currentCodeLength];
				currentCodeLength++;
			}
		}
		return dest;
	}
	return std::vector<uint8_t>();
}