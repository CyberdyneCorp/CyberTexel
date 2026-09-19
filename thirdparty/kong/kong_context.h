#ifndef CYBERTEXEL_KONG_CONTEXT_H
#define CYBERTEXEL_KONG_CONTEXT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kong_context kong_context;

kong_context *kong_context_create(void);
void          kong_context_destroy(kong_context *context);
bool          kong_context_compile_wgsl(kong_context *context, const char *source,
	                                     char **vertex_source, char **fragment_source);
const char   *kong_context_last_error(const kong_context *context);
void          kong_context_free_string(char *value);

#ifdef __cplusplus
}
#endif

#endif
