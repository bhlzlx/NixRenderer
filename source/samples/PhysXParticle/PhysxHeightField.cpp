#pragma once
#include "PhysxHeightField.h"
#include <vector>

namespace Nix {

	physx::PxHeightField* createHeightField(uint8_t * _rawData, uint32_t _row, uint32_t _col, PxVec2 _fieldOffset, PxVec2 _fieldSize)
	{
		std::vector<PxHeightFieldSample> samples(_row * _col);
		for ( uint32_t r = 0; r < _row; ++r )
		{
			for (uint32_t c = 0; c < _col; ++c )
			{
				uint32_t index = r * _col + c;
				samples[index].height = uint16_t _rawData;
				if (c & 0x1) {
					samples[index].clearTessFlag();
				} else {
					samples[index].setTessFlag();
				}
				samples[index].materialIndex0 = 0;
				samples[index].materialIndex1 = 0;
			}
		}
		PxHeightFieldDesc hfdesc;
		hfdesc.format = eS16_TM;
		hfdesc.flags = 0;
		hfdesc.nbColumns = _col;
		hfdesc.nbRows = _row;
		PxStridedData strideData;
		strideData.data = _rawData;
		strideData.stride = sizeof( PxHeightFieldSample );
		hfdesc.samples = samples.data();

	}

}