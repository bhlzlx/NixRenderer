#pragma once

#ifdef _WIN32

#pragma warning(disable:4251)

#ifdef NIX_DYNAMIC_LINK
	#define NIX_API_DECL _declspec(dllexport)
#elif defined NIX_STATIC_LINK
	#define NIX_API_DECL __declspec(dllimport)
#else
	#define NIX_API_DECL
#endif
#else
#define NIX_API_DECL 
#endif

#ifndef NIX_JSON
#define NIX_JSON(...)
#endif

#include <memory>
#include <vector>
#include <string>

namespace Nix {

	class IArchieve;

	static const uint32_t MaxRenderTarget = 4;
    static const uint32_t MaxVertexAttribute = 16;
	static const uint32_t MaxVertexBufferBinding = 8;
	static const uint32_t UniformChunkSize = 1024 * 512; // 512KB
	static const uint32_t MaxFlightCount = 3;
	static const uint32_t MaxArgumentCount = 4;

    template < class T >
    struct Point {
        T x;
        T y;
    };

    template < class T >
    struct Size {
        T width;
        T height;
    };

    template < class T >
    struct Size3D {
        T width;
        T height;
        T depth;
    };

    template< class T >
    struct Rect {
        Point<T> origin;
        Size<T> size;
    };

	template< class T >
	struct Offset2D {
		T x, y;
	};

	template< class T >
	struct Offset3D {
		T x, y, z;
	};

    struct Viewport {
        float x,y;
        float width,height;
        float zNear,zFar;
    };

    typedef Rect<int> Scissor;

	struct TextureRegion {
		uint32_t mipLevel; // mip map level
		// for texture 2d, baseLayer must be 0
		// for texture 2d array, baseLayer is the index
		// for texture cube, baseLayer is from 0 ~ 5
		// for texture cube array, baseLayer is ( index * 6 + face )
		// for texture 3d, baseLayer is ( depth the destination layer )
		// for texture 3d array, baseLayer is ( {texture depth} * index + depth of the destination layer )
		uint32_t baseLayer;
		// for texture 2d, offset.z must be 0
		Offset3D<uint32_t> offset;
		// for texture 2d, size.depth must be 1
		Size3D<uint32_t> size;
	};

    enum DriverType {
        Software = 0,
        GLES3,
        Metal,
        Vulkan,
		DX12,
        OpenGLCore
    };

	enum DeviceType {
		OtherGPU = 0,
		IntegratedGPU,
		DiscreteGPU,
		VirtualGPU,
		CPU,
	};

    enum ShaderModuleType {
        VertexShader = 0,
        FragmentShader,
        ComputeShader
    };

	enum NixFormat : uint8_t {
		NixInvalidFormat,
		// 32 bit
		NixRGBA8888_UNORM,
		NixBGRA8888_UNORM,
		NixRGBA8888_SNORM,
		// 16 bit
		NixRGB565_PACKED,
		NixRGBA5551_PACKED,
		// Float type
		NixRGBA_F16,
		NixRGBA_F32,
		// Depth stencil type
		NixDepth24FStencil8,
		NixDepth32F,
		NixDepth32FStencil8,
		// compressed type
		NixETC2_LINEAR_RGBA,
		NixEAC_RG11_UNORM,
		NixBC1_LINEAR_RGBA,
		NixBC3_LINEAR_RGBA,
		NixPVRTC_LINEAR_RGBA,
	};

    enum PolygonMode :uint8_t {
        PMPoint = 0,
        PMLine,
        PMFill
    };

    enum TopologyMode :uint8_t {
        TMPoints = 0,
        TMLineStrip,
        TMLineList,
		TMTriangleStrip,
		TMTriangleList,
		TMTriangleFan,
		TMCount,
    };

    enum CullMode :uint8_t {
        CullNone = 0,
        CullBack = 1,
        CullFront = 2,
		CullAll = 3
    };

    enum WindingMode : uint8_t {
        Clockwise = 0,
        CounterClockwise = 1,
    };

    enum CompareFunction : uint8_t {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        GreaterEqual,
        Always,
    };

