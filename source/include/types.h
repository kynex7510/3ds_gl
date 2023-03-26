/**
 * types.h
 * Contains all type related definitions.
 */
#ifndef _GLASS_TYPES_H
#define _GLASS_TYPES_H

#include "gl2.h"

#include "GLASS.h"

#include <stddef.h>
#include <stdint.h>

#define DECL_FLAG(id) (uint32_t)(1 << (id))

// Constant.
#define GLASS_INVALID_OBJECT 0
#define GLASS_NUM_ATTRIB_SLOTS 12
#define GLASS_NUM_ATTRIB_REGS 16
#define GLASS_NUM_COMBINER_STAGES 6

#define GLASS_UNI_BOOL 0x00
#define GLASS_UNI_INT 0x01
#define GLASS_UNI_FLOAT 0x02

#define GLASS_UNI_VERTEX 0x00
#define GLASS_UNI_GEOMETRY 0x01

// Object types.
#define GLASS_UNKNOWN_TYPE 0x00
#define GLASS_BUFFER_TYPE 0x01
#define GLASS_TEXTURE_TYPE 0x02
#define GLASS_PROGRAM_TYPE 0x03
#define GLASS_SHADER_TYPE 0x04
#define GLASS_FRAMEBUFFER_TYPE 0x05
#define GLASS_RENDERBUFFER_TYPE 0x06

// Type checking.
#define ObjectIsBuffer(x) CheckObjectType((x), GLASS_BUFFER_TYPE)
#define ObjectIsTexture(x) CheckObjectType((x), GLASS_TEXTURE_TYPE)
#define ObjectIsProgram(x) CheckObjectType((x), GLASS_PROGRAM_TYPE)
#define ObjectIsShader(x) CheckObjectType((x), GLASS_SHADER_TYPE)
#define ObjectIsFramebuffer(x) CheckObjectType((x), GLASS_FRAMEBUFFER_TYPE)
#define ObjectIsRenderbuffer(x) CheckObjectType((x), GLASS_RENDERBUFFER_TYPE)

// Buffer flags.
#define BUFFER_FLAG_BOUND DECL_FLAG(0)

// Represents a vertex buffer.
typedef struct {
  uint32_t type;    // GL type (GLASS_BUFFER_TYPE).
  uint8_t *address; // Data address.
  GLenum usage;     // Buffer usage type.
  uint16_t flags;   // Buffer flags.
} BufferInfo;

// Renderbuffer flags.
#define RENDERBUFFER_FLAG_BOUND DECL_FLAG(0)

// Represents a renderbuffer.
typedef struct {
  uint32_t type;    // GL type (GLASS_RENDERBUFFER_TYPE).
  uint8_t *address; // Data address.
  GLsizei width;    // Buffer width.
  GLsizei height;   // Buffer height.
  GLenum format;    // Buffer format.
  uint16_t flags;   // Renderbuffer flags.
} RenderbufferInfo;

// Framebuffer flags.
#define FRAMEBUFFER_FLAG_BOUND DECL_FLAG(0)

// Represents a framebuffer.
typedef struct {
  uint32_t type;                 // GL type (GLASS_FRAMEBUFFER_TYPE).
  RenderbufferInfo *colorBuffer; // Bound color buffer.
  RenderbufferInfo *depthBuffer; // Bound depth (+ stencil) buffer.
  uint32_t flags;                // Framebuffer flags.
} FramebufferInfo;

// Represents a vertex attribute.
typedef struct {
  GLenum type;           // Type of each component.
  GLint count;           // Num of components.
  GLsizei stride;        // Num of all bytes.
  GLuint boundBuffer;    // Bound array buffer.
  u32 physAddr;          // Physical address to buffer.
  GLfloat components[4]; // Fixed attrib X-Y-Z-W.
} AttributeInfo;

// Shader flags.
#define SHADER_FLAG_DELETE DECL_FLAG(0)
#define SHADER_FLAG_GEOMETRY DECL_FLAG(1)
#define SHADER_FLAG_MERGE_OUTMAPS DECL_FLAG(2)
#define SHADER_FLAG_USE_TEXCOORDS DECL_FLAG(3)

// Represents shared shader data.
typedef struct {
  uint32_t refc;           // Reference count.
  uint32_t *binaryCode;    // Binary code buffer.
  uint32_t numOfCodeWords; // Num of instructions.
  uint32_t *opDescs;       // Operand descriptors.
  uint32_t numOfOpDescs;   // Num of operand descriptors.
} SharedShaderData;

