#pragma once
#include <set>
#include <vector>

namespace Nix {

	class UIRenderer;
	class UITextureManager;
	class UITexture;
	
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
		IArchieve*				m_archieve;
        UIRenderer*				m_renderer;
		UITextureManager*		m_textureManager;
		UIWidget*				m_rootWidget;
		std::vector<UIWidget*>  m_vecUpdates;
		std::set<UIWidget*>		m_updateSet;
    public:
        UISystem() 
		: m_screenWidth(STANDARD_SCREEN_WIDTH)
		, m_screenHeight(STANDARD_SCREEN_HEIGHT)
		, m_scale({ 1.0f, 1.0f })
		, m_standardScale(1.0f)
		, m_archieve( nullptr)
		, m_renderer( nullptr )
		, m_rootWidget( nullptr )
		{
        }
		static UISystem* getInstance();
		//
		void release();
		float getStandardScale() const {
			return m_standardScale;
		}
		const Nix::Point<float>& getScale() const {
			return m_scale;
		}
		UIRenderer* getRenderer() {
			return m_renderer;
		}
		const UITexture* getTexture( const std::string& _name );
		//
		bool initialize( IContext* _context, IArchieve* _archieve);
		void refresh();
		//
		void queueUpdate( UIWidget* _widget );
		// system event driven handlers
		//void onTouch();
		void onTick();
		void onResize( int _width, int _height );
	private:
		void updateUIChanges();
    };
	extern UISystem* NixUISystem;
}