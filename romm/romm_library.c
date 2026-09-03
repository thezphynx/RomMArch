#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <file/file_path.h>
#include <streams/file_stream.h>
#include "romm_library.h"

static romm_platform_entry_t g_platforms[ROMM_LIBRARY_MAX_PLATFORMS];
static size_t g_platform_count;
static romm_library_entry_t g_entries[ROMM_LIBRARY_PAGE_SIZE];
static size_t g_count;
static bool g_loading;
static char g_error[128];
static long g_platform_id;
static unsigned g_page;
static unsigned long g_total;
static romm_library_entry_t g_pending[ROMM_LIBRARY_MAX_PENDING];
static size_t g_pending_count;
static long g_delete_rom_id;
static char g_delete_path[1024];
static char g_delete_name[ROMM_LIBRARY_FILENAME_LENGTH];

void romm_library_clear(void)
{
   g_count = 0;
   g_total = 0;
   g_error[0] = '\0';
}

void romm_library_clear_platforms(void)
{
   g_platform_count = 0;
   g_error[0] = '\0';
}

void romm_library_set_loading(bool loading) { g_loading = loading; }
bool romm_library_is_loading(void) { return g_loading; }

void romm_library_set_error(const char *message)
{
   g_error[0] = '\0';
   if (message)
   {
      strncpy(g_error, message, sizeof(g_error) - 1);
      g_error[sizeof(g_error) - 1] = '\0';
   }
}

const char *romm_library_get_error(void) { return g_error; }

void romm_library_set_context(long platform_id, unsigned page)
{
   g_platform_id = platform_id;
   g_page = page;
}

long romm_library_get_platform_id(void) { return g_platform_id; }
unsigned romm_library_get_page(void) { return g_page; }
unsigned long romm_library_get_total(void) { return g_total; }
bool romm_library_has_previous(void) { return g_page > 0; }
bool romm_library_has_next(void)
{
   unsigned long offset = (unsigned long)g_page * ROMM_LIBRARY_PAGE_SIZE;
   if (g_total)
      return offset + g_count < g_total;
   return g_count == ROMM_LIBRARY_PAGE_SIZE;
}

size_t romm_library_get_platforms(romm_platform_entry_t *out, size_t max_entries)
{
   size_t n = g_platform_count < max_entries ? g_platform_count : max_entries;
   if (out && n)
      memcpy(out, g_platforms, n * sizeof(*out));
   return n;
}

const romm_platform_entry_t *romm_library_find_platform(long platform_id)
{
   size_t i;
   for (i = 0; i < g_platform_count; i++)
      if (g_platforms[i].platform_id == platform_id)
         return &g_platforms[i];
   return NULL;
}

size_t romm_library_get_entries(romm_library_entry_t *out, size_t max_entries)
{
   size_t n = g_count < max_entries ? g_count : max_entries;
   if (out && n)
      memcpy(out, g_entries, n * sizeof(*out));
   return n;
}

romm_library_entry_t *romm_library_find_entry(long rom_id)
{
   size_t i;
   for (i = 0; i < g_count; i++)
      if (g_entries[i].rom_id == rom_id)
         return &g_entries[i];
   return NULL;
}

bool romm_library_pending_contains(long rom_id)
{
   size_t i;
   for (i = 0; i < g_pending_count; i++)
      if (g_pending[i].rom_id == rom_id)
         return true;
   return false;
}

bool romm_library_pending_add(const romm_library_entry_t *entry)
{
   if (!entry || entry->rom_id <= 0 || !*entry->filename)
      return false;
   if (romm_library_pending_contains(entry->rom_id))
      return true;
   if (g_pending_count >= ROMM_LIBRARY_MAX_PENDING)
      return false;
   g_pending[g_pending_count++] = *entry;
   return true;
}

bool romm_library_pending_remove(long rom_id)
{
   size_t i;
   for (i = 0; i < g_pending_count; i++)
   {
      if (g_pending[i].rom_id == rom_id)
      {
         if (i + 1 < g_pending_count)
            memmove(&g_pending[i], &g_pending[i + 1],
                  (g_pending_count - i - 1) * sizeof(g_pending[0]));
         g_pending_count--;
         return true;
      }
   }
   return false;
}

size_t romm_library_pending_count(long platform_id)
{
   size_t i, count = 0;
   for (i = 0; i < g_pending_count; i++)
      if (platform_id <= 0 || g_pending[i].platform_id == platform_id)
         count++;
   return count;
}

size_t romm_library_pending_get(long platform_id,
      romm_library_entry_t *out, size_t max_entries)
{
   size_t i, count = 0;
   if (!out || !max_entries)
      return 0;
   for (i = 0; i < g_pending_count && count < max_entries; i++)
      if (platform_id <= 0 || g_pending[i].platform_id == platform_id)
         out[count++] = g_pending[i];
   return count;
}

void romm_library_pending_clear(long platform_id)
{
   size_t i = 0;
   while (i < g_pending_count)
   {
      if (platform_id <= 0 || g_pending[i].platform_id == platform_id)
      {
         if (i + 1 < g_pending_count)
            memmove(&g_pending[i], &g_pending[i + 1],
                  (g_pending_count - i - 1) * sizeof(g_pending[0]));
         g_pending_count--;
      }
      else
         i++;
   }
}

