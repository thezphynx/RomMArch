#include <stdio.h>
#include <string.h>

#include <string/stdstring.h>

#include "romm_config.h"

#define ROMM_CONFIG_PATH "sdmc:/retroarch/rommarch.cfg"
#define ROMM_CONFIG_TEMP "sdmc:/retroarch/rommarch.cfg.tmp"

static bool romm_config_get_value(const char *key, char *out, size_t out_size)
{
   FILE *fp;
   char line[1024];
   size_t key_len;

   if (!key || !out || out_size == 0)
      return false;

   out[0] = '\0';
   key_len = strlen(key);
   fp = fopen(ROMM_CONFIG_PATH, "r");
   if (!fp)
      return false;

   while (fgets(line, sizeof(line), fp))
   {
      size_t len = strlen(line);
      while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
         line[--len] = '\0';

      if (!strncmp(line, key, key_len) && line[key_len] == '=')
      {
         strlcpy(out, line + key_len + 1, out_size);
         fclose(fp);
         return *out != '\0';
      }
   }

   fclose(fp);
   return false;
}

static bool romm_config_set_value(const char *key, const char *value)
{
   FILE *src = NULL;
   FILE *dst;
   char line[1024];
   char clean[768];
   size_t key_len;
   size_t i = 0;
   bool replaced = false;

   if (!key || !value)
      return false;

   while (*value && i + 1 < sizeof(clean))
   {
      if (*value != '\n' && *value != '\r')
         clean[i++] = *value;
      value++;
   }
   clean[i] = '\0';
   key_len = strlen(key);

   src = fopen(ROMM_CONFIG_PATH, "r");
   dst = fopen(ROMM_CONFIG_TEMP, "w");
   if (!dst)
   {
      if (src)
         fclose(src);
      return false;
   }

   if (src)
   {
      while (fgets(line, sizeof(line), src))
      {
         if (!strncmp(line, key, key_len) && line[key_len] == '=')
         {
            if (!replaced)
            {
               if (fprintf(dst, "%s=%s\n", key, clean) < 0)
                  goto error;
               replaced = true;
            }
         }
         else if (fputs(line, dst) == EOF)
            goto error;
      }
      fclose(src);
      src = NULL;
   }

   if (!replaced && fprintf(dst, "%s=%s\n", key, clean) < 0)
      goto error;

   if (fclose(dst) != 0)
      return false;

   remove(ROMM_CONFIG_PATH);
   if (rename(ROMM_CONFIG_TEMP, ROMM_CONFIG_PATH) != 0)
   {
      remove(ROMM_CONFIG_TEMP);
      return false;
   }

   return true;

error:
   if (src)
      fclose(src);
   fclose(dst);
   remove(ROMM_CONFIG_TEMP);
   return false;
}

static bool romm_config_remove_value(const char *key)
{
   FILE *src = NULL;
   FILE *dst = NULL;
   char line[1024];
   size_t key_len;
   bool found = false;

   if (!key || !*key)
      return false;

   src = fopen(ROMM_CONFIG_PATH, "r");
   if (!src)
      return true;

   dst = fopen(ROMM_CONFIG_TEMP, "w");
   if (!dst)
   {
      fclose(src);
      return false;
   }

   key_len = strlen(key);
   while (fgets(line, sizeof(line), src))
   {
      if (!strncmp(line, key, key_len) && line[key_len] == '=')
      {
         found = true;
         continue;
      }
      if (fputs(line, dst) == EOF)
         goto error;
   }

   fclose(src);
   src = NULL;
   if (fclose(dst) != 0)
      return false;
   dst = NULL;

   if (!found)
   {
      remove(ROMM_CONFIG_TEMP);
      return true;
   }

   remove(ROMM_CONFIG_PATH);
   if (rename(ROMM_CONFIG_TEMP, ROMM_CONFIG_PATH) != 0)
   {
      remove(ROMM_CONFIG_TEMP);
      return false;
   }
   return true;

error:
   if (src) fclose(src);
   if (dst) fclose(dst);
   remove(ROMM_CONFIG_TEMP);
   return false;
}

bool romm_config_get_server_url(char *out, size_t out_size)
{ return romm_config_get_value("server_url", out, out_size); }
bool romm_config_set_server_url(const char *value)
{ return romm_config_set_value("server_url", value); }
bool romm_config_get_api_token(char *out, size_t out_size)
{ return romm_config_get_value("api_token", out, out_size); }
bool romm_config_set_api_token(const char *value)
{ return romm_config_set_value("api_token", value); }
bool romm_config_get_roms_path(char *out, size_t out_size)
{ return romm_config_get_value("roms_path", out, out_size); }
bool romm_config_set_roms_path(const char *value)
{ return romm_config_set_value("roms_path", value); }

