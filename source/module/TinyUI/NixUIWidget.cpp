#include "NixUIWidget.h"
#include "NixUISystem.h"
#include "NixUIUtils.h"
#include <utility>
#include <cassert>

namespace Nix {
	/*
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
	*/
	
	UIWidget::UIWidget(bool _clipSubWidget) 
		: m_super(nullptr)
		, m_index(-1)
		, m_widgetLevel(-1)
		, m_align({UIAlignVertMid,UIAlignHoriMid})
		, m_layout( UILayoutMode::UIRestrictRatio )
		, m_sicssorSubWidgets( _clipSubWidget )
		, m_scissorRendering( nullptr )
		, m_layoutChanged( true )
		, m_contentChanged ( true )
	{
		m_scissorRendering = new Nix::Scissor();
	}
	
	void UIWidget::setRect(const Rect<int16_t>& _rc)
	{
		m_rect = _rc;
		if (m_super) {
			NixUISystem->queueUpdate(this);
		}
		layoutChanged();
		contentChanged();
	}

	void UIWidget::setAlign(const UIAlign& _align)
	{
		m_align = _align;
		if (m_super) {
			NixUISystem->queueUpdate(this);
		}
	}

	void UIWidget::setLayout(UILayoutMode _layout)
	{
		m_layout = _layout;
		//
		if (m_super) {
			NixUISystem->queueUpdate(this);
		}
	}

	void UIWidget::addSubWidget(UIWidget * _widget)
	{
		// remove the _widget from it's super node
		if (_widget->m_super) {
			_widget->m_super->removeSubWidget(_widget->m_index);
		}
		// add _widget to current sub widgets
		_widget->m_index = (uint32_t)m_subWidgets.size();
		_widget->m_widgetLevel = m_widgetLevel + 1;
		_widget->m_super = this;
		m_subWidgets.push_back(_widget);
		// update the _widget's status
		_widget->layoutChanged();
		_widget->contentChanged();
		//
		NixUISystem->queueUpdate(_widget);
	}

	UIWidget* UIWidget::removeSubWidget(uint32_t _index)
	{
		// check the status
		assert(_index < m_subWidgets.size());
		// remove the widget
		auto it = m_subWidgets.begin() + _index;
		UIWidget* wgt = *it;
		it = m_subWidgets.erase(it);
		// update sub widgets's index
		while (it != m_subWidgets.end()) {
			--(*it)->m_index;
		}
		return wgt;
	}

	void UIWidget::update()
	{
		if (m_layoutChanged) {
			updateLayout();
		}
		if (m_contentChanged) {
			updateDrawingContent();
		}
	}

	void UIWidget::updateLayout()
	{
		// update the layout
		Nix::Rect<int16_t> realRc;
		if (this == NixUISystem->getRootWidget()) {
			realRc = this->m_rect;
		}
		else {
			realRc = alignRect<int16_t>(m_rect, m_super->m_rect, m_align.vert, m_align.hori);
		}		
		//
		float scaleX = 1.0f;
		float scaleY = 1.0f;
		switch (m_layout) {
		case UILayoutMode::UIRestrictRatio: {
			scaleX = scaleY = NixUISystem->getStandardScale();
			break;
		}
		case UILayoutMode::UIStretch: {
			scaleX = NixUISystem->m_scale.x;
			scaleY = NixUISystem->m_scale.y;
			break;
		}
		case UILayoutMode::UIAbsulote: {
			scaleX = scaleY = 1.0f;
			break;
		}
		}
		m_rectRendering.origin.x = realRc.origin.x * scaleX;
		m_rectRendering.origin.y = realRc.origin.y * scaleY;
		m_rectRendering.size.width = realRc.size.width * scaleX;
		m_rectRendering.size.height = realRc.size.height * scaleY;
		//
		if( m_sicssorSubWidgets ) {
			m_scissorRendering->origin.x = m_rectRendering.origin.x;
			m_scissorRendering->origin.y = m_rectRendering.origin.y;
			m_scissorRendering->size.width = m_rectRendering.size.width;
			m_scissorRendering->size.height = m_rectRendering.size.height;
		}
		else {
			m_scissorRendering = m_super->m_scissorRendering;
		}
		for (auto& subWidget : m_subWidgets) {
			subWidget->updateLayout();
			subWidget->updateDrawingContent();
		}
		m_layoutChanged = false;
	}

	void UIWidget::updateDrawingContent(){
		// should done by sub-class
		m_contentChanged = false;
	}

	void UIWidget::draw( UIRenderer* _renderer )
	{
		// should skip invisible widgets
		for (auto& widget : m_subWidgets) {
			widget->draw(_renderer);
		}
	}
}