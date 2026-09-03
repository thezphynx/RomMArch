#ifndef ROMM_SYNC_LIST_H
#define ROMM_SYNC_LIST_H

#include <boolean.h>
#include <retro_miscellaneous.h>

#define ROMM_SYNC_NAME_LENGTH 256
#define ROMM_SYNC_PLATFORM_LENGTH 64

typedef struct {
   long rom_id;
   char name[ROMM_SYNC_NAME_LENGTH];
   char platform[ROMM_SYNC_PLATFORM_LENGTH];
   char save_path[PATH_MAX_LENGTH];
   bool enabled;
} romm_sync_entry_t;

bool romm_sync_list_add(const romm_sync_entry_t *entry);
bool romm_sync_list_set_enabled(long rom_id, bool enabled);
bool romm_sync_list_remove(long rom_id);
bool romm_sync_list_get(long rom_id, romm_sync_entry_t *entry);
bool romm_sync_list_upsert(const romm_sync_entry_t *entry);
size_t romm_sync_list_load(romm_sync_entry_t *entries, size_t max_entries);

#endif
