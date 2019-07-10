#pragma once
#include <PxPhysicsAPI.h>
#include <PxFoundation.h>
#include <vector>

namespace Nix {

	PxHeightField* createHeightField( uint8_t * _rawData, uint32_t _row, uint32_t _col, PxVec2 _fieldOffset, PxVec2 _fieldSize );

}