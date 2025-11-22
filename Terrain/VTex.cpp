#include "VTex.h"
#include "Texture.h"
#include <fstream>
#include <cmath>

struct MipLevel
{
	std::vector<char> data;
	int width;
	int height;
	int tilesX;
	int tilesY;
};

static MipLevel GenerateMipLevel(const char* srcData, int srcWidth, int srcHeight, int bytesPerPixel)
{
	MipLevel mip;
	mip.width = std::max(1, srcWidth / 2);
	mip.height = std::max(1, srcHeight / 2);
	mip.data.resize(mip.width * mip.height * bytesPerPixel);

	for (int y = 0; y < mip.height; y++)
	{
		for (int x = 0; x < mip.width; x++)
		{
			int srcX = x * 2;
			int srcY = y * 2;

			int r = 0, g = 0, b = 0, a = 0;
			int samples = 0;

			for (int dy = 0; dy < 2; dy++)
			{
				for (int dx = 0; dx < 2; dx++)
				{
					int sampleX = srcX + dx;
					int sampleY = srcY + dy;

					if (sampleX < srcWidth && sampleY < srcHeight)
					{
						int srcIndex = (sampleY * srcWidth + sampleX) * bytesPerPixel;
						r += (unsigned char)srcData[srcIndex + 0];
						g += (unsigned char)srcData[srcIndex + 1];
						b += (unsigned char)srcData[srcIndex + 2];
						a += (unsigned char)srcData[srcIndex + 3];
						samples++;
					}
				}
			}

			int dstIndex = (y * mip.width + x) * bytesPerPixel;
			mip.data[dstIndex + 0] = (char)(r / samples);
			mip.data[dstIndex + 1] = (char)(g / samples);
			mip.data[dstIndex + 2] = (char)(b / samples);
			mip.data[dstIndex + 3] = (char)(a / samples);
		}
	}

	return mip;
}

