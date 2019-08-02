#pragma once
#include "NixUIWidget.h"
#include "NixUIWidgetUtils.h"
#include "NixUIUtils.h"
#include <NixRenderer.h>

namespace Nix {

	struct UIAlign {
		UIVertAlign vert;
		UIHoriAlign hori;
	};

	class UIWidget {
	private:
		UIWidget*				m_super;
		std::vector<UIWidget*>	m_subWidgets;
		std::string				m_name;
		uint32_t				m_index;
		// the position & size set directly to the widget
		UIAlign					m_align;				// 
		Nix::Rect<int16_t>		m_rect;					// the area of the widgets
		uint8_t					m_ownScissor;			// whether this control has it's own scissor rect
		Nix::Rect<int16_t>*		m_scissor;				// the pointer to the scissor rect
														//
		Nix::Rect<int16_t>		m_rectRendering;		// the widget rect ( real ) that passed to the rendering layer
		Nix::Rect<int16_t>*		m_scissorRendering;		// the scissor rect ( real ) that passed to the rendering layer
	public:
		UIWidget() {
		}

		// modify the widget tree
		virtual void addSubWidget(UIWidget* _widget);
		virtual void removeSubWidget(uint32_t _index);
		// update widget content & layout information
		virtual void updateLayout();
		virtual void updateDrawingContent();
	};
    
}