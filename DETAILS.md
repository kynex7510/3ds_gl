# Details

## Caveats

- Fragment pipeline is not programmable.

- `glBind*` functions will not create buffers when passing arbitrary buffer values, instead `glGen*` must be used for buffer creation.

- No default framebuffer, nor default shader program, is provided.

- Unlike the OpenGL specs which limit each shader type to 1 entry, `glShaderBinary` supports as much shader entries as are contained in the shader binary, and are loaded in the order they're linked.

- In `glPolygonOffset` the `factor` argument has no effect.

- In `glVertexAttribPointer` the `normalized` argument must be set to `GL_FALSE`.

- The GPU has 16 input registers, but only 12 can be used at the same time for vertex attributes. `glEnableVertexAttribArray` will fail with `GL_OUT_OF_MEMORY` if all 12 slots are used; to use a different register call `glDisableVertexAttribArray` to disable one of the used registers first. 

- It is possible to use `glUniform` on uniform variables where the number of components is greater than that of the command. The other component values will be left unchanged.

## Extensions

### Texture combiners

```c
void glCombinerColorPICA(GLclampf red, GLclampf green, GLclampf blue,
                         GLclampf alpha);
void glCombinerFuncPICA(GLenum pname, GLenum func);
void glCombinerOpPICA(GLenum pname, GLenum op);
void glCombinerScalePICA(GLenum pname, GLfloat scale);
void glCombinerSrcPICA(GLenum pname, GLenum src);
void glCombinerStagePICA(GLint index);
```

### Fragment operation

```c
#define GL_FRAGOP_MODE_DEFAULT_PICA 0x6030
#define GL_FRAGOP_MODE_SHADOW_PICA 0x6048
#define GL_FRAGOP_MODE_GAS_PICA 0x6051
void glFragOpPICA(GLenum mode);
```

This function allows to control the fragment pipeline.

- `GL_FRAGOP_MODE_DEFAULT_PICA`: act like a normal, OpenGL compatible pipeline;

- `GL_FRAGOP_MODE_SHADOW_PICA`: ?

- `GL_FRAGOP_MODE_GAS_PICA`: ?

### Block mode

```c
#define GL_BLOCK8_PICA 0x6789
#define GL_BLOCK32_PICA 0x678A
void glBlockModePICA(GLenum mode);
```

This function changes how pixels are handled internally.

- `GL_BLOCK8_PICA`: ?

- `GL_BLOCK32_PICA`: ?

### Early depth tests

```c
#define GL_EARLY_DEPTH_TEST_PICA 0x6780
void glClearEarlyDepthPICA(GLclampf depth);
void glEarlyDepthFuncPICA(GLenum func);
```

The GPU is capable of running early depth tests. `glEnable`/`glDisable` can be used to enable or disable this feature. Possible functions: `GL_LESS`, `GL_LEQUAL`, `GL_GREATER`, `GL_GEQUAL`.

### Inverted scissor test

```c
#define GL_SCISSOR_TEST_INVERTED_PICA 0x6E00
```

This capability can be used to control inverted scissor test.

## Allocator

It is possible to manage allocations by defining the following functions:

```c
void *GLASS_virtualAlloc(const size_t size);
void GLASS_virtualFree(void *p);
void *GLASS_linearAlloc(const size_t size);
void GLASS_linearFree(void *p);
void *GLASS_vramAlloc(const size_t size, const vramAllocPos pos);
void GLASS_vramFree(void *p);
```

In debug mode, hooks are called after each allocation/before each deallocation:

```c
void GLASS_virtualAllocHook(const void *p, const size_t size);
void GLASS_virtualFreeHook(const void *p, const size_t size);
void GLASS_linearAllocHook(const void *p, const size_t size);
void GLASS_linearFreeHook(const void *p, const size_t size);
void GLASS_vramAllocHook(const void *p, const size_t size);
void GLASS_vramFreeHook(const void *p, const size_t size);
```