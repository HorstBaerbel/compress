#include "compressor.h"

#include "huffman_codec.h"
#include "delta_codec.h"
#include "bwt_codec.h"
#include "lzss_codec.h"
#include "mtf1_codec.h"
#include "rgb2planes_codec.h"
#include "rle0_codec.h"
#include "tools.h"

#include <cstdlib>
#include <iostream>
#include <chrono>
#include <string>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <regex>

enum CompressMode { None, Compress, Decompress, Test };
CompressMode m_mode = None;
bool m_beVerbose = false;
bool m_doBenchmark = false;
bool m_doTest = false;
bool m_useDirectories = false;
std::tr2::sys::path m_inputPath; //input file name.
std::tr2::sys::path m_outputPath; //output file name.
std::ofstream m_badOfStream; //we need this later as a default parameter...
std::vector<I_Codec::SPtr> m_codecs; //list of codes to use for compression

//-------------------------------------------------------------------------------------------------

void replaceAll(std::string & str, const std::string & from, const std::string & to)
{
	if (from.empty())
	{
		return;
	}
	size_t startPos = 0;
	while ((startPos = str.find(from, startPos)) != std::string::npos)
	{
		str.replace(startPos, from.length(), to);
		startPos += to.length();
	}
}

//-------------------------------------------------------------------------------------------------

std::vector<uint8_t> generateRandomData()
{
	const size_t dataSize = 256 * 1024;
	//allocate data
	std::vector<uint8_t> result(dataSize);
	//generate random data
	std::srand(3571);
	for (uint32_t i = 0; i < dataSize; ++i)
	{
		result[i] = (std::rand() * 255) / RAND_MAX;
	}
	if (m_beVerbose) std::cout << "Generated " << dataSize << " bytes of test data." << std::endl;
	return result;
}

std::vector<uint8_t> readFileContent(const std::tr2::sys::path & fileName)
{
	//check if the file name is "random"
	if (fileName.string() == "random")
	{
		return generateRandomData();
	}
	std::cout << "Opening \"" << fileName << "\"" << std::endl;
	//check if the file exists
	if (std::tr2::sys::exists(fileName))
	{
		//check if it isn't empty or too big for us
		const auto fileSize = std::tr2::sys::file_size(fileName);
		if (fileSize == 0)
		{
			std::cout << "File is empty!" << std::endl;
			return std::vector<uint8_t>();
		}
		else if (fileSize < INT_MAX)
		{
			//try opening file
			std::ifstream in(fileName, std::ios_base::in | std::ios_base::binary);
			if (in)
			{
				//allocate data
				std::vector<uint8_t> result(fileSize);
				//try reading from file
				in.read((char*)result.data(), fileSize);
				if (in.gcount() == fileSize)
				{
					if (m_beVerbose) std::cout << "Read " << fileSize << " bytes from file." << std::endl;
					in.close();
					return result;
				}
				else
				{
					std::cout << "Error reading from file!" << std::endl;
				}
				in.close();
			}
			else
			{
				std::cout << "Failed to open " << fileName << " for reading!" << std::endl;
			}
		}
		else
		{
			std::cout << "File " << fileName << " is too big!" << std::endl;
		}
	}
	else
	{
		std::cout << "File " << fileName << " does not exist!" << std::endl;
	}
	return std::vector<uint8_t>();
}

