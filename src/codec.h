#pragma once

#include <inttypes.h>
#include <vector>
#include <memory>
#include <string>

/// @brief Abstract base class for preprocessors and compressors used in the application.
/// You will have to implement codecIdentifier(), encode() and decode().
class I_Codec
{
public:
	/// @brief Typedef for sharing a codec. Used in Compressor::compress().
	typedef std::shared_ptr<I_Codec> SPtr;

	/// @brief Typedef for a creator function for a codec.
	typedef I_Codec * (*Creator)();

	/// @brief Toggle verbose output for operations.
	/// @param verbose Pass true to enable verbose output during compression.
	virtual void setVerboseOutput(bool verbose = false);

	/// @brief Codec identifier. Make sure there are no duplicate identifiers in the software!
	/// @return Codec identifier.
	/// @note It makes sense to store a static value for this in the codec for registering the codec in factories etc.
	virtual uint8_t codecIdentifier() const = 0;

	/// @brief Codec (human-readable) name.
	/// @return Codec name.
	virtual std::string codecName() const = 0;
	
	/// @brief Apply compression algorithm to source.
	/// @param source Source data.
	/// @return Number of bytes written to dest.
	virtual std::vector<uint8_t> encode(const std::vector<uint8_t> & source) const = 0;
	
	/// @brief Apply decompression algorithm to source.
	/// @param source Source data.
	/// @return Number of bytes written to dest.
	virtual std::vector<uint8_t> decode(const std::vector<uint8_t> & source) const = 0;

protected:
	/// @brief If true the routines should output more information about the (de-)compression operation.
	bool m_verbose = false;
};
