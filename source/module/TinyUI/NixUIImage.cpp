#include <NixUIRenderer.h>
#include "NixUIImage.h"
#include "NixUISystem.h"
#include <utility>

namespace Nix {

	void UIImage::setRect(const Rect<int16_t>& _rc)
	{
		UIWidget::setRect(_rc);
	}

	void UIImage::addSubWidget(UIWidget* _widget)
	{
		UIWidget::addSubWidget(_widget);
	}

	UIWidget* UIImage::removeSubWidget(uint32_t _index)
	{
		return UIWidget::removeSubWidget(_index);
	}

	void UIImage::updateLayout()
	{
		UIWidget::updateLayout();
	}

	void UIImage::updateDrawingContent()
	{
		UIRenderer* renderer = NixUISystem->getRenderer();
		UIRenderer::ImageDraw draw;
		draw.layer = m_texture->layer;
		draw.uv[0] = m_texture->uv[0];
		draw.uv[1] = m_texture->uv[1];
		draw.uv[2] = m_texture->uv[2];
		draw.uv[3] = m_texture->uv[3];
		draw.rect = this->m_rectRendering;
		draw.color = m_colorMask;
		m_drawData = renderer->build( &draw, 1, m_drawData);
	}

	void UIImage::draw(UIRenderer* _renderer)
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

	void UIImage::setImage( const std::string& _image)
	{
		m_texture = NixUISystem->getTexture(_image);
		m_contentChanged = true;
		//
		NixUISystem->queueUpdate(this);
	}
	void UIImage::setColor(uint32_t _color)
	{
		m_colorMask = _color;
	}
}