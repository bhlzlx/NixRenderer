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

#include "../FreeCamera.h"

#ifdef _WIN32
    #include <Windows.h>
#endif

namespace Nix {

	struct CVertex {
		glm::vec3 xyz;
		glm::vec3 norm;
		glm::vec2 coord;
		glm::vec3 tangent;
		glm::vec3 bitangent;
	};

	class Mesh {
	private:
		std::vector< VertexType >	m_vertexAttributes;
		IBuffer*					m_vertexBuffer;
		IBuffer*					m_indexBuffer;
	public:
		Mesh() {
		}
	};


	extern std::vector<CVertex> modelVertices;

	static inline glm::mat3x2 calcTB(const glm::vec3& dp1, const glm::vec3 & dp2, const glm::vec2& duv1, const glm::vec2 & duv2) {
		glm::mat3x2 dp = {
			dp1.x, dp2.x, 
			dp1.y, dp2.y, 
			dp1.z, dp2.z
		};
		glm::mat2x2 uv = {
			duv1.x, duv2.x,
			duv1.y, duv2.y
		};
		glm::mat3x2 TB;
		//
		TB = glm::inverse(uv) * dp;
// 		glm::vec3 tangent = glm::vec3(tb[0].x, tb[1].x, tb[2].x);
// 		glm::vec3 btangent = glm::vec3(tb[0].y, tb[1].y, tb[2].y);
		return TB;
	}

	struct Config {
		float eye[3];
		float lookat[3];
		float up[3];
		float light[3];
		NIX_JSON( eye, lookat, up, light )
	};	

	class Sphere : public NixApplication {
	private:
		IArchieve*					m_arch;
		IDriver*					m_driver;
		IContext*					m_context;
		IRenderPass*				m_mainRenderPass;
		IGraphicsQueue*				m_primQueue;
		//
		IMaterial*					m_material;

		FreeCamera					m_camera;

		IArgument*					m_argCommon;
		uint32_t					m_samBase;
		uint32_t					m_matGlobal;

		IArgument*					m_argInstance;
		uint32_t					m_matLocal;

		IRenderable*				m_renderable;
		
		IBuffer*					m_indexBuffer;
		uint32_t					m_indexCount;
		IBuffer*					m_vertexBuffer;
		//
		IPipeline*					m_pipeline;

		ITexture*					m_normalMap;

		glm::mat4					m_model;

		glm::vec3					m_light;

		//
		virtual bool initialize(void* _wnd, Nix::IArchieve* _archieve );

		virtual void resize(uint32_t _width, uint32_t _height);

		virtual void release();

		virtual void tick();

		virtual const char * title();

		virtual uint32_t rendererType();
public:
	virtual void onKeyEvent(unsigned char _key, eKeyEvent _event) override;
	virtual void onMouseEvent(eMouseButton _bt, eMouseEvent _event, int _x, int _y) override;

	};
}

NixApplication* GetApplication();