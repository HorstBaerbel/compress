#pragma once

#include "codec.h"
#include <inttypes.h>


class Bwt : public I_Codec
{
public:
	/// @brief Codec identifier. Do not change this and make sure there are no duplicate identifiers!
	static const uint8_t CodecIdentifier;

	/// @brief Typedef for sharing a codec. Used in Compressor::compress().
	typedef std::shared_ptr<Bwt> SPtr;

	/// @brief Create a new codec instance.
	/// @return Return codec instance.
	static Bwt * Create();

	/// @brief Codec identifier.
	/// @return Codec identifier.
	virtual uint8_t codecIdentifier() const override;

	/// @brief Codec (human-readable) name.
	/// @return Codec name.
	virtual std::string codecName() const override;

	/// @brief Set the block size used for compression.
	/// @param blockSize Block size for compression. The allowed maximum is 16MB - 1, due to algorithm restrictions.
	void setBlockSizeForCompression(const uint32_t blockSize = 256*1024);

	/// @brief Apply Burrows-Wheeler transform to data.
	/// @param source Source data.
	/// @return Data transformed with Burrows-Wheeler transform. The algorithm writes the block size to the result as a
	/// 4-byte value and adds 4 bytes for every block encoded to indicate the start of the initial string in the sequence.
	/// @note The algorithm will allocate 6 * blockSize bytes of memory for compression!
	virtual std::vector<uint8_t> encode(const std::vector<uint8_t> & source) const override;

	/// @brief Apply reverse Burrows-Wheeler transform to data. Will read the block size from the source data.
	/// @param source Source data.
	/// @return Data transformed with inverse Burrows-Wheeler transform.
	/// @note The algorithm will allocate 4 * blockSize + 2048 bytes of memory for decompression!
	virtual std::vector<uint8_t> decode(const std::vector<uint8_t> & source) const override;

private:
	uint32_t m_blockSize = 256 * 1024;

};