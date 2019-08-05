#pragma once
#include "NixUIWidget.h"
#include "NixUITextureManager.h"
#include <string>

namespace Nix {

	class UIRenderer;

	class UIImage
		: public UIWidget {
	protected:
		std::string				m_imageName;		// the name of the image
		uint32_t				m_colorMask;		// mask
		const UITexture*		m_texture;			// the texture handle
		UIDrawData*				m_drawData;			// builded draw data
	public:
		UIImage()
			: m_imageName("")
			, m_colorMask( 0xffffffff )
			, m_texture(nullptr)
			, m_drawData(nullptr)
		{
		}

		virtual void setRect(const Rect<int16_t>& _rc) override;
		// modify the widget tree
		virtual void addSubWidget(UIWidget* _widget) override;
		virtual UIWidget* removeSubWidget(uint32_t _index) override;
		// update widget content & layout information
		virtual void updateLayout() override;
		virtual void updateDrawingContent() override;
		virtual void draw( UIRenderer* _renderer) override;
		//
		void setImage( const std::string& _image );
		void setColor( uint32_t _color );
	};
}