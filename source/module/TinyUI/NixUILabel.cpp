#include "NixUILabel.h"
#include "NixUIUtils.h"
#include "NixUISystem.h"
#include "NixUIRenderer.h"
#include <utility>

namespace Nix {
	void UILabel::setRect(const Rect<int16_t>& _rc)
	{
		UIWidget::setRect(_rc);
	}

	void UILabel::addSubWidget(UIWidget* _widget)
	{
		UIWidget::addSubWidget(_widget);
	}

	UIWidget* UILabel::removeSubWidget(uint32_t _index)
	{
		return UIWidget::removeSubWidget(_index);
	}

	void UILabel::updateLayout()
	{
		UIWidget::updateLayout();
	}

	void UILabel::updateDrawingContent()
	{
		if (m_u16text.size()) {
			UIRenderer* renderer = NixUISystem->getRenderer();
			UIRenderer::PlainTextDraw draw;
			draw.colorMask = m_color;
			draw.fontId = m_font;
			draw.fontSize = m_fontSize * NixUISystem->getStandardScale();
			if (draw.fontSize < 8) {
				draw.fontSize = 8;
			}
			draw.halign = m_textAlign.hori;
			draw.valign = m_textAlign.vert;
			draw.length = m_u16text.size();
			draw.rect = m_rectRendering;
			draw.text = m_u16text.data();
			Nix::Rect<float> rc;
			m_drawData = renderer->build(draw, m_drawData, rc);
		}		
	}

	void UILabel::draw(UIRenderer* _renderer)
	{
		if (m_drawData) {
			UIDrawState state;
			Nix::Scissor scissor;
			scissor.origin.x = m_scissorRendering->origin.x;
			scissor.origin.y = m_scissorRendering->origin.y;
			scissor.size.width = m_scissorRendering->size.width;
			scissor.size.height = m_scissorRendering->size.height;
			state.setScissor(scissor);
			_renderer->buildDrawCall(m_drawData, state);
		}
		UIWidget::draw(_renderer);
	}

	void UILabel::setText(const std::vector<char16_t>& _u16)
	{
		m_u16text = _u16;
		m_contentChanged = true;
		NixUISystem->queueUpdate(this);
	}

	void UILabel::setColor(uint32_t _color)
	{
		if (m_color == _color) return;
		m_color = _color;
		m_contentChanged = true;
		NixUISystem->queueUpdate(this);
	}

	void UILabel::setFont(uint16_t _font) {
		if (m_font != _font) {
			m_font = _font;
			m_contentChanged = true;
			NixUISystem->queueUpdate(this);
		}

	}
	void UILabel::setFontSize(uint32_t _size) {
		if (m_fontSize != _size) {
			m_fontSize = _size;
			m_contentChanged = true;
			NixUISystem->queueUpdate(this);
		}
	}
	void UILabel::setTextAlign(const UIAlign& _align) {
		if (m_textAlign.hori != _align.hori && m_textAlign.vert != _align.vert ) {
			m_align = _align;
			m_contentChanged = true;
			NixUISystem->queueUpdate(this);
		}
	}
}