    enum BlendFactor :uint8_t {
        Zero,
        One,
        SourceColor,
        InvertSourceColor,
        SourceAlpha,
        InvertSourceAlpha,
		SourceAlphaSat,
        //
        DestinationColor,
        InvertDestinationColor,
        DestinationAlpha,
        InvertDestinationAlpha,
    };

    enum BlendOperation :uint8_t {
        BlendOpAdd,
        BlendOpSubtract,
        BlendOpRevsubtract,
    };

    enum StencilOperation :uint8_t {
        StencilOpKeep,
        StencilOpZero,
        StencilOpReplace,
        StencilOpIncrSat,
        StencilOpDecrSat,
        StencilOpInvert,
        StencilOpInc,
        StencilOpDec
    };

    enum AddressMode :uint8_t {
        AddressModeWrap,
        AddressModeClamp,
        AddressModeMirror,
    };

    enum TextureFilter :uint8_t {
        TexFilterNone,
        TexFilterPoint,
        TexFilterLinear
    };

    enum TextureCompareMode :uint8_t {
        RefNone = 0,
        RefToTexture = 1
    };

	enum VertexType :uint8_t {
		VertexTypeFloat1,
		VertexTypeFloat2,
		VertexTypeFloat3,
		VertexTypeFloat4,
		VertexTypeHalf2,
		VertexTypeHalf4,
		VertexTypeUByte4,
		VertexTypeUByte4N,
	};

	enum TextureType :uint8_t {
		Texture1D,
		Texture2D,
		Texture2DArray,
		TextureCube,
		TextureCubeArray,
		Texture3D
	};

	enum TextureUsageFlagBits :uint8_t {
		TextureUsageNone = 0x0,
		TextureUsageTransferSource = 0x1,
		TextureUsageTransferDestination = 0x2,
		TextureUsageSampled = 0x4,
		TextureUsageStorage = 0x8,
		TextureUsageColorAttachment = 0x10,
		TextureUsageDepthStencilAttachment = 0x20,
		TextureUsageTransientAttachment = 0x40,
		TextureUsageInputAttachment = 0x80
	};

	enum AttachmentUsageBits : uint8_t {
		AOU_ColorAttachment,
		AOU_DepthStencilAttachment,
		AOU_ShaderRead,
		AOU_Present,
	};

	enum BufferType :uint8_t {
		SVBO, // stable vertex buffer object
		CVBO, // cached vertex buffer object
		IBO, // index buffer object
		UBO, // uniform buffer object
		SSBO, // shader storage buffer objects
		TBO, // Texel buffer object
	};

	enum MultiSampleType {
		MsaaNone,
		Msaax2,
		Msaax4,
		Msaax8,
		Msaax16
	};

	enum AttachmentLoadAction :uint8_t {
		Keep,
		Clear,
		DontCare,
	};

#pragma pack(push)
#pragma pack(1)
    struct SamplerState {
        AddressMode u : 8;
        AddressMode v : 8;
        AddressMode w : 8;
        TextureFilter min : 8;
        TextureFilter mag : 8;
        TextureFilter mip :8 ;
        //
        TextureCompareMode compareMode : 8;
        CompareFunction compareFunction : 8;
        bool operator < ( const SamplerState& _ss ) const {
            return *(uint64_t*)this < *(uint64_t*)&_ss;
        }
    };
#pragma pack(pop)

    struct VertexAttribueDescription {
		std::string name;
        uint32_t    bufferIndex;
        uint32_t    offset;
        VertexType  type;
        VertexAttribueDescription() : 
            bufferIndex(0),
            offset(0),
            type(VertexTypeFloat1){
            }
		NIX_JSON( name, bufferIndex, offset, type )
    };

    struct VertexBufferDescription {
        uint32_t stride;
        uint32_t instanceMode;
        uint32_t rate;
        VertexBufferDescription():
        stride(0),
        instanceMode(0),
        rate(1){
        }
		NIX_JSON( stride, instanceMode, rate )
    };

