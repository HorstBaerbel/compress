#pragma once

#include "codec.h"

#include <inttypes.h>
#include <vector>
#include <map>


class Compressor
{
public:
	/// @brief Magic header "CMP5" == "CoMPre5sor" for compressed data. May be increased in future versions (ala "CMP6") for compatibility.
	static const uint32_t MagicHeader;

	/// @brief Toggle verbose output for operations.
	/// @param verbose Pass true to enable verbose output during compression.
	virtual void setVerboseOutput(bool verbose = false);

	/// @brief Compress source data and return result.
	/// @param source Source data.
	/// @param codecs List of pre-configured codecs to use for compression, in this particular order.
	/// @return Compressed data. Empty if compression failed.
	std::vector<uint8_t> compress(const std::vector<uint8_t> source, std::vector<I_Codec::SPtr> codecs) const;

	/// @brief Decompress source and return result.
	/// @param source Source data.
	/// @return Decompressed data. Empty if decompression failed.
	/// @note All necessary information for decompression will be extracted from source if it was
	/// compressed with compress() before. If not compression will fail.
	std::vector<uint8_t> decompress(const std::vector<uint8_t> source) const;

protected:
	/// @brief If true the routines output more information about the (de-)compression operation.
	bool m_verbose = false;

	/// @brief Map of available codecs sorted by their identifier.
	static const std::map<uint8_t, I_Codec::Creator> m_codecs;
};