bool writeFileContent(const std::tr2::sys::path & fileName, const std::vector<uint8_t> & data)
{
	//try opening file
	std::ofstream out(fileName, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
	if (out)
	{
		if (m_beVerbose) std::cout << "File " << fileName << " opened." << std::endl;
		//try writing to file
		out.write((char*)data.data(), data.size());
		if (out.good())
		{
			if (m_beVerbose) std::cout << "Data written to file." << std::endl;
			out.close();
			return true;
		}
		else
		{
			std::cout << "Error writing to file!" << std::endl;
		}
		out.close();
	}
	else
	{
		std::cout << "Failed to open " << fileName << " for writing!" << std::endl;
	}
	return false;
}

//-------------------------------------------------------------------------------------------------

int compress(const std::tr2::sys::path & input, const std::tr2::sys::path & output)
{
	//read file data
	std::vector<uint8_t> source = readFileContent(input);
	if (source.size() > 0)
	{
		//try to compress input data
		Compressor comp;
		comp.setVerboseOutput(m_beVerbose);
		if (m_beVerbose) std::cout << "Compressing..." << std::endl;
		std::vector<uint8_t> result = comp.compress(source, m_codecs);
		if (result.size() > 0)
		{
			//worked. write to file
			std::cout << "Data compressed to " << result.size() << " bytes (including header)." << std::endl;
			std::cout << "Compression ratio is " << 100.0f - (float)result.size() / (float)source.size() * 100.0f << "% (" << (float)result.size() * 8 / (float)source.size() << " bpc)." << std::endl;
			return writeFileContent(output, result) ? 0 : -3;
		}
		else
		{
			std::cout << "Compression failed!" << std::endl;
		}
		return -2;
	}
	else
	{
		std::cout << "No source data. Skipping " << input << "!" << std::endl;
	}
	return -1;
}

int decompress(const std::tr2::sys::path & input, const std::tr2::sys::path & output)
{
	//read file data
	std::vector<uint8_t> source = readFileContent(input);
	if (source.size() > 0)
	{
		//try to decompress input data
		Compressor comp;
		comp.setVerboseOutput(m_beVerbose);
		if (m_beVerbose) std::cout << "Decompressing..." << std::endl;
		std::vector<uint8_t> result = comp.decompress(source);
		if (result.size() > 0)
		{
			//worked. write to file
			std::cout << "Data decompressed to " << result.size() << " bytes (including header)." << std::endl;
			return writeFileContent(output, result) ? 0 : -3;
		}
		else
		{
			std::cout << "Decompression failed!" << std::endl;
		}
		return -2;
	}
	else
	{
		std::cout << "No source data. Skipping " << input << "!" << std::endl;
	}
	return -1;
}

int test(const std::tr2::sys::path & input)
{
	//read file data
	std::vector<uint8_t> source = readFileContent(input);
	if (source.size() > 0)
	{
		//try to compress input data
		Compressor comp;
		comp.setVerboseOutput(m_beVerbose);
		if (m_beVerbose) std::cout << "Compressing..." << std::endl;
		//record start time
		const uint32_t testCount = m_doBenchmark ? 10 : 1;
		auto startTime = std::chrono::steady_clock::now();
		//do compression
		std::vector<uint8_t> compressedData;
		for (uint32_t i = 0; i < testCount; ++i)
		{
			compressedData = comp.compress(source, m_codecs);
		}
		//print compression information
		std::cout << "Data compressed to " << compressedData.size() << " bytes (including header)." << std::endl;
		std::cout << "Compression ratio is " << 100.0f - (float)compressedData.size() / (float)source.size() * 100.0f << "% (" << (float)compressedData.size() * 8 / (float)source.size() << " bpc)." << std::endl;
		//print timing information
		if (m_doBenchmark)
		{
			auto endTime = std::chrono::steady_clock::now();
			float millis = (float)std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
			std::cout << "Compression took " << millis / (float)testCount << "ms." << std::endl;
		}
		//check if compression worked
		if (compressedData.size() > 0)
		{
			if (m_beVerbose) std::cout << "Decompressing..." << std::endl;
			//record new start time
			startTime = std::chrono::steady_clock::now();
			//worked. try to decompress again
			std::vector<uint8_t> decompressedData;
			for (uint32_t i = 0; i < testCount; ++i)
			{
				decompressedData = comp.decompress(compressedData);
			}
			//print timing information
			if (m_doBenchmark)
			{
				auto endTime = std::chrono::steady_clock::now();
				float millis = (float)std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
				std::cout << "Decompression took " << millis / (float)testCount << "ms." << std::endl;
			}
			//compare size and binary data
			if (source.size() == decompressedData.size())
			{
				if (std::equal(source.cbegin(), source.cend(), decompressedData.cbegin()))
				{
					if (m_beVerbose) std::cout << "Compress/Decompress run worked." << std::endl;
					return 0;
				}
				else
				{
					const auto mismatchIt = std::mismatch(source.cbegin(), source.cend(), decompressedData.cbegin());
					std::cout << "Decompressed data does not match input data at byte " << std::distance(decompressedData.cbegin(), mismatchIt.second) << "!" << std::endl;
				}
			}
			else
			{
				std::cout << "Decompressed data size does not match!" << std::endl;
			}
			return -3;
		}
		else
		{
			std::cout << "Compression failed!" << std::endl;
		}
		return -2;
	}
	else
	{
		std::cout << "No source data. Skipping " << input << "!" << std::endl;
	}
	return -1;
}

int run()
{
	//first check wether we have a directory, regular file, or wildcards
	if (std::tr2::sys::is_directory(m_inputPath))
	{
		//check if input is sufficient
		if (m_mode == CompressMode::Compress && !std::tr2::sys::is_directory(m_outputPath))
		{
			std::cout << "Error: Invalid output directory for compression!" << std::endl;
			return -2;
		}
		else if (m_mode == CompressMode::Decompress && !std::tr2::sys::is_directory(m_outputPath))
		{
			std::cout << "Error: Invalid output directory for decompression!" << std::endl;
			return -2;
		}
		//loop through directory
		std::tr2::sys::directory_iterator endIt;
		std::tr2::sys::directory_iterator dirIt(m_inputPath);
		for (; dirIt != endIt; ++dirIt)
		{
			//get next file and check if it is a regular file
			int result = -2;
			auto & inFilePath = dirIt->path();
			if (is_regular_file(inFilePath))
			{
				//build output file name from output directory and input file name
				std::tr2::sys::path outFilePath = m_outputPath;
				outFilePath /= inFilePath.filename();
				if (m_mode == CompressMode::Compress)
				{
					result = compress(inFilePath, outFilePath);
				}
				else if (m_mode == CompressMode::Decompress)
				{
					result = decompress(inFilePath, outFilePath);
				}
				else if (m_mode == CompressMode::Test)
				{
					result = test(inFilePath);
				}
			}
			//check if operation worked
			if (result != 0)
			{
				return result;
			}
		}
		//when we get here, everything was fine for all files
		return 0;
	}
	else if (std::tr2::sys::is_regular_file(m_inputPath))
	{
		//check if input is sufficient
		if (m_mode == CompressMode::Compress && (std::tr2::sys::exists(m_outputPath) && !std::tr2::sys::is_regular_file(m_outputPath)))
		{
			std::cout << "Error: Invalid output file for compression!" << std::endl;
			return -2;
		}
		else if (m_mode == CompressMode::Decompress && (std::tr2::sys::exists(m_outputPath) && !std::tr2::sys::is_regular_file(m_outputPath)))
		{
			std::cout << "Error: Invalid output file for decompression!" << std::endl;
			return -2;
		}
		//operate on files
		if (m_mode == CompressMode::Compress)
		{
			return compress(m_inputPath, m_outputPath);
		}
		else if (m_mode == CompressMode::Decompress)
		{
			return decompress(m_inputPath, m_outputPath);
		}
		else if (m_mode == CompressMode::Test)
		{
			return test(m_inputPath);
		}
	}
	else {
		//check if we're using wildcards
		std::string fileName = m_inputPath.filename();
		if (std::find(fileName.cbegin(), fileName.cend(), '*') != fileName.cend())
		{
			//seems so. check if input is sufficient
			if (m_mode == CompressMode::Compress && !std::tr2::sys::is_directory(m_outputPath))
			{
				std::cout << "Error: Invalid output directory for compression!" << std::endl;
				return -2;
			}
			else if (m_mode == CompressMode::Decompress && !std::tr2::sys::is_directory(m_outputPath))
			{
				std::cout << "Error: Invalid output directory for decompression!" << std::endl;
				return -2;
			}
			//build regular expression from wildcards
			replaceAll(fileName, ".", "\\.");
			replaceAll(fileName, "*", ".*");
			std::regex fileRegEx(fileName);
			//get directory from input path
			auto directoryPath = m_inputPath.remove_filename();
			//loop through directory
			std::tr2::sys::directory_iterator endIt;
			std::tr2::sys::directory_iterator dirIt(directoryPath);
			for (; dirIt != endIt; ++dirIt)
			{
				//get next file, check if it is a regular file and matches our wildcards
				int result = 0;
				auto & inFilePath = dirIt->path();
				std::smatch dummyMatch;
				if (is_regular_file(inFilePath) && std::regex_match(inFilePath.filename(), dummyMatch, fileRegEx))
				{
					//build output file name from output directory and input file name
					std::tr2::sys::path outFilePath = m_outputPath;
					outFilePath /= inFilePath.filename();
					if (m_mode == CompressMode::Compress)
					{
						result = compress(inFilePath, outFilePath);
					}
					else if (m_mode == CompressMode::Decompress)
					{
						result = decompress(inFilePath, outFilePath);
					}
					else if (m_mode == CompressMode::Test)
					{
						result = test(inFilePath);
					}
				}
				//check if operation worked
				if (result != 0)
				{
					return result;
				}
			}
			//when we get here, everything was fine for all files
			return 0;
		}
		else
		{
			std::cout << "File or directory \"" << m_inputPath << "\" not found!" << std::endl;
		}
	}
	return -2;
}

//-----------------------------------------------------------------------------

void printVersion()
{
	std::cout << "CoMPres5 v0.6" << std::endl << std::endl;
}

void printUsage()
{
	std::cout << std::endl;
	std::cout << "Usage: cmp5 [-c, -d, -t] [options] <infile> [outfile]" << std::endl;
	std::cout << "Available options (you must specify -c, -d or -t):" << std::endl;
	std::cout << "-c Compress data from <infile> to <outfile>." << std::endl;
	std::cout << "-d Decompress data from <infile> to <outfile>." << std::endl;
	std::cout << "-t Test routines by compressing/decompressing data from <infile> in memory." << std::endl;
	std::cout << "-b Benchmark compression and decompression." << std::endl;
	std::cout << "-v Be verbose." << std::endl;
	std::cout << "Use \"random\" for <infile> to generate random input data." << std::endl;
	std::cout << "Available pre-processing options (optional):" << std::endl;
	std::cout << "-rgbSplit Split R8G8B8 data into color planes (size must be divisible by 3)." << std::endl;
	std::cout << "-delta Apply delta-encoding." << std::endl;
	std::cout << "-bwt[block size] Apply Burrows-Wheeler transform. Block size is optional," << std::endl;
	std::cout << "                 e.g. \"-bwt1024\" (Default is 65535, max. is 16MB - 1Byte)." << std::endl;
	std::cout << "-mtf1 Apply move-to-front-1 encoding." << std::endl;
	std::cout << "-rle0 Apply zero run-length encoding." << std::endl;
	std::cout << "Available entropy coders (optional):" << std::endl;
	std::cout << "-huffman Use static Huffman entropy coder." << std::endl;
	//std::cout << "-ahuffman Use adaptive Huffman entropy coder." << std::endl;
	std::cout << "-lzss[dict size] Use LZSS entropy coder. Dictionary size is optional." << std::endl;
	std::cout << "                 e.g. \"-lzss1024\" (Default is 4096, must be a power of 2)." << std::endl;
	std::cout << "Examples:" << std::endl;
	std::cout << "cmp5 -c -huffman ./canterbury/alice29.txt ./alice29.cmp5 (compress file)" << std::endl;
	std::cout << "cmp5 -d ./alice29.cmp5 ./canterbury/alice29_2.txt (decompress file)" << std::endl;
	std::cout << "cmp5 -t -bwt -huffman ./canterbury/alice29.txt (test routines)" << std::endl;
	std::cout << "cmp5 -c -huffman ./canterbury ./compressed (compress files in directory)" << std::endl;
	std::cout << "cmp5 -c -huffman ./test/*.txt (compress files matching wildcards)" << std::endl;
	std::cout << "cmp5 -c -v -lzss random (compress random generated data and be verbose)" << std::endl;
}
bool readArguments(int argc, const char * argv[])
{
	bool pastOptions = false;
	bool pastInput = false;
	bool pastOutput = false;
	for (int i = 1; i < argc; ++i)
	{
		//read argument from list
		std::string argument = argv[i];
		if (!pastOptions)
		{
			//check which option it is
			if (argument == "-b") { m_doBenchmark = true; continue; }
			else if (argument == "-c") { m_mode = CompressMode::Compress; continue; }
			else if (argument == "-d") { m_mode = CompressMode::Decompress; continue; }
			else if (argument == "-t") { m_mode = CompressMode::Test; continue; }
			else if (argument == "-v") { m_beVerbose = true; continue; }
			else if (argument == "-rgbSplit")
			{
				if (m_mode == CompressMode::Compress || m_mode == CompressMode::Test)
				{
					m_codecs.push_back(I_Codec::SPtr(RgbToPlanes::Create()));
				}
				else
				{
					std::cout << "Not compressing. Ignoring \"" << argument << "\"." << std::endl;
				}
				continue;
			}
			else if (argument == "-delta")
			{
				if (m_mode == CompressMode::Compress || m_mode == CompressMode::Test)
				{
					m_codecs.push_back(I_Codec::SPtr(Delta::Create()));
				}
				else
				{
					std::cout << "Not compressing. Ignoring \"" << argument << "\"." << std::endl;
				}
				continue;
			}
			else if (argument == "-mtf1")
			{
				if (m_mode == CompressMode::Compress || m_mode == CompressMode::Test)
				{
					m_codecs.push_back(I_Codec::SPtr(Mtf1::Create()));
				}
				else
				{
					std::cout << "Not compressing. Ignoring \"" << argument << "\"." << std::endl;
				}
				continue;
			}
			else if (argument == "-rle0")
			{
				if (m_mode == CompressMode::Compress || m_mode == CompressMode::Test)
				{
					m_codecs.push_back(I_Codec::SPtr(Rle0::Create()));
				}
				else
				{
					std::cout << "Not compressing. Ignoring \"" << argument << "\"." << std::endl;
				}
				continue;
			}
			else if (argument.find("-bwt") == 0)
			{
				if (m_mode == CompressMode::Compress || m_mode == CompressMode::Test)
				{
					Bwt::SPtr bwtCodec(Bwt::Create());
					//check if the user has passed a block size
					const std::string blockString = argument.substr(4);
					if (!blockString.empty())
					{
						//check if the string can be converted to a number
						const uint32_t blockSize = std::stoul(blockString);
						if (blockSize > 0 && blockSize < 16 * 1024 * 1024 - 1)
						{
							bwtCodec->setCompressionParameters(blockSize);
						}
						else
						{
							std::cout << "Error: Bad block size value \"" << blockString << "\"! Ignoring." << std::endl;
						}
					}
					m_codecs.push_back(bwtCodec);
				}
				else
				{
					std::cout << "Not compressing. Ignoring \"" << argument << "\"." << std::endl;
				}
				continue;
			}
			else if (argument == "-huffman")
			{
				if (m_mode == CompressMode::Compress || m_mode == CompressMode::Test)
				{
					m_codecs.push_back(I_Codec::SPtr(StaticHuffman::Create()));
				}
				else
				{
					std::cout << "Not compressing. Ignoring \"" << argument << "\"." << std::endl;
				}
				continue;
			}
			else if (argument.find("-lzss") == 0)
			{
				if (m_mode == CompressMode::Compress || m_mode == CompressMode::Test)
				{
					LZSS::SPtr lzssCodec(LZSS::Create());
					//check if the user has passed a block size
					const std::string blockString = argument.substr(5);
					if (!blockString.empty())
					{
						//check if the string can be converted to a number
						const uint32_t dictSize = std::stoul(blockString);
						const uint32_t dictBits = Tools::log2(dictSize);
						if (dictBits >= 4 && dictBits <= 16)
						{
							lzssCodec->setCompressionParameters(dictBits, dictBits / 4);
						}
						else
						{
							std::cout << "Error: Bad dictionary size value \"" << blockString << "\"! Ignoring." << std::endl;
						}
					}
					m_codecs.push_back(lzssCodec);
				}
				else
				{
					std::cout << "Not compressing. Ignoring \"" << argument << "\"." << std::endl;
				}
				continue;
			}
			//none of the options was matched until here so we must be past the options
			pastOptions = true;
		}
		//check if this is an input or output file
		if (pastOptions && !pastInput)
		{
			//do not accept random data for decompression
			if (m_mode == CompressMode::Decompress || argument == "random")
			{
				std::cout << "Can not use \"random\" input data for decompression!" << std::endl;
				return false;
			}
			m_inputPath = argument;
			pastInput = true;
			continue;
		}
		else if (pastOptions && pastInput && !pastOutput)
		{
			if (m_mode == CompressMode::Compress || m_mode == CompressMode::Decompress)
			{
				//do not accept random data as output
				if (argument == "random")
				{
					std::cout << "Can not use \"random\" for <outfile>!" << std::endl;
					return false;
				}
				//check if the argument is a directory
				m_outputPath = argument;
				if ((std::tr2::sys::is_directory(m_inputPath) && std::tr2::sys::is_directory(m_outputPath)) ||
					 (std::tr2::sys::is_regular_file(m_inputPath) && std::tr2::sys::is_regular_file(m_outputPath)) ||
					 (std::tr2::sys::is_regular_file(m_inputPath) && !std::tr2::sys::exists(m_outputPath)))
				{
					pastOutput = true;
				}
				else
				{
					std::cout << "<infile> and <outfile> must both be files or directories!" << std::endl;
					return false;
				}
			}
			else
			{
				std::cout << "Not compressing. Ignoring \"" << argument << "\"." << std::endl;
			}
			continue;
		}
		else
		{
			std::cout << "Error: Unknown argument \"" << argument << "\"!" << std::endl;
			return false;
		}
	}
	//if we compress or decompress we need a second argument
	if (m_mode == CompressMode::Compress || m_mode == CompressMode::Decompress)
	{
		if (!(std::tr2::sys::is_directory(m_inputPath) && std::tr2::sys::is_directory(m_outputPath)) &&
			 !(std::tr2::sys::is_regular_file(m_inputPath) && std::tr2::sys::is_regular_file(m_outputPath)) &&
			 !(std::tr2::sys::is_regular_file(m_inputPath) && !std::tr2::sys::exists(m_outputPath)))
		{
			std::cout << "Error: Compression and decompression need an <outfile>!" << std::endl;
			return false;
		}
	}
	return true;
}

int main(int argc, const char * argv[])
{
	printVersion();
	//check number of arguments and if all arguments can be read
	if (argc < 2 || !readArguments(argc, argv))
	{
		printUsage();
		return -1;
	}
	return run();
}