    struct VertexLayout {
        uint32_t                    attributeCount;
        VertexAttribueDescription   attributes[MaxVertexAttribute];
        uint32_t                    bufferCount;
        VertexBufferDescription     buffers[MaxVertexBufferBinding];
        VertexLayout(): attributeCount(0), bufferCount(0) {
        }
		NIX_JSON(attributeCount, attributes, bufferCount, buffers)
    };

	struct DepthState {
		uint8_t             writable = 1;
		uint8_t             testable = 1;
		CompareFunction     cmpFunc = CompareFunction::LessEqual;
		NIX_JSON(writable, testable, cmpFunc)
	};

	struct BlendState {
		uint8_t             enable = 1;
		BlendFactor         srcFactor = SourceAlpha;
		BlendFactor         dstFactor = InvertSourceAlpha;
		BlendOperation      op = BlendOpAdd;
		NIX_JSON(enable, srcFactor, dstFactor, op)
	};

	struct StencilState {
		uint8_t             enable = 0;
		StencilOperation    opFail = StencilOpKeep;
		StencilOperation    opZFail = StencilOpKeep;
		StencilOperation    opPass = StencilOpKeep;
		uint8_t             enableCCW = 0;
		StencilOperation    opFailCCW = StencilOpKeep;
		StencilOperation    opZFailCCW = StencilOpKeep;
		StencilOperation    opPassCCW = StencilOpKeep;
		CompareFunction     cmpFunc = Greater;
		uint32_t            mask = 0xffffffff;
		NIX_JSON(enable, opFail, opZFail, opPass, enableCCW, opFailCCW, opZFailCCW, opPassCCW, cmpFunc, mask)
	};

    enum ColorMask {
        MaskRed = 1,
        MaskGreen = 2,
        MaskBlue = 4,
        MaskAlpha = 8
    };

    struct RenderState {
		uint8_t				writeMask;
		CullMode            cullMode = CullMode::CullNone;
		WindingMode         windingMode = Clockwise;
		uint8_t             scissorEnable = 1;
		DepthState			depthState;
		BlendState			blendState;
		StencilState		stencilState;
		//
		NIX_JSON(writeMask, cullMode, windingMode, scissorEnable, depthState, blendState, stencilState )
    };

	typedef uint8_t TextureUsageFlags;

    struct TextureDescription {
		TextureType type;
        NixFormat format;
        uint32_t mipmapLevel;
		//
        uint32_t width;
        uint32_t height;
        uint32_t depth;
    };

	// RenderPassDescription describe the 
	// load action
#pragma pack( push, 1 )
	struct AttachmentDescription {
		NixFormat						format = NixInvalidFormat;
		MultiSampleType					multisample = MsaaNone;
		AttachmentLoadAction			loadAction = DontCare;
		AttachmentUsageBits				usage = AOU_ColorAttachment;
		//
		NIX_JSON(format, multisample, loadAction, usage)
	};
	struct RenderPassDescription {
		// render pass behavior
		uint32_t colorCount = 0;
		// framebuffer description
		AttachmentDescription colors[MaxRenderTarget];
		AttachmentDescription depthStencil;
		uint32_t inputCount = 0;
		AttachmentDescription inputs[MaxRenderTarget];
		AttachmentDescription resolve;

		NIX_JSON(colorCount, colors, depthStencil)
	};
#pragma pack (pop)

	struct RpClear {
		struct color4 {
			float r, g, b, a;
		} colors[MaxRenderTarget];
		float depth;
		int stencil;
	};

	enum ShaderDescriptorType {
		SDT_UniformBlock,
		SDT_Sampler,
		SDT_SSBO,
		SDT_TBO,
	};

	struct ShaderDescriptor {
		ShaderDescriptorType	type;
		uint32_t				binding;
		std::string				name;
		ShaderModuleType		shaderStage;
		uint32_t				dataSize;
		NIX_JSON( type, binding, name, shaderStage, dataSize)
	};

	struct ArgumentDescription {
		uint32_t						index;
		std::vector< ShaderDescriptor > descriptors;
		NIX_JSON(index, descriptors)
	};

