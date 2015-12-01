#pragma once

#include "codec.h"
#include "huffman_codes.h"

#include <inttypes.h>
#include <vector>
#include <array>

/// @brief Static Huffman compressor.
// Compressed data layout:
// 00h                     | uint8_t  | Huffman code length for symbol 0 + 1 (nibble each).
// 01h                     | uint8_t  | Huffman code length for symbol 2 + 3 (nibble each).
// ... (256 code lengths) ...
// 80h                     | uint32_t | Size of uncompressed data.
// 84h                     | bits     | Compressed data.
class StaticHuffman : public I_Codec
{
public:
	/// @brief Codec identifier. Do not change this and make sure there are no duplicate identifiers!
	static const uint8_t CodecIdentifier;

	/// @brief Create a new codec instance.
	/// @return Return codec instance.
	static I_Codec * Create();

	/// @brief Codec identifier.
	/// @return Codec identifier.
	virtual uint8_t codecIdentifier() const override;

	/// @brief Codec (human-readable) name.
	/// @return Codec name.
	virtual std::string codecName() const override;

	/// @brief Set the decompression method used. Available functions:
	/// Method | Time (ms) | RAM needed (bytes)
	///--------+-----------+--------------------
	///      0 | 11.35     | ~1060
	///      1 |  3.33     | ~1080
	///      2 |  2.48     | ~1060
	///      3 |  2.38     | ~ 540
	/// (tested in x64 release mode with images/1.raw on an i7-4810MQ)
	/// @param method Method index.
	void setDecodeMethod(uint32_t method = 3);

	/// @brief Compress source data.
	/// @param source Source data.
	/// @return Returns the compressed data, including header data and Huffman code length table.
	/// If the output did not end on a full byte, zero bits are appended to the output to ensure this.
	virtual std::vector<uint8_t> encode(const std::vector<uint8_t> & source) const override;

	/// @brief Decompress source data. Will use the method currently set via setDecodeMethod().
	/// @param source Source data.
	/// @return Returns decompressed data.
	virtual std::vector<uint8_t> decode(const std::vector<uint8_t> & source) const override;

private:
	/// @brief Array of frequencies of symbols.
	typedef std::array<uint32_t, 256> Frequencies;

	/// @brief Array of Huffman code lengths;
	typedef std::array<uint8_t, 256> CodeLengths;

	/// @brief Gets frequencies from the data an normalizes them so that the lowest valid frequency is close to 1.
	/// This is necessary to prevent the degeneration of the Huffman tree (see: http://www.arturocampos.com/cp_ch3-4.html)
	/// It might have a minor negative impact on compression.
	/// @param source Source data.
	/// @return Returns frequencies in source data.
	Frequencies frequenciesFromData(const std::vector<uint8_t> & source) const;

	/// @brief Build Huffman tree from frequencies and build canonical codes from that.
	/// @param Source data frequencies.
	/// @param allowedCodeLength Optional. Maximum length of Huffman codes allowed. This is restricted to 15 here, 
	/// because decompression can not handle longer codes atm.
	/// @return Canonical Huffman codes for symbols.
	HuffmanCodes codesFromFrequencies(Frequencies frequencies, uint8_t allowedCodeLength = 15) const;

	//------------------------------------------------------------------------------------------------

	/// @brief Return the Huffman code lengths read from compressed source data and the min/max code length.
	/// @param source Source data.
	/// @param index Index into source data. Will be increased when code lengths are read.
	/// @return Returns reconstructed Huffman code lengths or invalid/zero code lengths.
	CodeLengths StaticHuffman::getCodeLengthsFromHeader(const std::vector<uint8_t> & source, uint32_t & index, uint8_t & minLength, uint8_t & maxLength) const;

	/// @brief Return the canonical Huffman codes reconstructed from compressed source data.
	/// @param source Source data.
	/// @return Returns reconstructed Huffman codes or invalid/empty codes if failed.
	HuffmanCodes getCodesFromHeader(const std::vector<uint8_t> & source) const;

	//------------------------------------------------------------------------------------------------

	/// @brief Decompress source data.
	/// @param source Source data.
	/// @return Returns decompressed data.
	/// @note This is the semi-default algorithm using a table and linear code word search.
	std::vector<uint8_t> decode0(const std::vector<uint8_t> & source) const;

	/// @brief Decompress source data.
	/// @param source Source data.
	/// @return Returns decompressed data.
	/// @note Similar to and slightly faster than decompress1(). Uses the index directly, skipping some checks.
	std::vector<uint8_t> decode1(const std::vector<uint8_t> & source) const;

	/// @brief Decompress source data.
	/// @param source Source data.
	/// @return Returns decompressed data.
	/// @note Similar to decompress2(), but uses the code word buffer the other way 'round and uses a precalculated index table.
	std::vector<uint8_t> decode2(const std::vector<uint8_t> & source) const;

	/// @brief Decompress source data.
	/// @param source Source data.
	/// @return Returns decompressed data.
	/// @note Uses the minimum amount of memory to store only code length counts and symbol table.
	std::vector<uint8_t> decode3(const std::vector<uint8_t> & source) const;

	/// @brief The decompression method to use.
	uint32_t m_decodeMethod = 3;
};