// Represents uniform informations.
typedef struct {
  uint8_t ID;      // Uniform ID.
  uint8_t type;    // Uniform type.
  size_t count;    // Number of elements.
  uint8_t *symbol; // Pointer to symbol.
  union {
    uint16_t mask;    // Bool, bool array mask.
    uint32_t value;   // Int value.
    uint32_t *values; // int array, float, float array data.
  } data;
  bool dirty; // Uniform dirty.
} UniformInfo;

// Represents a shader object.
typedef struct {
  uint32_t type;                // GL type (GLASS_SHADER_TYPE).
  SharedShaderData *sharedData; // Shared shader data.
  size_t codeEntrypoint;        // Code entrypoint.
  DVLE_geoShaderMode gsMode;    // Mode for geometry shader.
  uint16_t outMask;             // Used output registers mask.
  uint16_t outTotal;            // Total number of output registers.
  uint32_t outSems[7];          // Output register semantics.
  uint32_t outClock;            // Output register clock.
  uint8_t *symbolTable;         // This shader symbol table.
  uint32_t sizeOfSymbolTable;   // Size of symbol table.
  UniformInfo *activeUniforms;  // Active uniforms.
  uint32_t numOfUniforms;       // Num of active uniforms.
  uint16_t flags;               // Shader flags.
  uint16_t refc;                // Reference count.
} ShaderInfo;

// Program flags.
#define PROGRAM_FLAG_DELETE DECL_FLAG(0)
#define PROGRAM_FLAG_LINK_FAILED DECL_FLAG(1)
#define PROGRAM_FLAG_UPDATE_VERTEX DECL_FLAG(2)
#define PROGRAM_FLAG_UPDATE_GEOMETRY DECL_FLAG(3)

// Represents a shader program.
typedef struct {
  uint32_t type; // GL type (GLASS_PROGRAM_TYPE).
  // Attached = result of glAttachShader.
  // Linked = result of glLinkProgram.
  GLuint attachedVertex;   // Attached vertex shader.
  GLuint linkedVertex;     // Linked vertex shader.
  GLuint attachedGeometry; // Attached geometry shader.
  GLuint linkedGeometry;   // Linked geometry shader.
  uint32_t flags;          // Program flags.
} ProgramInfo;

// Represents a texture combiner.
typedef struct {
  GLenum rgbSrc[3];   // RGB source 0-1-2.
  GLenum alphaSrc[3]; // Alpha source 0-1-2.
  GLenum rgbOp[3];    // RGB operand 0-1-2.
  GLenum alphaOp[3];  // Alpha operand 0-1-2.
  GLenum rgbFunc;     // RGB function.
  GLenum alphaFunc;   // Alpha function.
  GLfloat rgbScale;   // RGB scale.
  GLfloat alphaScale; // Alpha scale.
  uint32_t color;     // Constant color.
} CombinerInfo;

// Context flags.
#define CONTEXT_FLAG_FRAMEBUFFER DECL_FLAG(0)
#define CONTEXT_FLAG_DRAW DECL_FLAG(1)
#define CONTEXT_FLAG_VIEWPORT DECL_FLAG(2)
#define CONTEXT_FLAG_SCISSOR DECL_FLAG(3)
#define CONTEXT_FLAG_PROGRAM DECL_FLAG(4)
#define CONTEXT_FLAG_COMBINERS DECL_FLAG(5)
#define CONTEXT_FLAG_VERTEX_UNIFORMS DECL_FLAG(6)
#define CONTEXT_FLAG_GEOMETRY_UNIFORMS DECL_FLAG(7)
#define CONTEXT_FLAG_ATTRIBS DECL_FLAG(8)
#define CONTEXT_FLAG_FRAGMENT DECL_FLAG(9)
#define CONTEXT_FLAG_DEPTHMAP DECL_FLAG(10)
#define CONTEXT_FLAG_COLOR_DEPTH DECL_FLAG(11)
#define CONTEXT_FLAG_EARLY_DEPTH DECL_FLAG(12)
#define CONTEXT_FLAG_EARLY_DEPTH_CLEAR DECL_FLAG(13)
#define CONTEXT_FLAG_STENCIL DECL_FLAG(14)
#define CONTEXT_FLAG_CULL_FACE DECL_FLAG(15)
#define CONTEXT_FLAG_ALPHA DECL_FLAG(16)
#define CONTEXT_FLAG_BLEND DECL_FLAG(17)