bool romm_config_get_device_id(char *device_id, size_t device_id_size)
{
   if (!device_id || !device_id_size)
      return false;
   device_id[0] = '\0';
   return romm_config_get_value("device_id", device_id, device_id_size) && *device_id;
}

bool romm_config_set_device_id(const char *device_id)
{
   if (!device_id || !*device_id)
      return romm_config_remove_value("device_id");
   return romm_config_set_value("device_id", device_id);
}

bool romm_config_get_save_sync_hash(long rom_id, char *out, size_t out_size)
{
   char key[64];
   if (rom_id <= 0 || !out || !out_size)
      return false;
   snprintf(key, sizeof(key), "save_sync_%ld_hash", rom_id);
   return romm_config_get_value(key, out, out_size);
}

bool romm_config_set_save_sync_hash(long rom_id, const char *hash)
{
   char key[64];
   char current[65];
   if (rom_id <= 0 || !hash || !*hash)
      return false;
   snprintf(key, sizeof(key), "save_sync_%ld_hash", rom_id);
   if (romm_config_get_value(key, current, sizeof(current)) && string_is_equal_noncase(current, hash))
      return true;
   return romm_config_set_value(key, hash);
}

bool romm_config_set_save_sync_server_meta(long rom_id, long save_id, const char *updated_at)
{
   char key[64];
   char value[64];
   bool ok = true;
   if (rom_id <= 0 || save_id <= 0)
      return false;
   snprintf(key, sizeof(key), "save_sync_%ld_server_id", rom_id);
   snprintf(value, sizeof(value), "%ld", save_id);
   ok = romm_config_set_value(key, value) && ok;
   if (updated_at && *updated_at)
   {
      snprintf(key, sizeof(key), "save_sync_%ld_server_updated_at", rom_id);
      ok = romm_config_set_value(key, updated_at) && ok;
   }
   return ok;
}

bool romm_config_cleanup_legacy_save_path(void)
{
   bool ok1 = romm_config_remove_value("saves_path");
   bool ok2 = romm_config_remove_value("savefile_path");
   return ok1 && ok2;
}

static long g_save_platform_id = 0;
static char g_save_platform_name[128];

bool romm_config_get_save_enabled(long platform_id)
{
   char key[64];
   char value[32];
   if (platform_id <= 0)
      return false;
   snprintf(key, sizeof(key), "save_%ld_enabled", platform_id);
   if (!romm_config_get_value(key, value, sizeof(value)))
      return false;
   return string_is_equal(value, "1") ||
          string_is_equal_noncase(value, "true") ||
          string_is_equal_noncase(value, "yes") ||
          string_is_equal_noncase(value, "on");
}

bool romm_config_set_save_enabled(long platform_id, bool enabled)
{
   char key[64];
   if (platform_id <= 0)
      return false;
   snprintf(key, sizeof(key), "save_%ld_enabled", platform_id);
   return romm_config_set_value(key, enabled ? "1" : "0");
}

bool romm_config_get_save_path(long platform_id, char *out, size_t out_size)
{
   char key[64];
   if (platform_id <= 0 || !out || out_size == 0)
      return false;
   snprintf(key, sizeof(key), "save_%ld_path", platform_id);
   return romm_config_get_value(key, out, out_size);
}

bool romm_config_set_save_path(long platform_id, const char *value)
{
   char key[64];
   if (platform_id <= 0)
      return false;
   snprintf(key, sizeof(key), "save_%ld_path", platform_id);
   return romm_config_set_value(key, value ? value : "");
}

bool romm_config_save_ready(long platform_id)
{
   char path[768];
   return romm_config_get_save_enabled(platform_id) &&
          romm_config_get_save_path(platform_id, path, sizeof(path)) && *path;
}

void romm_config_set_save_platform_context(long platform_id, const char *name)
{
   g_save_platform_id = platform_id;
   strlcpy(g_save_platform_name, name ? name : "", sizeof(g_save_platform_name));
}

long romm_config_get_save_platform_id(void)
{
   return g_save_platform_id;
}

const char *romm_config_get_save_platform_name(void)
{
   return g_save_platform_name;
}
