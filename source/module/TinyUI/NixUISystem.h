#pragma once
#include <NixUIRenderer.h>

namespace Nix {
	
	enum UIPlatformTouchEvent {
		UITouchDown = 0,
		UITouchMove = 1,
		UITouchUp   = 2
	};

	enum UITouchEvent {
		
	};

    class UIWidget;
    class UISystem {
	public:
		const static uint32_t STANDARD_SCREEN_WIDTH = 1334;
		const static uint32_t STANDARD_SCREEN_HEIGHT = 750;
		//
		uint32_t				m_screenWidth;
		uint32_t				m_screenHeight;
		Nix::Point<float>		m_scale;
		float					m_standardScale;
    private:
		//
        UIRenderer* m_renderer;
    public:
        UISystem(){
        }
		float getStandardScale() const {
			return m_standardScale;
		}
		const Nix::Point<float>& getScale() const {
			return m_scale;
		}
		//
		bool initialize( UIRenderer* _renderer );
		void refresh();

		// system driven handlers
		void onTouch();
		void onTick();
    };
	extern UISystem* NixUISystem;
}