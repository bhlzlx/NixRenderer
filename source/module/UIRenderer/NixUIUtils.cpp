#include "NixUIUtils.h"
#include <algorithm>

namespace Nix {

	/* ----------
	*	0 -- 3
	*	| \  |
	*	|  \ |
	*	1 -- 2
	* --------- */

	//
	VertexClipFlags clipRect(UIVertex* _rc, const Nix::Scissor& _scissor, UIVertex* _output)
	{
		memcpy( _output, _rc, sizeof(UIVertex) );
		_output[0].x = std::max((float)_scissor.origin.x, _rc[0].x);
		_output[0].y = std::min((float)(_scissor.origin.y+_scissor.size.height), _rc[0].y);
		_output[2].x = std::min((float)(_scissor.origin.x+_scissor.size.width), _rc[2].x);
		_output[2].y = std::max((float)(_scissor.origin.y), _rc[2].y);
		if (_output[0].x >= _output[2].x || _output[0].y <= _output[2].y) {
			return VertexClipFlagBits::AllClipped;
		}
		// update uv
		float width = _rc[3].x - _rc[0].x;
		float height = _rc[0].y - _rc[1].y;
		float ustride = _rc[3].u - _rc[0].u;
		float vstride = _rc[0].v - _rc[1].v;
		//
		_output[0].u = (_output[0].x - _rc[0].x) / width * ustride + _rc[0].u;
		_output[2].u = (_output[2].x - _rc[0].x) / width * ustride + _rc[0].u;
		_output[0].v = (_output[0].y - _rc[1].y) / height * vstride + _rc[1].v;
		_output[2].v = (_output[2].y - _rc[1].y) / height * vstride + _rc[1].v;
		//
		_output[1].x = _output[0].x; _output[1].y = _output[2].y;
		_output[3].x = _output[2].x; _output[3].y = _output[0].y;

		_output[1].u = _output[0].u; _output[1].v = _output[2].v;
		_output[3].u = _output[2].u; _output[3].v = _output[0].v;
		//
		return VertexClipFlagBits::NoneClipped | PartClipped;
	}
}


