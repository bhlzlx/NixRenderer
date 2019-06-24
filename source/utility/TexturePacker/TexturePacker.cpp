#include "TexturePacker.h"

namespace Nix {

}

NIX_API_DECL void EncodeToDXT5(const uint32_t* _bitmapData, uint32_t _width, uint32_t _height, std::vector<uint8_t>& _data)
{
	uint32_t totalBlockCols = ((_width + 3) & ~(3)) / 4;
	uint32_t totalBlockRows = ((_height + 3) & ~(3)) / 4;
//	uint32_t pixelCol, pixelRow;
	_data.resize(sizeof(Nix::DXT5Block) * totalBlockCols * totalBlockRows);
	std::vector<uint8_t>& compressedData = _data;// (sizeof(Nix::DXT5Block) * totalBlockCols * totalBlockRows);
	memset(compressedData.data(), 0, compressedData.size());
	//
	Nix::DXT5Block* block = (Nix::DXT5Block*)compressedData.data();
	for (uint32_t blockCol = 0; blockCol < totalBlockCols; ++blockCol) {
		for (uint32_t blockRow = 0; blockRow < totalBlockRows; ++blockRow) {
			uint32_t bitmapBlock[16]; // 4x4
			for (uint32_t localPixelCol = 0; localPixelCol < 4; ++localPixelCol) {
				uint32_t pixelCol = blockCol * 4 + localPixelCol;
				for (uint32_t localPixelRow = 0; localPixelRow < 4; ++localPixelRow) {
					uint32_t pixelRow = localPixelRow + blockRow * 4;

					uint32_t localPixelIndex = localPixelRow + localPixelCol * 4;
					uint32_t pixelIndex = pixelRow + pixelCol * _width;
					//
					if (pixelCol >= _height || pixelRow >= _width) {
						bitmapBlock[localPixelRow + localPixelCol * 4] = bitmapBlock[0];
					}else{
						bitmapBlock[localPixelIndex] = ((const uint32_t*)_bitmapData)[pixelIndex];
					}
				}
			}
			block->compressBitmap(bitmapBlock, 4, 4);
			++block;
		}
	}
}