void VTex::ConvertToVTex(const std::string & path, int tileSize)
{
	std::cout << "Converting " << path << " to .vtex format with mip mapping..." << std::endl;

	Image sourceImage;
	sourceImage.LoadImageFromFile(path, true, STBI_rgb_alpha);

	if (!sourceImage.bytes)
	{
		std::cout << "Failed to load source image: " << path << std::endl;
		return;
	}

	std::cout << "Loaded image size: " << sourceImage.width << "x" << sourceImage.height << std::endl;

	const int bytesPerPixel = 4; // RGBA8
	const int tileDataSize = tileSize * tileSize * bytesPerPixel;

	int maxDim = std::max(sourceImage.width, sourceImage.height);
	int mipCount = 1 + (int)std::floor(std::log2(maxDim));
	std::cout << "Generating " << mipCount << " mip levels..." << std::endl;

	std::vector<MipLevel> mips(mipCount);

	mips[0].data.assign(sourceImage.bytes, sourceImage.bytes + sourceImage.width * sourceImage.height * bytesPerPixel); // Mip 0 is the source image
	mips[0].width = sourceImage.width;
	mips[0].height = sourceImage.height;
	mips[0].tilesX = (sourceImage.width + tileSize - 1) / tileSize;
	mips[0].tilesY = (sourceImage.height + tileSize - 1) / tileSize;

	for (int mip = 1; mip < mipCount; mip++)
	{
		mips[mip] = GenerateMipLevel(mips[mip - 1].data.data(), mips[mip - 1].width, mips[mip - 1].height, bytesPerPixel);
		mips[mip].tilesX = (mips[mip].width + tileSize - 1) / tileSize;
		mips[mip].tilesY = (mips[mip].height + tileSize - 1) / tileSize;
		std::cout << "  Mip " << mip << ": " << mips[mip].width << "x" << mips[mip].height
		          << " (" << mips[mip].tilesX << "x" << mips[mip].tilesY << " tiles)" << std::endl;
	}

	int totalTileCount = 0;
	for (int mip = 0; mip < mipCount; mip++)
	{
		totalTileCount += mips[mip].tilesX * mips[mip].tilesY;
	}
	std::cout << "Total tiles across all mips: " << totalTileCount << std::endl;

	VTexHeader header = {};
	header.magic[0] = 'V';
	header.magic[1] = 'T';
	header.magic[2] = 'E';
	header.magic[3] = 'X';
	header.version = 1;
	header.width = sourceImage.width;
	header.height = sourceImage.height;
	header.tileSize = tileSize;
	header.tilesX = mips[0].tilesX;
	header.tilesY = mips[0].tilesY;
	header.bytesPerPixel = bytesPerPixel;
	header.mipLevels = mipCount;

	std::vector<std::vector<VTexTileEntry>> tileTables(mipCount);
	for (int mip = 0; mip < mipCount; mip++)
	{
		int mipTileCount = mips[mip].tilesX * mips[mip].tilesY;
		tileTables[mip].resize(mipTileCount);
	}

	uint64_t currentOffset = sizeof(VTexHeader);
	for (int mip = 0; mip < mipCount; mip++)
	{
		currentOffset += sizeof(VTexTileEntry) * tileTables[mip].size();
	}

	for (int mip = 0; mip < mipCount; mip++)
	{
		for (size_t i = 0; i < tileTables[mip].size(); i++)
		{
			tileTables[mip][i].offset = currentOffset;
			tileTables[mip][i].compressedSize = tileDataSize;
			currentOffset += tileDataSize;
		}
	}

	std::string outputPath = path.substr(0, path.find_last_of('.')) + ".vtex";
	std::ofstream outFile(outputPath, std::ios::binary);
	if (!outFile.is_open())
	{
		std::cout << "Failed to create output file: " << outputPath << std::endl;
		return;
	}

	outFile.write(reinterpret_cast<const char*>(&header), sizeof(VTexHeader));

	for (int mip = 0; mip < mipCount; mip++)
	{
		outFile.write(reinterpret_cast<const char*>(tileTables[mip].data()),
		              sizeof(VTexTileEntry) * tileTables[mip].size());
	}

	std::vector<char> tileBuffer(tileDataSize);

	const bool DEBUG_MIP_COLORS = true;

	for (int mip = 0; mip < mipCount; mip++)
	{
		for (int tileY = 0; tileY < mips[mip].tilesY; tileY++)
		{
			for (int tileX = 0; tileX < mips[mip].tilesX; tileX++)
			{
				std::fill(tileBuffer.begin(), tileBuffer.end(), 0); // Clear tile buffer (for padding if tile extends beyond image bounds)

				for (int y = 0; y < tileSize; y++)
				{
					for (int x = 0; x < tileSize; x++)
					{
						int srcX = tileX * tileSize + x;
						int srcY = tileY * tileSize + y;

						if (srcX < mips[mip].width && srcY < mips[mip].height)
						{
							int dstIndex = (y * tileSize + x) * bytesPerPixel;

							if (DEBUG_MIP_COLORS) // TODO Demain remove this
							{
								// Mip 0: Original texture (not colored)
								// Mip 1: Red
								// Mip 2: Blue
								// Mip 3: Green
								// Mip 4+: White
								if (mip == 0)
								{
									int srcIndex = (srcY * mips[mip].width + srcX) * bytesPerPixel;
									tileBuffer[dstIndex + 0] = mips[mip].data[srcIndex + 0];
									tileBuffer[dstIndex + 1] = mips[mip].data[srcIndex + 1];
									tileBuffer[dstIndex + 2] = mips[mip].data[srcIndex + 2];
									tileBuffer[dstIndex + 3] = mips[mip].data[srcIndex + 3];
								}
								else if (mip == 1)
								{
									tileBuffer[dstIndex + 0] = (char)255;
									tileBuffer[dstIndex + 1] = (char)0;
									tileBuffer[dstIndex + 2] = (char)0;
									tileBuffer[dstIndex + 3] = (char)255;
								}
								else if (mip == 2)
								{
									tileBuffer[dstIndex + 0] = (char)0;
									tileBuffer[dstIndex + 1] = (char)0;
									tileBuffer[dstIndex + 2] = (char)255;
									tileBuffer[dstIndex + 3] = (char)255;
								}
								else if (mip == 3)
								{
									tileBuffer[dstIndex + 0] = (char)0;
									tileBuffer[dstIndex + 1] = (char)255;
									tileBuffer[dstIndex + 2] = (char)0;
									tileBuffer[dstIndex + 3] = (char)255;
								}
								else // mip 4+
								{
									tileBuffer[dstIndex + 0] = (char)255;
									tileBuffer[dstIndex + 1] = (char)255;
									tileBuffer[dstIndex + 2] = (char)255;
									tileBuffer[dstIndex + 3] = (char)255;
								}
							}
							else
							{
								int srcIndex = (srcY * mips[mip].width + srcX) * bytesPerPixel;
								tileBuffer[dstIndex + 0] = mips[mip].data[srcIndex + 0]; // R
								tileBuffer[dstIndex + 1] = mips[mip].data[srcIndex + 1]; // G
								tileBuffer[dstIndex + 2] = mips[mip].data[srcIndex + 2]; // B
								tileBuffer[dstIndex + 3] = mips[mip].data[srcIndex + 3]; // A
							}
						}
						// else: pixel remains black (padding for out-of-bounds tiles)
					}
				}

				outFile.write(tileBuffer.data(), tileDataSize);
			}
		}
	}

	outFile.close();

	std::cout << "Successfully created: " << outputPath << std::endl;
	std::cout << "File size: " << (currentOffset / 1024) << " KB" << std::endl;
}

