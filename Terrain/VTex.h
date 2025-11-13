#pragma once
#include "Core.h"

struct VTexHeader
{
	char magic[4];           // "VTEX"
	uint32_t version;        // File format version
	uint32_t width;          // Image width in pixels
	uint32_t height;         // Image height in pixels
	uint32_t tileSize;       // Tile dimension (e.g., 256 = 256x256)
	uint32_t tilesX;         // Number of tiles horizontally
	uint32_t tilesY;         // Number of tiles vertically
	uint32_t bytesPerPixel;  // Bytes per pixel (4 for RGBA8)
	uint32_t padding;        // Reserved for future use
};

struct VTexTileEntry
{
	uint64_t offset;         // Offset in file where tile data starts
	uint32_t compressedSize; // Size of tile data (uncompressed = tileSize * tileSize * bytesPerPixel)
};

class VTex
{
public:
	static void ConvertToVTex(const std::string& path, int tileSize = 256);
	static bool LoadHeader(const std::string& vtexPath, VTexHeader& outHeader);
	static bool LoadTile(const std::string& vtexPath, int tileX, int tileY, std::vector<char>& outData);
};