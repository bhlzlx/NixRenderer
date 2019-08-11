#pragma once
#include "NixUIWidget.h"
#include <uchar.h>

namespace Nix {

	class UILabel
		: UIWidget
	{
	private:
		std::vector< char16_t > m_u16text;
	public:
		virtual void setRect(const Rect<int16_t>& _rc) override;
		// modify the widget tree
		virtual void addSubWidget(UIWidget* _widget) override;
		virtual UIWidget* removeSubWidget(uint32_t _index) override;
		// update widget content & layout information
		virtual void updateLayout() override;
		virtual void updateDrawingContent() override;
		virtual void draw(UIRenderer* _renderer) override;
		//
		void setText( const std::vector<char16_t>& _u16 );
		void setImage(const std::string& _image);
		void setColor(uint32_t _color);
	};
    
}