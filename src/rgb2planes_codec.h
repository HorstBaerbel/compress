#pragma once

#include "codec.h"
#include <inttypes.h>


class RgbToPlanes : public I_Codec
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

	/// @brief Converts RGBRGBRGB... data to RRR...GGG...BBB... data format for better compression.
	/// @param source Source data. Must be power of 3, else a 1to1 copy is made instead.
	/// @return Returns compressed data.
	virtual std::vector<uint8_t> encode(const std::vector<uint8_t> & source) const override;

	/// @brief Converts RRR...GGG...BBB... plane data to RGBRGBRGB... data format.
	/// @param source Source data. Must be power of 3, else a 1to1 copy is made instead.
	/// @return Returns decompressed data.
	virtual std::vector<uint8_t> decode(const std::vector<uint8_t> & source) const override;

};