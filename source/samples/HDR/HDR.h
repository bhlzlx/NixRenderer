#include <NixApplication.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <NixJpDecl.h>
#include <NixRenderer.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tiny_obj_loader.h>

#include <nix/io/archieve.h>
#include <cstdio>
#include <cassert>

#ifdef _WIN32
    #include <Windows.h>
#endif

namespace Nix {

	struct Config {
		float eye[3];
		float lookat[3];
		float up[3];
		float light[3];
		NIX_JSON(eye, lookat, up, light)
	};

	class HDR : public NixApplication {
	private:
		IArchieve*					m_arch;
		IDriver*					m_driver;
		IContext*					m_context;
		IRenderPass*				m_mainRenderPass;
		IGraphicsQueue*				m_primQueue;

		// ============== vertex buffer & index buffer ==============
		//IBuffer*					m_indexBuffer;
		//uint32_t					m_indexCount;
		IBuffer*					m_vertPosition;
		uint32_t					m_vertCount;
		IBuffer*					m_vertNormal;

		glm::mat4					m_projection;
		glm::mat4					m_view;
		glm::mat4					m_model;

		glm::vec3					m_lights[3];

		// ============== depth pass for Eary-Z ==============

		IMaterial*					m_mtlDp;
		IArgument*					m_argDp;
		IRenderable*				m_renderableDp;
		IPipeline*					m_pipelineDp;

		RenderPassDescription		m_renderPassDpDesc;
		IAttachment*				m_depthDp;
		IRenderPass*				m_renderPassDp;

		glm::mat4					m_mvpDp;

		// ============== color pass with Early-Z ==============

		IMaterial*					m_mtlEarlyZ;
		IArgument*					m_argEarlyZ;
		IRenderable*				m_renderableEarlyZ;
		IPipeline*					m_pipelineEarlyZ;

		//
		virtual bool initialize(void* _wnd, Nix::IArchieve* _archieve);
		virtual void resize(uint32_t _width, uint32_t _height);
		virtual void release();
		virtual void tick();
		virtual const char * title();
		virtual uint32_t rendererType();
	};
}