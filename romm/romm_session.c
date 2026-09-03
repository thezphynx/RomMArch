#include <stdio.h>
#include <string.h>

#include <retro_miscellaneous.h>
#include <file/file_path.h>

#include "../core_info.h"
#include "romm_session.h"

#define ROMM_SESSION_PATH "sdmc:/retroarch/rommarch_session.txt"
#define ROMM_SAVE_ROOT    "sdmc:/retroarch/cores/savefiles"

void romm_session_record_launch(const char *core_path, const char *content_path)
{
   FILE *fp;
   core_info_t *core_info = NULL;
   char save_dir[PATH_MAX_LENGTH];
   char save_path[PATH_MAX_LENGTH];
   const char *core_name = "";

   if (core_path && *core_path)
   {
      if (core_info_find(core_path, &core_info)
            && core_info
            && core_info->core_name
            && *core_info->core_name)
         core_name = core_info->core_name;
   }

   save_dir[0]  = '\0';
   save_path[0] = '\0';

   if (*core_name && content_path && *content_path)
   {
      fill_pathname_join(save_dir, ROMM_SAVE_ROOT, core_name, sizeof(save_dir));
      strlcpy(save_path, content_path, sizeof(save_path));
      fill_pathname(save_path, save_path, ".srm", sizeof(save_path));

      {
         char save_name[PATH_MAX_LENGTH];
         strlcpy(save_name, path_basename(save_path), sizeof(save_name));
         fill_pathname_join(save_path, save_dir, save_name, sizeof(save_path));
      }
   }

   fp = fopen(ROMM_SESSION_PATH, "w");

   if (!fp)
      return;

   fprintf(fp, "core=%s\n", core_path ? core_path : "");
   fprintf(fp, "content=%s\n", content_path ? content_path : "");
   fprintf(fp, "core_name=%s\n", core_name);
   fprintf(fp, "save=%s\n", save_path);
   fprintf(fp, "pending=1\n");

   fclose(fp);
}

bool romm_session_has_pending(void)
{
   FILE *fp = fopen(ROMM_SESSION_PATH, "r");
   char line[128];

   if (!fp)
      return false;

   while (fgets(line, sizeof(line), fp))
   {
      if (!strncmp(line, "pending=1", 9))
      {
         fclose(fp);
         return true;
      }
   }

   fclose(fp);
   return false;
}

bool romm_session_load(romm_session_t *session)
{
   FILE *fp;
   char line[PATH_MAX_LENGTH + 32];

   if (!session)
      return false;

   memset(session, 0, sizeof(*session));

   fp = fopen(ROMM_SESSION_PATH, "r");
   if (!fp)
      return false;

   while (fgets(line, sizeof(line), fp))
   {
      char *value;
      size_t len = strlen(line);

      while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
         line[--len] = '\0';

      if (!strncmp(line, "core=", 5))
      {
         value = line + 5;
         strlcpy(session->core, value, sizeof(session->core));
      }
      else if (!strncmp(line, "content=", 8))
      {
         value = line + 8;
         strlcpy(session->content, value, sizeof(session->content));
      }
      else if (!strncmp(line, "core_name=", 10))
      {
         value = line + 10;
         strlcpy(session->core_name, value, sizeof(session->core_name));
      }
      else if (!strncmp(line, "save=", 5))
      {
         value = line + 5;
         strlcpy(session->save, value, sizeof(session->save));
      }
      else if (!strcmp(line, "pending=1"))
         session->pending = true;
      else if (!strcmp(line, "pending=0"))
         session->pending = false;
   }

   fclose(fp);
   return true;
}

bool romm_session_save_exists(const romm_session_t *session)
{
   if (!session || !*session->save)
      return false;

   return path_is_valid(session->save);
}
