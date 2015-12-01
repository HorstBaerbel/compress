#include "compressor.h"

#include "huffman_codec.h"
#include "delta_codec.h"
#include "bwt_codec.h"
#include "mtf1_codec.h"
#include "rgb2planes_codec.h"
#include "rle0_codec.h"
#include "tools.h"

#include <iostream>


const uint32_t Compressor::MagicHeader = 0x434D5035; //"CMP5" == "CoMPre5sor" data version 5

const std::map<uint8_t, I_Codec::Creator> Compressor::m_codecs = {
	std::make_pair(Bwt::CodecIdentifier, (I_Codec::Creator)Bwt::Create),
	std::make_pair(Delta::CodecIdentifier, Delta::Create),
	std::make_pair(StaticHuffman::CodecIdentifier, StaticHuffman::Create),
	std::make_pair(Mtf1::CodecIdentifier, Mtf1::Create),
	std::make_pair(RgbToPlanes::CodecIdentifier, RgbToPlanes::Create),
	std::make_pair(Rle0::CodecIdentifier, Rle0::Create)};

void Compressor::setVerboseOutput(bool verbose)
{
	m_verbose = verbose;
}

std::vector<uint8_t> Compressor::compress(const std::vector<uint8_t> source, std::vector<I_Codec::SPtr> codecs) const
{
	//build header with magic number, uncompressed size and codecs
	std::vector<uint8_t> result(8 + 1 + codecs.size());
	uint32_t destIndex = 0;
	*((uint32_t *)&result[destIndex]) = MagicHeader;
	destIndex += 4;
	*((uint32_t *)&result[destIndex]) = static_cast<uint32_t>(source.size());
	destIndex += 4;
	//write codec count to data
	result[destIndex++] = static_cast<uint8_t>(codecs.size());
	//write codec identifiers to data
	for (const auto & codec : codecs)
	{
		result[destIndex++] = codec->codecIdentifier();
	}
	//apply all encodings
	std::vector<uint8_t> compressed = source;
	for (const auto & codec : codecs)
	{
		if (m_verbose) { std::cout << codec->codecName() << " input data checksum is 0x" << std::hex << Tools::calculateAdler32(compressed) << std::dec << std::endl; }
		codec->setVerboseOutput(m_verbose);
		compressed = codec->encode(compressed);
	}
	//combine compressed data and header to result
	result.resize(result.size() + compressed.size());
	std::copy(compressed.cbegin(), compressed.cend(), std::next(result.begin(), destIndex));
	return result;
}

std::vector<uint8_t> Compressor::decompress(const std::vector<uint8_t> source) const
{
	//check minimum size
	if (source.size() > 8)
	{
		//ok. check for magic header
		if (source[3] == 'C' && source[2] == 'M' && source[1] == 'P')
		{
			//ok. check version
			if (source[0] == '5')
			{
				//ok. read uncompressed size from data
				const uint32_t uncompressedSize = *((uint32_t *)&source[4]);
				if (uncompressedSize > 0)
				{
					//ok. get codec count from header
					const uint8_t nrOfCodecs = source[8];
					if (nrOfCodecs > 0)
					{
						//source is encoded. get codecs from header
						std::vector<uint8_t> codecs(nrOfCodecs);
						for (int i = 0; i < nrOfCodecs; ++i)
						{
							codecs[i] = source[9 + i];
						}
						std::reverse(codecs.begin(), codecs.end());
						//copy compressed data for processing
						std::vector<uint8_t> result(source.size() - 8 - 1 - nrOfCodecs);
						std::copy(std::next(source.cbegin(), 8 + 1 + nrOfCodecs), source.cend(), result.begin());
						//apply codecs
						for (int i = 0; i < nrOfCodecs; ++i)
						{
							//try to find codec in list
							if (m_codecs.find(codecs[i]) != m_codecs.cend())
							{
								I_Codec::SPtr codec(m_codecs.at(codecs[i])());
								codec->setVerboseOutput(m_verbose);
								result = codec->decode(result);
								if (m_verbose) { std::cout << codec->codecName() << " output data checksum is 0x" << std::hex << Tools::calculateAdler32(result) << std::dec << std::endl; }
							}
							else
							{
								std::cout << "Unknown codec #" << codecs[i] << "!" << std::endl;
								std::cout << "Decompression failed!" << std::endl;
								return std::vector<uint8_t>();
							}
						}
						//check if data size matches
						if (result.size() == uncompressedSize)
						{
							std::cout << "Decompression succeeded." << std::endl;
							return result;
						}
						else
						{
							std::cout << "Uncompressed data size does not match!" << std::endl;
						}
					}
					else
					{
						//copy compressed data for processing
						std::vector<uint8_t> result(source.size() - 8 - 1);
						std::copy(std::next(source.cbegin(), 8 + 1), source.cend(), result.begin());
						return result;
					}
				}
				else
				{
					std::cout << "Invalid uncompressed size of 0!" << std::endl;
				}
			}
			else
			{
				std::cout << "Unknown version number found!" << std::endl;
			}
		}
		else
		{
			std::cout << "Bad header found!" << std::endl;
		}
	}
	else
	{
		std::cout << "Source data size too small!" << std::endl;
	}
	std::cout << "Decompression failed!" << std::endl;
	return std::vector<uint8_t>();
}