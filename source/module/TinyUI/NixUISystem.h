#pragma once
#include <set>
#include <vector>
#include <map>

namespace Nix {

	class UIRenderer;
	class UITextureManager;
	struct UITexture;
	
	enum UIPlatformTouchEvent {
		UITouchDown = 0,
		UITouchMove = 1,
		UITouchUp   = 2
	};

	enum UITouchEvent {
	};

	//class WidgetUpdateManager {
	//private:
	//	std::map< uint32_t, std::vector<UIWidget*> > m_levelOfUpdates;
	//public:
	//	WidgetUpdateManager() {

	//	}
	//	void 
	//};

    class UIWidget;
    class UISystem {
	public:
		const static uint32_t STANDARD_SCREEN_WIDTH = 1334;
		const static uint32_t STANDARD_SCREEN_HEIGHT = 750;
		//
		uint32_t				m_screenWidth;
		uint32_t				m_screenHeight;
		uint32_t				m_flightIndex;
		Nix::Point<float>		m_scale;
		float					m_standardScale;
    private:
		//
		IArchieve*				m_archieve;
        UIRenderer*				m_renderer;
		UITextureManager*		m_textureManager;
		UIWidget*				m_dummyRoot;
		UIWidget*				m_rootWidget;
		std::vector<UIWidget*>  m_vecUpdates;
		std::set<UIWidget*>		m_updateSet;
    public:
        UISystem() 
		: m_screenWidth(STANDARD_SCREEN_WIDTH)
		, m_screenHeight(STANDARD_SCREEN_HEIGHT)
		, m_flightIndex( 0 )
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
		UIWidget* getRootWidget() {
			return m_rootWidget;
		}
		const UITexture* getTexture( const std::string& _name );
		//
		bool initialize( IContext* _context, IArchieve* _archieve);
		void refresh();
		//
		void queueUpdate( UIWidget* _widget );
		// system event driven handlers
		//void onTouch();
		void onTick(); // update control & rebuild draw batch
		void onResize( int _width, int _height );
	private:
		void updateUIChanges();
    };
	extern UISystem* NixUISystem;
}