bool VTex::LoadHeader(const std::string& vtexPath, VTexHeader& outHeader)
{
	std::ifstream inFile(vtexPath, std::ios::binary);
	if (!inFile.is_open())
	{
		std::cout << "Failed to open .vtex file: " << vtexPath << std::endl;
		return false;
	}

	inFile.read(reinterpret_cast<char*>(&outHeader), sizeof(VTexHeader));

	if (!inFile.good())
	{
		std::cout << "Failed to read header from: " << vtexPath << std::endl;
		inFile.close();
		return false;
	}

	if (outHeader.magic[0] != 'V' || outHeader.magic[1] != 'T' ||
		outHeader.magic[2] != 'E' || outHeader.magic[3] != 'X')
	{
		std::cout << "Invalid .vtex file (bad magic number): " << vtexPath << std::endl;
		inFile.close();
		return false;
	}

	if (outHeader.version != 1)
	{
		std::cout << "Unsupported .vtex version " << outHeader.version << ": " << vtexPath << std::endl;
		inFile.close();
		return false;
	}

	if (outHeader.mipLevels == 0)
	{
		std::cout << "Invalid mipCount (must be at least 1): " << vtexPath << std::endl;
		inFile.close();
		return false;
	}

	inFile.close();
	return true;
}

bool VTex::LoadTile(const std::string& vtexPath, int tileX, int tileY, uint32_t mipLevel, std::vector<char>& outData)
{
	VTexHeader header;
	if (!LoadHeader(vtexPath, header))
	{
		return false;
	}

	if (mipLevel >= header.mipLevels)
	{
		std::cout << "Mip level out of bounds: " << mipLevel << " (max: " << header.mipLevels - 1 << ")" << std::endl;
		return false;
	}

	int mipWidth = std::max(1, (int)header.width >> mipLevel); // width/(2^n)
	int mipHeight = std::max(1, (int)header.height >> mipLevel); // height/(2^n)
	int mipTilesX = (mipWidth + header.tileSize - 1) / header.tileSize;
	int mipTilesY = (mipHeight + header.tileSize - 1) / header.tileSize;

	if (tileX < 0 || tileX >= mipTilesX || tileY < 0 || tileY >= mipTilesY)
	{
		std::cout << "Tile coordinates out of bounds for mip " << mipLevel << ": (" << tileX << ", " << tileY << ") "
			<< "max: (" << mipTilesX << ", " << mipTilesY << ")" << std::endl;
		return false;
	}

	std::ifstream inFile(vtexPath, std::ios::binary);
	if (!inFile.is_open())
	{
		std::cout << "Failed to open .vtex file: " << vtexPath << std::endl;
		return false;
	}

	uint64_t tileTableOffset = sizeof(VTexHeader);
	for (uint32_t mip = 0; mip < mipLevel; mip++)
	{
		int prevMipWidth = std::max(1, (int)header.width >> mip);
		int prevMipHeight = std::max(1, (int)header.height >> mip);
		int prevMipTilesX = (prevMipWidth + header.tileSize - 1) / header.tileSize;
		int prevMipTilesY = (prevMipHeight + header.tileSize - 1) / header.tileSize;
		int prevMipTileCount = prevMipTilesX * prevMipTilesY;

		tileTableOffset += prevMipTileCount * sizeof(VTexTileEntry);
	}

	int tileIndex = tileY * mipTilesX + tileX;
	tileTableOffset += tileIndex * sizeof(VTexTileEntry);

	inFile.seekg(tileTableOffset, std::ios::beg);
	VTexTileEntry tileEntry;
	inFile.read(reinterpret_cast<char*>(&tileEntry), sizeof(VTexTileEntry));

	if (!inFile.good())
	{
		std::cout << "Failed to read tile entry for mip " << mipLevel << " tile (" << tileX << ", " << tileY << ")" << std::endl;
		inFile.close();
		return false;
	}

	inFile.seekg(tileEntry.offset, std::ios::beg);
	outData.resize(tileEntry.compressedSize);
	inFile.read(outData.data(), tileEntry.compressedSize);

	if (!inFile.good())
	{
		std::cout << "Failed to read tile data for mip " << mipLevel << " tile (" << tileX << ", " << tileY << ")" << std::endl;
		inFile.close();
		return false;
	}

	inFile.close();

	/*std::cout << "Successfully loaded mip " << mipLevel << " tile (" << tileX << ", " << tileY << ") from " << vtexPath
		<< " (" << tileEntry.compressedSize << " bytes)" << std::endl;*/

	return true;
}