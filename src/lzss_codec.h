#pragma once

#include "codec.h"
#include <inttypes.h>


class LZSS : public I_Codec
{
public:
	/// @brief Codec identifier. Do not change this and make sure there are no duplicate identifiers!
	static const uint8_t CodecIdentifier;

	/// @brief Typedef for sharing a codec. Used in Compressor::compress().
	typedef std::shared_ptr<LZSS> SPtr;

	/// @brief Create a new codec instance.
	/// @return Return codec instance.
	static LZSS * Create();

	/// @brief Codec identifier.
	/// @return Codec identifier.
	virtual uint8_t codecIdentifier() const override;

	/// @brief Codec (human-readable) name.
	/// @return Codec name.
	virtual std::string codecName() const override;

	/// @brief Set the dictionary size and maximum match length used for compression.
	/// @param dictionaryBits How many bits to allocate for the dictionary [4,20].
	/// @param matchLengthBits How many bits to allocate for length of the matched string [3,8].
	void setCompressionParameters(const uint32_t dictionaryBits = 12, const uint32_t matchLengthBits = 4);

	/// @brief Apply LZSS entropy encoding.
	/// @param source Source data.
	/// @return Returns compressed data.
	/// @note This implementation uses 9/17 bits for un-encoded/encoded bytes and a dictionary of 4096 bytes per default.
	/// The first bit is the un-encoded/encoded flag, then follows the data.
	/// Un-encoded bytes are appended verbatim, whereas
	/// encoded runs are split into a dictionary index of 12 bits and a run length of 4 bits.
	/// Runs under 2 bytes are not encoded, thus the run length goes from 3 to 18 (0-15 binary).
	/// The default values can be changed using setCompressionParameters() before encoding.
	virtual std::vector<uint8_t> encode(const std::vector<uint8_t> & source) const override;

	/// @brief Apply reverse LZSS entropy encoding.
	/// @param source Source data.
	/// @return Returns decompressed data.
	virtual std::vector<uint8_t> decode(const std::vector<uint8_t> & source) const override;

private:
	/// @brief Output a verbatim / un-encoded byte to dest.
	void outputVerbatim(uint8_t symbol, std::vector<uint8_t> & dest, uint32_t & destIndex, uint32_t & buffer, uint32_t & bufferBits) const;

	/// @brief Output an encoded match to dest.
	void outputMatch(int32_t index, int32_t length, std::vector<uint8_t> & dest, uint32_t & destIndex, uint32_t & buffer, uint32_t & bufferBits) const;

	/// @brief Number of bits used for dictionary in encoded data.
	int32_t m_dictionaryBits = 12;
	/// @brief Size of dictionary.
	int32_t m_dictionarySize = (1 << 12);
	/// @brief Last index in dictionary.
	int32_t m_dictionaryMaxIndex = (1 << 12) - 1;
	/// @brief Size of look-ahead-buffer.
	int32_t m_lookAheadSize = (1 << 9);
	/// @brief Number of bits used for the length of a match in encoded data.
	int32_t m_matchLengthBits = 4;
	/// @brief Minimum length of match in symbols.
	int32_t m_matchLengthMin = ((12 + 4 + 7) / 8) + 1;
	/// @brief Maximum length of match in symbols.
	int32_t m_matchLengthMax = ((1 << 4) - 1) + ((12 + 4 + 7) / 8) + 1;

};