void romm_library_mark_local(long rom_id, bool local_present)
{
   romm_library_entry_t *entry = romm_library_find_entry(rom_id);
   if (entry)
   {
      entry->local_present = local_present;
      entry->selected      = local_present || romm_library_pending_contains(rom_id);
   }
   if (local_present)
      romm_library_pending_remove(rom_id);
}

bool romm_library_prepare_delete(long rom_id, const char *roms_path)
{
   romm_library_entry_t *entry = romm_library_find_entry(rom_id);
   size_t len;
   if (!entry || !entry->local_present || !roms_path || !*roms_path || !*entry->filename)
      return false;
   g_delete_rom_id = rom_id;
   strlcpy(g_delete_path, roms_path, sizeof(g_delete_path));
   len = strlen(g_delete_path);
   if (len && g_delete_path[len - 1] != '/' && g_delete_path[len - 1] != '\\')
      strlcat(g_delete_path, "/", sizeof(g_delete_path));
   strlcat(g_delete_path, entry->filename, sizeof(g_delete_path));
   strlcpy(g_delete_name, entry->filename, sizeof(g_delete_name));
   return true;
}

bool romm_library_execute_delete(void)
{
   long rom_id = g_delete_rom_id;
   if (rom_id <= 0 || !*g_delete_path)
      return false;
   if (filestream_delete(g_delete_path) != 0)
      return false;
   romm_library_pending_remove(rom_id);
   romm_library_mark_local(rom_id, false);
   g_delete_rom_id = 0;
   g_delete_path[0] = '\0';
   g_delete_name[0] = '\0';
   return true;
}

void romm_library_cancel_delete(void)
{
   g_delete_rom_id = 0;
   g_delete_path[0] = '\0';
   g_delete_name[0] = '\0';
}

const char *romm_library_pending_delete_name(void)
{
   return g_delete_name;
}

static const char *skip_ws(const char *p, const char *end)
{
   while (p < end && isspace((unsigned char)*p)) p++;
   return p;
}

static const char *find_key(const char *start, const char *end, const char *key)
{
   size_t klen = strlen(key);
   const char *p;
   for (p = start; p + klen + 2 <= end; p++)
      if (*p == '"' && (size_t)(end - p) >= klen + 2 &&
            !memcmp(p + 1, key, klen) && p[1 + klen] == '"')
         return p + klen + 2;
   return NULL;
}

static bool extract_long(const char *start, const char *end, const char *key, long *value)
{
   const char *p = find_key(start, end, key);
   char *after;
   if (!p) return false;
   p = skip_ws(p, end);
   if (p >= end || *p != ':') return false;
   p = skip_ws(p + 1, end);
   if (p >= end) return false;
   *value = strtol(p, &after, 10);
   return after != p;
}

static bool extract_ull(const char *start, const char *end, const char *key,
      unsigned long long *value)
{
   const char *p = find_key(start, end, key);
   char *after;
   if (!p) return false;
   p = skip_ws(p, end);
   if (p >= end || *p != ':') return false;
   p = skip_ws(p + 1, end);
   if (p >= end) return false;
   *value = strtoull(p, &after, 10);
   return after != p;
}

static bool extract_string(const char *start, const char *end, const char *key,
      char *out, size_t out_size)
{
   const char *p = find_key(start, end, key);
   size_t n = 0;
   if (!p || !out || !out_size) return false;
   p = skip_ws(p, end);
   if (p >= end || *p != ':') return false;
   p = skip_ws(p + 1, end);
   if (p >= end || *p != '"') return false;
   p++;
   while (p < end && *p != '"')
   {
      char c = *p++;
      if (c == '\\' && p < end)
      {
         c = *p++;
         switch (c)
         {
            case 'n': c = ' '; break;
            case 'r': c = ' '; break;
            case 't': c = ' '; break;
            case '"': break;
            case '\\': break;
            default: break;
         }
      }
      if (n + 1 < out_size)
         out[n++] = c;
   }
   out[n] = '\0';
   return n > 0;
}

static const char *find_array(const char *json, const char *end, const char *key)
{
   const char *p = find_key(json, end, key);
   if (!p) return NULL;
   p = skip_ws(p, end);
   if (p < end && *p == ':') p = skip_ws(p + 1, end);
   return (p < end && *p == '[') ? p : NULL;
}

