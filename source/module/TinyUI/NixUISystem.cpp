#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <NixJpDecl.h>

#include <NixUIRenderer.h>
#include "NixUISystem.h"
#include "NixUIWidget.h"

namespace Nix {

	UISystem* NixUISystem = nullptr;

	UISystem* UISystem::getInstance()
	{
		if (!NixUISystem) {
			NixUISystem = new UISystem();
		}
		return NixUISystem;
	}

	void UISystem::release()
	{
		// destroy objects
		delete NixUISystem;
		NixUISystem = nullptr;
	}

	const UITexture* UISystem::getTexture(const std::string& _name)
	{
		const UITexture* handle = nullptr;
		m_textureManager->getUITexture(_name, &handle);
		return handle;
	}

	bool UISystem::initialize(IContext* _context, IArchieve* _archieve )
	{
		assert(_context && _archieve);
		UIRenderer* renderer = new UIRenderer();
		//
		m_archieve = _archieve;
		m_renderer = renderer;
		UIRenderConfig config;
		TextReader configReader;
		configReader.openFile(_archieve, "ui/config.json");
		config.parse(configReader.getText());
		renderer->initialize(_context, _archieve, config);
		m_textureManager = m_renderer->getUITextureManager();
		//
		m_rootWidget = new UIWidget();
		//
		return true;
	}

	//
	void UISystem::refresh(){
		m_rootWidget->update();
	}

	void UISystem::queueUpdate(UIWidget* _widget)
	{
		if (m_updateSet.find(_widget) == m_updateSet.end()) {
			m_vecUpdates.push_back(_widget);
		}
	}

	void UISystem::onTick(){
		updateUIChanges();
	}

	void UISystem::onResize(int _width, int _height){
		Nix::Rect<int16_t> rc;
		rc.origin = { 0, 0 };
		rc.size = { (int16_t)_width, (int16_t)_height };
		m_rootWidget->setRect(rc);
		m_rootWidget->update();
	}

	void UISystem::updateUIChanges(){
		if (m_vecUpdates.size()) {
			// do widget update stuffs
			for (auto widget : m_vecUpdates) {
				widget->update();
			}
			// clean up
			m_updateSet.clear();
			m_vecUpdates.clear();
		}
	}

}