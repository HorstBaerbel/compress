#pragma once

#include "codec.h"
#include <inttypes.h>


class Mtf1: public I_Codec
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

	/// @brief Apply move-to-front encoding on data. This is actually the MTF-1 variant of the algorithm.
	/// It moves a new symbol to the 0th entry only if it has already occurred directly before (is at index 1),
	/// otherwise it moves it to entry 1 first.
	/// @param source Source data.
	/// @return Returns compressed data.
	virtual std::vector<uint8_t> encode(const std::vector<uint8_t> & source) const override;

	/// @brief Apply reverse move-to-front encoding on data. This is actually the MTF-1 variant of the algorithm.
	/// @param source Source data.
	/// @return Returns decompressed data.
	virtual std::vector<uint8_t> decode(const std::vector<uint8_t> & source) const override;

};
