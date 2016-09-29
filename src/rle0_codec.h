#pragma once

#include "codec.h"
#include <inttypes.h>


class Rle0 : public I_Codec
{
public:
	/// @brief Codec identifier. Do not change this and make sure there are no duplicate identifiers!
	static const uint8_t CodecIdentifier;

	/// @brief Create a new codec instance.
	/// @return Return codec instance.
	static Rle0 * Create();

	/// @brief Codec identifier.
	/// @return Codec identifier.
	virtual uint8_t codecIdentifier() const override;

	/// @brief Codec (human-readable) name.
	/// @return Codec name.
	virtual std::string codecName() const override;

	/// @brief Apply zero run-length encoding to data. This is note the proposed Wheeler variant, but the naive one.
	/// A run of zeros is encoded as a zero and the repetition count minus 1. Runs are no longer than 255 bytes.
	/// Other symbols are simply copied to output. Thus a single zero is encoded as 0,0. A run of 2 zeros as 0,1 and so on.
	/// @param source Source data.
	/// @return Returns compressed data.
	virtual std::vector<uint8_t> encode(const std::vector<uint8_t> & source) const override;

	/// @brief Apply reverse zero run-length encoding to data. This is NOT the Wheeler variant, but the naive one.
	/// @param source Source data.
	/// @return Returns decompressed data.
	virtual std::vector<uint8_t> decode(const std::vector<uint8_t> & source) const override;

};