	struct MaterialDescription {
		char									vertexShader[64];
		char									fragmentShader[64];
		VertexLayout							vertexLayout;
		std::vector<ArgumentDescription>		argumentLayouts;
		//
		RenderState								renderState;
		//
		TopologyMode							topologyMode;
		PolygonMode								pologonMode;
		//
		NIX_JSON(vertexShader, fragmentShader, vertexLayout, argumentLayouts, renderState, topologyMode, pologonMode)
	};

	class IRenderPass;
	class IContext;
	class IGraphicsQueue;

    class NIX_API_DECL ITexture {
	public:
        virtual const TextureDescription& getDesc() const = 0;
        virtual void setSubData( const void * _data, size_t _length, const TextureRegion& _region ) = 0;
		//virtual void setSubData(const void * _data, size_t _length, const TextureRegion& _region, uint32_t _mipCount ) = 0;
        virtual void release() = 0;
    };

    class NIX_API_DECL IBuffer {
	protected:
		BufferType m_type;
    public:
		IBuffer(BufferType _type) : m_type(_type) {}
		//
        virtual size_t getSize() = 0;
		virtual void setData(const void * _data, size_t _size, size_t _offset) = 0;
		virtual void release() = 0;
		// don't re-implement this method
		virtual BufferType getType() final {
			return m_type;
		}
    };

    class NIX_API_DECL IAttachment {
    public:
        virtual const ITexture* getTexture() const = 0;
		virtual void release() = 0;
		virtual NixFormat getFormat() const = 0;
    };
    //

	enum RenderPassType {
		MainRenderPass,
		OffscreenRenderPass
	};

	class IGraphicsQueue {
	private:
	public:
		virtual void release() = 0;
	};

	class IArgument;
	class IRenderable;
	class IPipeline;
    class NIX_API_DECL IRenderPass {
    private:
    public:
        virtual bool begin( IGraphicsQueue* _queue = nullptr ) = 0;
        virtual void end() = 0;
		virtual void release() = 0;
		virtual void setClear( const RpClear& _clear ) = 0;
		virtual void resize(uint32_t _width, uint32_t _height) = 0;
		// -------------------------------------------------------------
		// binding
		// -------------------------------------------------------------
		virtual void bindPipeline(IPipeline* _pipeline) = 0;
		virtual void bindArgument(IArgument* _argument) = 0;
		// -------------------------------------------------------------
		// drawing ( renderable )
		// -------------------------------------------------------------
		virtual void draw(IRenderable* _renderable, uint32_t _vertexOffset, uint32_t _vertexCount) = 0;
		virtual void drawElements(IRenderable* _renderable, uint32_t _indexOffset, uint32_t _indexCount) = 0;
		virtual void drawInstanced(IRenderable* _renderable, uint32_t _vertexOffset, uint32_t _vertexCount, uint32_t _baseInstance, uint32_t _instanceCount) = 0;
		virtual void drawElementInstanced(IRenderable* _renderable, uint32_t _indexOffset, uint32_t _indexCount, uint32_t _baseInstance, uint32_t _instanceCount) = 0;
		// -------------------------------------------------------------
		// destroy
		// -------------------------------------------------------------
		virtual RenderPassType type() = 0;
    };

	typedef union {
		// opengl slot
		uint64_t oglSlot;
		// vulkan slot
		struct {
			uint32_t vkSet;
			uint32_t vkBinding;
		};
	} SamplerSlot ;

	typedef union {
		// opengl slot
		uint64_t oglSlot;
		// vulkan slot
		struct {
			uint16_t vkSet;
			uint16_t vkUBOIndex;
			uint16_t vkUBOOffset;
			uint16_t vkUBOSize; // unused : default 0
		};
	} UniformSlot;

	class NIX_API_DECL IArgument {
	private:
	public:
		virtual bool getUniformBlock( const std::string& _name, uint32_t* id_ ) = 0;
		virtual bool getUniformMemberOffset(uint32_t _uniform, const std::string& _name, uint32_t* offset_ ) = 0;
		virtual bool getSampler(const std::string& _name, uint32_t* id_ ) = 0;
		//
		virtual void setSampler( uint32_t _index, const SamplerState& _sampler, const ITexture* _texture) = 0;
		virtual void setUniform(uint32_t _index, uint32_t _offset, const void * _data, uint32_t _size) = 0;
		virtual void setShaderCache( uint32_t _offset, const void* _data, uint32_t _size ) = 0;
		virtual void release() = 0;
	};

