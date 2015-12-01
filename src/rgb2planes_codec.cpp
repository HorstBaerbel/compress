#include "rgb2planes_codec.h"

#include <cstring>


const uint8_t RgbToPlanes::CodecIdentifier = 10;

uint8_t RgbToPlanes::codecIdentifier() const
{
	return CodecIdentifier;
}

std::string RgbToPlanes::codecName() const
{
	return "RGB to planes";
}

I_Codec * RgbToPlanes::Create()
{
	return new RgbToPlanes();
}

std::vector<uint8_t> RgbToPlanes::encode(const std::vector<uint8_t> & source) const
{
	const uint32_t srcSize = static_cast<uint32_t>(source.size());
	if (srcSize > 0)
	{
		std::vector<uint8_t> dest(srcSize);
		uint32_t destIndex = 0;
		//the count must be divisible by three because we separate the RGB planes
		if (srcSize % 3 == 0)
		{
			const uint32_t planeSize = srcSize / 3;
			const uint32_t greenStart = planeSize;
			const uint32_t blueStart = 2 * planeSize;
			for (uint32_t srcIndex = 0; srcIndex < srcSize; ++destIndex, srcIndex += 3)
			{
				dest[destIndex] = source[srcIndex];
				dest[destIndex + greenStart] = source[srcIndex + 1];
				dest[destIndex + blueStart] = source[srcIndex + 2];
			}
			return dest;
		}
		else
		{
			return source;
		}
	}
	return std::vector<uint8_t>();
}

std::vector<uint8_t> RgbToPlanes::decode(const std::vector<uint8_t> & source) const
{
	const uint32_t srcSize = static_cast<uint32_t>(source.size());
	if (srcSize > 0)
	{
		std::vector<uint8_t> dest(srcSize);
		uint32_t srcIndex = 0;
		//the count must be divisible by three because we separate the RGB planes
		if (srcSize % 3 == 0)
		{
			const uint32_t planeSize = srcSize / 3;
			const uint32_t greenStart = planeSize;
			const uint32_t blueStart = 2 * planeSize;
			for (uint32_t destIndex = 0; destIndex < srcSize; ++srcIndex, destIndex += 3)
			{
				dest[destIndex] = source[srcIndex];
				dest[destIndex + 1] = source[srcIndex + greenStart];
				dest[destIndex + 2] = source[srcIndex + blueStart];
			}
			return dest;
		}
		else
		{
			return source;
		}
	}
	return std::vector<uint8_t>();
}