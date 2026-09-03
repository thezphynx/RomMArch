#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "romm_sync_list.h"

#define ROMM_SYNC_LIST_PATH "sdmc:/retroarch/rommarch_sync_list.txt"
#define ROMM_SYNC_LIST_TEMP "sdmc:/retroarch/rommarch_sync_list.tmp"

static bool romm_sync_list_write_entry(FILE *fp, const romm_sync_entry_t *entry)
{
   if (!fp || !entry)
      return false;

   return fprintf(fp, "%ld\t%d\t%s\t%s\t%s\n",
         entry->rom_id,
         entry->enabled ? 1 : 0,
         entry->platform,
         entry->name,
         entry->save_path) >= 0;
}

bool romm_sync_list_add(const romm_sync_entry_t *entry)
{
   FILE *fp;

   if (!entry || entry->rom_id <= 0 || !*entry->name)
      return false;

   fp = fopen(ROMM_SYNC_LIST_PATH, "a");
   if (!fp)
      return false;

   if (!romm_sync_list_write_entry(fp, entry))
   {
      fclose(fp);
      return false;
   }

   fclose(fp);
   return true;
}

bool romm_sync_list_set_enabled(long rom_id, bool enabled)
{
   FILE *src;
   FILE *dst;
   char line[PATH_MAX_LENGTH + ROMM_SYNC_NAME_LENGTH + 128];
   bool found = false;

   src = fopen(ROMM_SYNC_LIST_PATH, "r");
   if (!src)
      return false;

   dst = fopen(ROMM_SYNC_LIST_TEMP, "w");
   if (!dst)
   {
      fclose(src);
      return false;
   }

   while (fgets(line, sizeof(line), src))
   {
      long current_id;
      int current_enabled;
      char platform[ROMM_SYNC_PLATFORM_LENGTH];
      char name[ROMM_SYNC_NAME_LENGTH];
      char save_path[PATH_MAX_LENGTH];

      platform[0] = '\0';
      name[0] = '\0';
      save_path[0] = '\0';

      if (sscanf(line, "%ld\t%d\t%63[^\t]\t%255[^\t]\t%511[^\n]",
               &current_id,
               &current_enabled,
               platform,
               name,
               save_path) == 5)
      {
         if (current_id == rom_id)
         {
            current_enabled = enabled ? 1 : 0;
            found = true;
         }

         fprintf(dst, "%ld\t%d\t%s\t%s\t%s\n",
               current_id,
               current_enabled,
               platform,
               name,
               save_path);
      }
      else
         fputs(line, dst);
   }

   fclose(src);
   fclose(dst);

   if (!found)
   {
      remove(ROMM_SYNC_LIST_TEMP);
      return false;
   }

   if (remove(ROMM_SYNC_LIST_PATH) != 0)
   {
      remove(ROMM_SYNC_LIST_TEMP);
      return false;
   }

   if (rename(ROMM_SYNC_LIST_TEMP, ROMM_SYNC_LIST_PATH) != 0)
      return false;

   return true;
}

bool romm_sync_list_remove(long rom_id)
{
   FILE *src;
   FILE *dst;
   char line[PATH_MAX_LENGTH + ROMM_SYNC_NAME_LENGTH + 128];
   bool found = false;

   src = fopen(ROMM_SYNC_LIST_PATH, "r");
   if (!src)
      return false;

   dst = fopen(ROMM_SYNC_LIST_TEMP, "w");
   if (!dst)
   {
      fclose(src);
      return false;
   }

   while (fgets(line, sizeof(line), src))
   {
      long current_id;

      if (sscanf(line, "%ld\t", &current_id) == 1 && current_id == rom_id)
      {
         found = true;
         continue;
      }

      fputs(line, dst);
   }

   fclose(src);
   fclose(dst);

   if (!found)
   {
      remove(ROMM_SYNC_LIST_TEMP);
      return false;
   }

   if (remove(ROMM_SYNC_LIST_PATH) != 0)
   {
      remove(ROMM_SYNC_LIST_TEMP);
      return false;
   }

   if (rename(ROMM_SYNC_LIST_TEMP, ROMM_SYNC_LIST_PATH) != 0)
      return false;

   return true;
}