	class NIX_API_DECL IRenderable {
	public:
		virtual uint32_t getVertexBufferCount() = 0;
		virtual TopologyMode getTopologyMode() = 0;
		virtual void setVertexBuffer( IBuffer* _buffer, size_t _offset, uint32_t _index ) = 0;
		virtual void setIndexBuffer( IBuffer* _buffer, size_t _offset ) = 0;
        virtual void release() = 0;
	};

	class NIX_API_DECL IPipeline {
	public:
        virtual void setViewport( const Viewport& _vp ) = 0;
        virtual void setScissor( const Scissor& _scissor ) = 0;
        virtual void setPolygonOffset(float _constantBias, float _slopeScaleBias) = 0;
        virtual void release() = 0;
	};

	class NIX_API_DECL IMaterial {
	public:
		virtual IArgument* createArgument( uint32_t _index ) = 0;
		virtual void destroyArgument(IArgument* _argument) = 0;
		virtual IRenderable* createRenderable() = 0;
		virtual void destroyRenderable(IRenderable* _renderable) = 0;
		//
        virtual IPipeline* createPipeline(const RenderPassDescription& _renderPass) = 0;
        virtual void release() = 0;
	};

	class IFrameCapture;
	typedef void(*FrameCaptureCallback) (IFrameCapture* _userData);
	class IFrameCapture {
	public:
		virtual void onCapture( void* _data, size_t _length ) = 0;
		virtual void* rawData() = 0;
		virtual size_t rawSize() = 0;
	};

	class ILogger {
	public:
		virtual void debug( const std::string& _text ) = 0;
		virtual void warning(const std::string& _text) = 0;
		virtual void error(const std::string& _text) = 0;
	};

	class IContext;
	//
	class NIX_API_DECL IDriver {
	public:
		virtual bool initialize( Nix::IArchieve* _arch, DeviceType _type) = 0;
		virtual IContext* createContext(void* _hwnd) = 0;
		virtual IArchieve* getArchieve() = 0;
		virtual ILogger* getLogger() = 0;
	};

    class NIX_API_DECL IContext {
	public:
		virtual IBuffer* createStaticVertexBuffer( const void* _data, size_t _size) = 0;
		virtual IBuffer* createCahcedVertexBuffer(size_t _size) = 0;
		virtual IBuffer* createIndexBuffer(const void* _data, size_t _size) = 0;
		//virtual IUniformBuffer* createUniformBuffer(size_t _size) = 0;
        virtual ITexture* createTexture(const TextureDescription& _desc, TextureUsageFlags _usage = TextureUsageNone ) = 0;
		virtual ITexture* createTextureDDS( const void* _data, size_t _length ) = 0;
		virtual ITexture* createTextureKTX(const void* _data, size_t _length) = 0;
		virtual IAttachment* createAttachment(NixFormat _format,uint32_t _width, uint32_t _height ) = 0;
        virtual IRenderPass* createRenderPass( const RenderPassDescription& _desc, IAttachment** _colorAttachments, IAttachment* _depthStencil ) = 0;
		virtual IMaterial* createMaterial( const MaterialDescription& _desc ) = 0;
        //virtual IPipeline* createPipeline( const MaterialDescription& _desc ) = 0;
		virtual NixFormat swapchainFormat() const = 0;
		virtual void captureFrame(IFrameCapture * _capture, FrameCaptureCallback _callback ) = 0;
		//
		virtual void resize(uint32_t _width, uint32_t _height) = 0;
		virtual bool beginFrame() = 0;
		virtual void endFrame() = 0;
		virtual IGraphicsQueue* getGraphicsQueue( uint32_t index = 0) = 0;
		virtual IRenderPass* getRenderPass() = 0;
		virtual IDriver* getDriver() = 0;
		virtual void release() = 0;
    };
 }

 extern "C" {
	 NIX_API_DECL Nix::IDriver* createDriver();
 }