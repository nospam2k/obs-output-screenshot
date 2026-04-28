/*
obs-websocket-api.h — vendored header for obs-output-screenshot plugin
Source: https://github.com/obsproject/obs-websocket/blob/master/lib/obs-websocket-api.h
License: GPLv2
*/
#pragma once

#include <obs-module.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle returned by obs_websocket_register_vendor */
typedef struct obs_websocket_vendor obs_websocket_vendor_t;

/*
 * Register a named vendor. Call once in obs_module_load().
 * Returns NULL if obs-websocket is not loaded.
 */
EXPORT obs_websocket_vendor_t *
obs_websocket_register_vendor(const char *name);

/*
 * Register a request handler for the given request type.
 * request_data  — obs_data_t with the caller's payload
 * response_data — obs_data_t to populate before returning
 */
typedef void (*obs_websocket_request_callback_t)(obs_data_t *request_data,
                                                  obs_data_t *response_data,
                                                  void *priv_data);

EXPORT bool
obs_websocket_vendor_register_request(obs_websocket_vendor_t *vendor,
                                      const char *type,
                                      obs_websocket_request_callback_t cb,
                                      void *priv_data);

/*
 * Emit an event to all connected WebSocket clients.
 */
EXPORT bool
obs_websocket_vendor_emit_event(obs_websocket_vendor_t *vendor,
                                const char *event_type,
                                obs_data_t *event_data);

#ifdef __cplusplus
}
#endif