bool romm_sync_list_get(long rom_id, romm_sync_entry_t *entry)
{
   FILE *fp;
   char line[PATH_MAX_LENGTH + ROMM_SYNC_NAME_LENGTH + 128];

   if (rom_id <= 0 || !entry)
      return false;

   fp = fopen(ROMM_SYNC_LIST_PATH, "r");
   if (!fp)
      return false;

   while (fgets(line, sizeof(line), fp))
   {
      long current_id;
      int enabled;
      char platform[ROMM_SYNC_PLATFORM_LENGTH];
      char name[ROMM_SYNC_NAME_LENGTH];
      char save_path[PATH_MAX_LENGTH];
      int matched;

      platform[0] = '\0';
      name[0] = '\0';
      save_path[0] = '\0';

      matched = sscanf(line, "%ld\t%d\t%63[^\t]\t%255[^\t]\t%511[^\n]",
            &current_id, &enabled, platform, name, save_path);
      if (matched < 4 || current_id != rom_id)
         continue;

      entry->rom_id = current_id;
      entry->enabled = enabled ? true : false;
      strlcpy(entry->platform, platform, sizeof(entry->platform));
      strlcpy(entry->name, name, sizeof(entry->name));
      if (matched >= 5)
         strlcpy(entry->save_path, save_path, sizeof(entry->save_path));
      else
         entry->save_path[0] = '\0';
      fclose(fp);
      return true;
   }

   fclose(fp);
   return false;
}

bool romm_sync_list_upsert(const romm_sync_entry_t *entry)
{
   FILE *src;
   FILE *dst;
   char line[PATH_MAX_LENGTH + ROMM_SYNC_NAME_LENGTH + 128];
   bool found = false;

   if (!entry || entry->rom_id <= 0 || !*entry->name)
      return false;

   src = fopen(ROMM_SYNC_LIST_PATH, "r");
   if (!src)
      return romm_sync_list_add(entry);

   dst = fopen(ROMM_SYNC_LIST_TEMP, "w");
   if (!dst)
   {
      fclose(src);
      return false;
   }

   while (fgets(line, sizeof(line), src))
   {
      long current_id;
      if (sscanf(line, "%ld\t", &current_id) == 1 && current_id == entry->rom_id)
      {
         if (!found)
            romm_sync_list_write_entry(dst, entry);
         found = true;
         continue;
      }
      fputs(line, dst);
   }

   if (!found)
      romm_sync_list_write_entry(dst, entry);

   fclose(src);
   fclose(dst);

   if (remove(ROMM_SYNC_LIST_PATH) != 0)
   {
      remove(ROMM_SYNC_LIST_TEMP);
      return false;
   }
   return rename(ROMM_SYNC_LIST_TEMP, ROMM_SYNC_LIST_PATH) == 0;
}

size_t romm_sync_list_load(romm_sync_entry_t *entries, size_t max_entries)
{
   FILE *fp;
   char line[PATH_MAX_LENGTH + ROMM_SYNC_NAME_LENGTH + 128];
   size_t count = 0;

   if (!entries || max_entries == 0)
      return 0;

   fp = fopen(ROMM_SYNC_LIST_PATH, "r");
   if (!fp)
      return 0;

   while (count < max_entries && fgets(line, sizeof(line), fp))
   {
      long rom_id;
      int enabled;
      char platform[ROMM_SYNC_PLATFORM_LENGTH];
      char name[ROMM_SYNC_NAME_LENGTH];
      char save_path[PATH_MAX_LENGTH];

      platform[0] = '\0';
      name[0] = '\0';
      save_path[0] = '\0';

      {
         int matched = sscanf(line, "%ld\t%d\t%63[^\t]\t%255[^\t]\t%511[^\n]",
               &rom_id, &enabled, platform, name, save_path);
         if (matched < 4)
            continue;
      }

      entries[count].rom_id = rom_id;
      entries[count].enabled = enabled ? true : false;
      strlcpy(entries[count].platform, platform, sizeof(entries[count].platform));
      strlcpy(entries[count].name, name, sizeof(entries[count].name));
      strlcpy(entries[count].save_path, save_path, sizeof(entries[count].save_path));
      count++;
   }

   fclose(fp);
   return count;
}
