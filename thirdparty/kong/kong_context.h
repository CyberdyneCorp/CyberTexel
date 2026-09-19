#ifndef CYBERTEXEL_KONG_CONTEXT_H
#define CYBERTEXEL_KONG_CONTEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kong_context kong_context;

typedef enum kong_target {
	KONG_TARGET_WGSL = 0,
	KONG_TARGET_MSL,
	KONG_TARGET_SPIRV,
	KONG_TARGET_HLSL,
	KONG_TARGET_COUNT
} kong_target;

typedef struct kong_compilation {
	char    *vertex_source;
	char    *fragment_source;
	char    *module_source;
	uint8_t *vertex_binary;
	size_t   vertex_binary_size;
	uint8_t *fragment_binary;
	size_t   fragment_binary_size;
} kong_compilation;

kong_context *kong_context_create(void);
void          kong_context_destroy(kong_context *context);
bool          kong_context_compile_wgsl(kong_context *context, const char *source,
	                                     char **vertex_source, char **fragment_source);
bool          kong_context_compile(kong_context *context, const char *source,
	                                kong_target target, kong_compilation *compilation);
const char   *kong_context_last_error(const kong_context *context);
const char   *kong_target_name(kong_target target);
void          kong_compilation_destroy(kong_compilation *compilation);
void          kong_context_free_string(char *value);

#ifdef __cplusplus
}
#endif

#endif
