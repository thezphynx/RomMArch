#ifndef ROMM_CONFIG_H
#define ROMM_CONFIG_H

#include <stddef.h>
#include <boolean.h>

bool romm_config_get_server_url(char *out, size_t out_size);
bool romm_config_set_server_url(const char *value);
bool romm_config_get_api_token(char *out, size_t out_size);
bool romm_config_set_api_token(const char *value);
bool romm_config_get_roms_path(char *out, size_t out_size);
bool romm_config_set_roms_path(const char *value);

bool romm_config_get_save_enabled(long platform_id);
bool romm_config_set_save_enabled(long platform_id, bool enabled);
bool romm_config_get_save_path(long platform_id, char *out, size_t out_size);
bool romm_config_set_save_path(long platform_id, const char *value);
bool romm_config_save_ready(long platform_id);
bool romm_config_get_device_id(char *device_id, size_t device_id_size);
bool romm_config_set_device_id(const char *device_id);
bool romm_config_get_save_sync_hash(long rom_id, char *out, size_t out_size);
bool romm_config_set_save_sync_hash(long rom_id, const char *hash);
bool romm_config_set_save_sync_server_meta(long rom_id, long save_id, const char *updated_at);
bool romm_config_cleanup_legacy_save_path(void);

void romm_config_set_save_platform_context(long platform_id, const char *name);
long romm_config_get_save_platform_id(void);
const char *romm_config_get_save_platform_name(void);

#endif
