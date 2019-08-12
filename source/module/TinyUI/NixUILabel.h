#pragma once
#include "NixUIWidget.h"
#include <uchar.h>

namespace Nix {

	class UILabel
		: public UIWidget
	{
	private:
		std::vector< char16_t >		m_u16text;
		uint32_t					m_color;
		uint16_t					m_font;
		uint16_t					m_fontSize;
		UIAlign						m_textAlign;
		Nix::UIDrawData*			m_drawData;
	public:
		UILabel() {
			m_drawData = nullptr;
			m_u16text = { u'¿Õ', u'×Ö', u'·û', u'´®' };
			m_font = 0;
			m_fontSize = 18;
			m_textAlign.hori = UIHoriAlign::UIAlignHoriMid;
			m_textAlign.vert = UIVertAlign::UIAlignVertMid;
			//
			m_color = 0x777777ff;
		}
		~UILabel() {
		}

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
		void setColor(uint32_t _color);
		void setFont( uint16_t _font );
		void setFontSize(uint32_t _size);
		void setTextAlign( const UIAlign& _align );

	};
    
}