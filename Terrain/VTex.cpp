#include "VTex.h"
#include "Texture.h"
#include <fstream>

void VTex::ConvertToVTex(const std::string & path, int tileSize)
{
	std::cout << "Converting " << path << " to .vtex format..." << std::endl;

	Image sourceImage;
	sourceImage.LoadImageFromFile(path, true, STBI_rgb_alpha);

	if (!sourceImage.bytes)
	{
		std::cout << "Failed to load source image: " << path << std::endl;
		return;
	}

	std::cout << "Loaded image size : " << sourceImage.width << "x" << sourceImage.height << std::endl;

	int tilesX = (sourceImage.width + tileSize - 1) / tileSize; // Ceiling division
	int tilesY = (sourceImage.height + tileSize - 1) / tileSize;
	int totalTiles = tilesX * tilesY;

	std::cout << "Tile layout: " << tilesX << "x" << tilesY << " (" << totalTiles << " tiles of "
		<< tileSize << "x" << tileSize << " pixels)" << std::endl;

	const int bytesPerPixel = 4; // RGBA8
	const int tileDataSize = tileSize * tileSize * bytesPerPixel;

	VTexHeader header = {};
	header.magic[0] = 'V';
	header.magic[1] = 'T';
	header.magic[2] = 'E';
	header.magic[3] = 'X';
	header.version = 1;
	header.width = sourceImage.width;
	header.height = sourceImage.height;
	header.tileSize = tileSize;
	header.tilesX = tilesX;
	header.tilesY = tilesY;
	header.bytesPerPixel = bytesPerPixel;
	header.padding = 0;

	std::vector<VTexTileEntry> tileTable(totalTiles);

	uint64_t currentOffset = sizeof(VTexHeader) + sizeof(VTexTileEntry) * totalTiles;

	for (int i = 0; i < totalTiles; i++)
	{
		tileTable[i].offset = currentOffset;
		tileTable[i].compressedSize = tileDataSize;
		currentOffset += tileDataSize;
	}

	std::vector<char> tileBuffer(tileDataSize);

	std::string outputPath = path.substr(0, path.find_last_of('.')) + ".vtex";

	std::ofstream outFile(outputPath, std::ios::binary);
	if (!outFile.is_open())
	{
		std::cout << "Failed to create output file: " << outputPath << std::endl;
		return;
	}

	outFile.write(reinterpret_cast<const char*>(&header), sizeof(VTexHeader));
	outFile.write(reinterpret_cast<const char*>(tileTable.data()), sizeof(VTexTileEntry) * totalTiles);

	for (int tileY = 0; tileY < tilesY; tileY++)
	{
		for (int tileX = 0; tileX < tilesX; tileX++)
		{
			// Clear tile buffer (for padding if tile extends beyond image bounds)
			std::fill(tileBuffer.begin(), tileBuffer.end(), 0);

			for (int y = 0; y < tileSize; y++)
			{
				for (int x = 0; x < tileSize; x++)
				{
					int srcX = tileX * tileSize + x;
					int srcY = tileY * tileSize + y;

					if (srcX < sourceImage.width && srcY < sourceImage.height)
					{
						int srcIndex = (srcY * sourceImage.width + srcX) * bytesPerPixel;
						int dstIndex = (y * tileSize + x) * bytesPerPixel;

						tileBuffer[dstIndex + 0] = sourceImage.bytes[srcIndex + 0]; // R
						tileBuffer[dstIndex + 1] = sourceImage.bytes[srcIndex + 1]; // G
						tileBuffer[dstIndex + 2] = sourceImage.bytes[srcIndex + 2]; // B
						tileBuffer[dstIndex + 3] = sourceImage.bytes[srcIndex + 3]; // A
					}
					// else: pixel remains black (padding for out-of-bounds tiles)
				}
			}

			outFile.write(tileBuffer.data(), tileDataSize);
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

	inFile.close();
	return true;
}

bool VTex::LoadTile(const std::string& vtexPath, int tileX, int tileY, std::vector<char>& outData)
{
	VTexHeader header;
	if (!LoadHeader(vtexPath, header))
	{
		return false;
	}

	if (tileX < 0 || tileX >= static_cast<int>(header.tilesX) ||
		tileY < 0 || tileY >= static_cast<int>(header.tilesY))
	{
		std::cout << "Tile coordinates out of bounds: (" << tileX << ", " << tileY << ") "
			<< "max: (" << header.tilesX << ", " << header.tilesY << ")" << std::endl;
		return false;
	}

	int tileIndex = tileY * header.tilesX + tileX;

	std::ifstream inFile(vtexPath, std::ios::binary);
	if (!inFile.is_open())
	{
		std::cout << "Failed to open .vtex file: " << vtexPath << std::endl;
		return false;
	}

	uint64_t tileTableOffset = sizeof(VTexHeader) + tileIndex * sizeof(VTexTileEntry);
	inFile.seekg(tileTableOffset, std::ios::beg); // Move to tile entry

	VTexTileEntry tileEntry;
	inFile.read(reinterpret_cast<char*>(&tileEntry), sizeof(VTexTileEntry));

	if (!inFile.good())
	{
		std::cout << "Failed to read tile entry for tile (" << tileX << ", " << tileY << ")" << std::endl;
		inFile.close();
		return false;
	}

	inFile.seekg(tileEntry.offset, std::ios::beg); // Move to tile actual data
	outData.resize(tileEntry.compressedSize);
	inFile.read(outData.data(), tileEntry.compressedSize);

	if (!inFile.good())
	{
		std::cout << "Failed to read tile data for tile (" << tileX << ", " << tileY << ")" << std::endl;
		inFile.close();
		return false;
	}

	inFile.close();

	/*std::cout << "Successfully loaded tile (" << tileX << ", " << tileY << ") from " << vtexPath
		<< " (" << tileEntry.compressedSize << " bytes)" << std::endl;*/

	return true;
}