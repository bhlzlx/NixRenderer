#include "NixUIRenderer.h"
#include <nix/io/archieve.h>

namespace Nix {

	Nix::PrebuildDrawData* UIRenderer::build(const TextDraw& _draw)
	{
		PrebuildBufferMemoryHeap::Allocation allocation = m_vertexMemoryHeap.allocate(_draw.length);
		//
	}

}