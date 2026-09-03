#ifndef ROMM_LIBRARY_H
#define ROMM_LIBRARY_H

#include <stddef.h>
#include <stdbool.h>

#define ROMM_LIBRARY_PAGE_SIZE 20
#define ROMM_LIBRARY_NAME_LENGTH 256
#define ROMM_LIBRARY_FILENAME_LENGTH 384
#define ROMM_LIBRARY_PLATFORM_LENGTH 128
#define ROMM_LIBRARY_MAX_PLATFORMS 128
#define ROMM_LIBRARY_MAX_PENDING 256

typedef struct romm_platform_entry
{
   long platform_id;
   char name[ROMM_LIBRARY_PLATFORM_LENGTH];
   char slug[ROMM_LIBRARY_PLATFORM_LENGTH];
} romm_platform_entry_t;

typedef struct romm_library_entry
{
   long rom_id;
   long platform_id;
   unsigned long long size_bytes;
   char name[ROMM_LIBRARY_NAME_LENGTH];
   char filename[ROMM_LIBRARY_FILENAME_LENGTH];
   bool local_present;
   bool selected;
} romm_library_entry_t;

void romm_library_clear(void);
void romm_library_clear_platforms(void);
void romm_library_set_loading(bool loading);
bool romm_library_is_loading(void);
void romm_library_set_error(const char *message);
const char *romm_library_get_error(void);

size_t romm_library_get_platforms(romm_platform_entry_t *out, size_t max_entries);
bool romm_library_parse_platforms_response(const char *json, size_t len);
const romm_platform_entry_t *romm_library_find_platform(long platform_id);

size_t romm_library_get_entries(romm_library_entry_t *out, size_t max_entries);
bool romm_library_parse_response(const char *json, size_t len,
      long platform_id, unsigned page, const char *roms_path);
romm_library_entry_t *romm_library_find_entry(long rom_id);

void romm_library_set_context(long platform_id, unsigned page);
long romm_library_get_platform_id(void);
unsigned romm_library_get_page(void);
unsigned long romm_library_get_total(void);
bool romm_library_has_previous(void);
bool romm_library_has_next(void);

/* Session-only queue. Local ROMs are always selected by filesystem state;
 * these functions track only missing ROMs explicitly queued for download. */
bool romm_library_pending_add(const romm_library_entry_t *entry);
bool romm_library_pending_remove(long rom_id);
bool romm_library_pending_contains(long rom_id);
size_t romm_library_pending_count(long platform_id);
size_t romm_library_pending_get(long platform_id,
      romm_library_entry_t *out, size_t max_entries);
void romm_library_pending_clear(long platform_id);
void romm_library_mark_local(long rom_id, bool local_present);

/* Local-delete confirmation state. */
bool romm_library_prepare_delete(long rom_id, const char *roms_path);
bool romm_library_execute_delete(void);
void romm_library_cancel_delete(void);
const char *romm_library_pending_delete_name(void);

#endif