// Internal context.
typedef struct {
  // Parameters publicly exposed.
  gfxScreen_t targetScreen;        // Target screen for drawing (top or bottom).
  gfx3dSide_t targetSide;          // Target side for drawing (left or right).
  GX_TRANSFER_SCALE transferScale; // TODO: Anti-aliasing.

  // Platform.
  uint32_t flags;       // State flags.
  GLenum lastError;     // Actually first error.
  u32 *cmdBuffer;       // Buffer for GPU commands.
  u32 cmdBufferSize;    // Offset relative to buffer offset.
  u32 cmdBufferOffset;  // Offset relative to buffer base.
  gxCmdQueue_s gxQueue; // Queue for GX commands.

  // Buffers.
  GLuint arrayBuffer;        // GL_ARRAY_BUFFER
  GLuint elementArrayBuffer; // GL_ELEMENT_ARRAY_BUFFER

  // Framebuffer.
  GLuint framebuffer;   // Bound framebuffer object.
  GLuint renderbuffer;  // Bound renderbuffer object.
  uint32_t clearColor;  // Color buffer clear value.
  GLclampf clearDepth;  // Depth buffer clear value.
  uint8_t clearStencil; // Stencil buffer clear value.
  bool block32;         // Block mode 32.

  // Viewport.
  GLint viewportX;   // Viewport X.
  GLint viewportY;   // Viewport Y.
  GLsizei viewportW; // Viewport width.
  GLsizei viewportH; // Viewport height.

  // Scissor.
  GPU_SCISSORMODE scissorMode; // Scissor test mode.
  GLint scissorX;              // Scissor box X.
  GLint scissorY;              // Scissor box Y.
  GLsizei scissorW;            // Scissor box W.
  GLsizei scissorH;            // Scissor box H.

  // Attributes.
  AttributeInfo attribs[GLASS_NUM_ATTRIB_REGS]; // Attributes data.
  size_t attribSlots[GLASS_NUM_ATTRIB_SLOTS];   // Attributes slots.

  // Program.
  GLuint currentProgram; // Shader program in use.

  // Combiners.
  GLint combinerStage;                               // Current combiner stage.
  CombinerInfo combiners[GLASS_NUM_COMBINER_STAGES]; // Combiners.

  // Fragment.
  GLenum fragMode; // Fragment mode.
  bool blendMode;  // Blend mode.

  // Color and depth.
  bool writeRed;    // Write red.
  bool writeGreen;  // Write green.
  bool writeBlue;   // Write blue.
  bool writeAlpha;  // Write alpha.
  bool writeDepth;  // Write depth.
  bool depthTest;   // Depth test enabled.
  GLenum depthFunc; // Depth test function.

  // Depth map.
  GLclampf depthNear;    // Depth map near val.
  GLclampf depthFar;     // Depth map far val.
  bool polygonOffset;    // Depth map offset enabled.
  GLfloat polygonFactor; // Depth map factor (unused).
  GLfloat polygonUnits;  // Depth map offset units.

  // Early depth.
  bool earlyDepthTest;      // Early depth test enabled.
  GLclampf clearEarlyDepth; // Early depth clear value.
  GLenum earlyDepthFunc;    // Early depth function.

  // Stencil.
  bool stencilTest;        // Stencil test enabled.
  GLenum stencilFunc;      // Stencil function.
  GLint stencilRef;        // Stencil reference value.
  GLuint stencilMask;      // Stencil mask.
  GLuint stencilWriteMask; // Stencil write mask.
  GLenum stencilFail;      // Stencil fail.
  GLenum stencilDepthFail; // Stencil pass, depth fail.
  GLenum stencilPass;      // Stencil pass + depth pass or no depth.

  // Cull face.
  bool cullFace;        // Cull face enabled.
  GLenum cullFaceMode;  // Cull face mode.
  GLenum frontFaceMode; // Front face mode.

  // Alpha.
  bool alphaTest;    // Alpha test enabled.
  GLenum alphaFunc;  // Alpha test function.
  GLclampf alphaRef; // Alpha test reference value.

  // Blend.
  uint32_t blendColor;  // Blend color.
  GLenum blendEqRGB;    // Blend equation RGB.
  GLenum blendEqAlpha;  // Blend equation alpha.
  GLenum blendSrcRGB;   // Blend source RGB.
  GLenum blendDstRGB;   // Blend destination RGB.
  GLenum blendSrcAlpha; // Blend source alpha.
  GLenum blendDstAlpha; // Blend destination alpha.

  // Logic Op.
  GLenum logicOp; // Logic op.
} CtxImpl;

// Create a GL object.
#define CreateObject GLASS_types_createObject
GLuint GLASS_types_createObject(const uint32_t type);

// Check type of object.
#define CheckObjectType GLASS_types_checkObjectType
bool GLASS_types_checkObjectType(const GLuint obj, const uint32_t type);

#endif /* _GLASS_TYPES_H */