static bool next_object(const char **cursor, const char *end,
      const char **obj_start, const char **obj_end)
{
   const char *p = *cursor;
   int depth;
   bool in_string = false, escape = false;

   *obj_start = NULL;
   *obj_end = NULL;

   for (; p < end; p++)
   {
      char c = *p;
      if (in_string)
      {
         if (escape) escape = false;
         else if (c == '\\') escape = true;
         else if (c == '"') in_string = false;
         continue;
      }
      if (c == '"') { in_string = true; continue; }
      if (c == ']') { *cursor = p; return false; }
      if (c == '{') { *obj_start = p; p++; break; }
   }
   if (!*obj_start) { *cursor = p; return false; }

   depth = 1;
   in_string = false;
   escape = false;
   for (; p < end; p++)
   {
      char c = *p;
      if (in_string)
      {
         if (escape) escape = false;
         else if (c == '\\') escape = true;
         else if (c == '"') in_string = false;
         continue;
      }
      if (c == '"') { in_string = true; continue; }
      if (c == '{') depth++;
      else if (c == '}' && --depth == 0)
      {
         *obj_end = p + 1;
         *cursor = p + 1;
         return true;
      }
   }
   *cursor = p;
   return false;
}

bool romm_library_parse_platforms_response(const char *json, size_t len)
{
   const char *p, *end, *array_start;
   g_platform_count = 0;
   if (!json || !len) return false;
   end = json + len;

   array_start = find_array(json, end, "items");
   if (!array_start)
   {
      p = skip_ws(json, end);
      if (p < end && *p == '[') array_start = p;
   }
   if (!array_start) return false;

   p = array_start + 1;
   while (g_platform_count < ROMM_LIBRARY_MAX_PLATFORMS)
   {
      const char *obj_start, *obj_end;
      romm_platform_entry_t *entry;
      if (!next_object(&p, end, &obj_start, &obj_end)) break;
      entry = &g_platforms[g_platform_count];
      memset(entry, 0, sizeof(*entry));
      if (!extract_long(obj_start, obj_end, "id", &entry->platform_id))
         continue;
      if (!extract_string(obj_start, obj_end, "name", entry->name, sizeof(entry->name)))
      {
         if (!extract_string(obj_start, obj_end, "display_name", entry->name, sizeof(entry->name)))
            continue;
      }
      if (!extract_string(obj_start, obj_end, "fs_slug", entry->slug, sizeof(entry->slug)))
         extract_string(obj_start, obj_end, "slug", entry->slug, sizeof(entry->slug));
      g_platform_count++;
   }
   return g_platform_count > 0;
}

static void filename_without_extension(char *out, size_t out_size, const char *filename)
{
   const char *base = filename;
   const char *slash;
   char *dot;
   if (!out || !out_size) return;
   out[0] = '\0';
   if (!filename || !*filename) return;
   slash = strrchr(filename, '/');
   if (slash) base = slash + 1;
   strlcpy(out, base, out_size);
   dot = strrchr(out, '.');
   if (dot && dot != out) *dot = '\0';
}

static bool local_file_present(const char *roms_path, const char *filename)
{
   char local_path[1024];
   size_t len;
   if (!roms_path || !*roms_path || !filename || !*filename)
      return false;
   strlcpy(local_path, roms_path, sizeof(local_path));
   len = strlen(local_path);
   if (len && local_path[len - 1] != '/' && local_path[len - 1] != '\\')
      strlcat(local_path, "/", sizeof(local_path));
   strlcat(local_path, filename, sizeof(local_path));
   return path_is_valid(local_path);
}

bool romm_library_parse_response(const char *json, size_t len,
      long platform_id, unsigned page, const char *roms_path)
{
   const char *p, *end, *array_start;
   long total = 0;

   romm_library_clear();
   romm_library_set_context(platform_id, page);
   if (!json || !len) return false;
   end = json + len;

   if (extract_long(json, end, "total", &total) && total > 0)
      g_total = (unsigned long)total;

   array_start = find_array(json, end, "items");
   if (!array_start)
   {
      p = skip_ws(json, end);
      if (p < end && *p == '[') array_start = p;
   }
   if (!array_start) return false;

   p = array_start + 1;
   while (g_count < ROMM_LIBRARY_PAGE_SIZE)
   {
      const char *obj_start, *obj_end;
      romm_library_entry_t *entry;
      char no_ext[ROMM_LIBRARY_FILENAME_LENGTH];

      if (!next_object(&p, end, &obj_start, &obj_end)) break;
      entry = &g_entries[g_count];
      memset(entry, 0, sizeof(*entry));
      entry->platform_id = platform_id;

      if (!extract_long(obj_start, obj_end, "id", &entry->rom_id) ||
            !extract_string(obj_start, obj_end, "name", entry->name, sizeof(entry->name)))
         continue;

      if (!extract_string(obj_start, obj_end, "fs_name", entry->filename, sizeof(entry->filename)))
         extract_string(obj_start, obj_end, "file_name", entry->filename, sizeof(entry->filename));

      if (!extract_ull(obj_start, obj_end, "fs_size_bytes", &entry->size_bytes))
         extract_ull(obj_start, obj_end, "file_size_bytes", &entry->size_bytes);

      if (*entry->filename)
      {
         filename_without_extension(no_ext, sizeof(no_ext), entry->filename);
         if (*no_ext)
            strlcpy(entry->name, no_ext, sizeof(entry->name));
      }

      entry->local_present = local_file_present(roms_path, entry->filename);
      entry->selected = entry->local_present || romm_library_pending_contains(entry->rom_id);
      if (entry->local_present)
         romm_library_pending_remove(entry->rom_id);
      g_count++;
   }
   return true;
}
