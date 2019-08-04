#pragma once
#include "NixUIWidget.h"
#include "NixUIWidgetUtils.h"
#include "NixUIUtils.h"
#include <NixRenderer.h>
#include <string>

namespace Nix {

	struct UIAlign {
		UIVertAlign vert;
		UIHoriAlign hori;
	};

	enum UILayoutMode {
		UIRestrictRatio = 0,
		UIStretch		= 1,
	};

	class UIRenderer;

	class UIWidget {
	protected:
		UIWidget*				m_super;
		std::vector<UIWidget*>	m_subWidgets;
		std::string				m_name;
		uint32_t				m_index;
		// the position & size set directly to the widget
		UIAlign					m_align;				// 
		UILayoutMode			m_layout;				//
		Nix::Rect<int16_t>		m_rect;					// the area of the widgets
		uint8_t					m_sicssorSubWidgets;    // whether this control has it's own scissor rect
		//
		Nix::Rect<float>		m_rectRendering;		// the widget rect ( real ) that passed to the rendering layer
		Nix::Scissor*			m_scissorRendering;		// the scissor rect ( real ) that passed to the rendering layer
		//
		uint8_t					m_layoutChanged;		// need to update the rect rendering
		uint8_t					m_contentChanged;		// need to update the draw data
	public:
		UIWidget(bool _clipSubWidget = false);
		//
		virtual void setRect( const Rect<int16_t>& _rc );
		// modify the widget tree
		virtual void addSubWidget(UIWidget* _widget);
		virtual void removeSubWidget(uint32_t _index);
		// update widget content & layout information
		virtual void update();
		virtual void updateLayout();
		virtual void updateDrawingContent();
		virtual void draw( UIRenderer* _renderer );
	protected:
		void layoutChanged() {
			m_layoutChanged = true;
		}
		void contentChanged() {
			m_contentChanged = true;
		}
	};
    
}