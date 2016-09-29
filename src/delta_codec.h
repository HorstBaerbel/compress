#pragma once

#include "codec.h"
#include <inttypes.h>


class Delta : public I_Codec
{
public:
	/// @brief Codec identifier. Do not change this and make sure there are no duplicate identifiers!
	static const uint8_t CodecIdentifier;

	/// @brief Create a new codec instance.
	/// @return Return codec instance.
	static Delta * Create();

	/// @brief Codec identifier.
	/// @return Codec identifier.
	virtual uint8_t codecIdentifier() const override;

	/// @brief Codec (human-readable) name.
	/// @return Codec name.
	virtual std::string codecName() const override;

	/// @brief Encode source data to dest using delta- and zig-zag encoding.
	/// The code does (int8_t)((int16_t)a - (int16_t)b) ^ 256 to clamp value to the range [-128, 128].
	/// Then those values are then zig-zag encoded using (n << 1) ^ (n >> 15) so they map to [0, 255].
	/// The zig-zag encoding groups small absolute value, because it maps -1 to 1, 1 to 2 and so forth.
	/// The encoding can be reversed using (n >> 1) ^ (-(n & 1)).
	/// See: https://developers.google.com/protocol-buffers/docs/encoding#signed-integers
	/// @param source Source data.
	/// @return Encoded result.
	virtual std::vector<uint8_t> encode(const std::vector<uint8_t> & source) const override;

	/// @brief Decode delta- and zig-zag-encoded source data to dest.
	/// @param source Source data.
	/// @return Decoded result.
	virtual std::vector<uint8_t> decode(const std::vector<uint8_t> & source) const override;

};
