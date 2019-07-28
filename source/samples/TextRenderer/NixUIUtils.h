#pragma once
#include "NixUIDefine.h"

namespace Nix {

	/*---------------------------------------------------
	* given the width & the left/right & the alignment
	* output the align result
	* this example shows how this function works with `UIAlignHoriMid`
	*
	*     left                             right
	*     |                                    |
	*				      width
	*	               |---------|
	*         alignedLeft      alignedRight
	*---------------------------------------------------*/

	template< class T >
	void alignHorizontal(T _width, T _left, T _right, UIHoriAlign _align, T& _alignedLeft, T& _alignedRight) {
		switch (_align) {
		case UIAlignLeft: {
			_alignedLeft = _left;
			_alignedRight = _alignedLeft + _width;
			return;
		}
		case UIAlignRight: {
			_alignedRight = _right;
			_alignedLeft = _alignedRight - _width;
			return;
		}
		case UIAlignHoriMid: {
			_alignedLeft = (_left + _right - _width) >> 1;
			_alignedRight = _alignedLeft + _width;
			return;
		}
		}
	}

	/*---------------------------------------------------
	* given the height & the top/bottom & the alignment
	* output the align result
	*---------------------------------------------------*/

	template< class T >
	void alignVertical(T _height, T _top, T _bottom, UIVertAlign _align, T& _alignTop, T& _alignBottom) {
		switch (_align) {
		case UIAlignTop: {
			_alignedTop = _top;
			_alignedBottom = _right + _width;
			return;
		}
		case UIAlignBottom: {
			_alignedBottom = _bottom;
			_alignedTop = _bottom + _height;
			return;
		}
		case UIAlignVertMid: {
			_alignedBottom = (_top + _bottom - _height) >> 1;
			_alignedTop = _alignedBottom + _height;
			return;
		}
		}
	}

	template< class T >
	Nix::Rect<T> alignRect(const Nix::Rect<T>& _rc, const Nix::Rect<T>& _alignTo, UIVertAlign _v, UIHoriAlign _h) {
		T top, bottom;
		T left, right;
		alignVertical<T>(
			_rc.size.height,
			_alignTo.origin.y + _alignTo.size.height,
			_alignTo.origin.y,
			_v,
			top,
			bottom
			);
		alignHorizontal<T>(
			_rc.size.width,
			_alignTo.origin.x,
			_alignTo.origin.x + _alignTo.size.width,
			_h,
			left,
			right
			);
		Nix::Rect<T> rc = {
			{ left, bottom },
			{ right - left, top - bottom }
		};
		return rc;
	}

}
