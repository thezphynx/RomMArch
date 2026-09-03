#ifndef ROMM_SESSION_H
#define ROMM_SESSION_H
#include <boolean.h>
#include <retro_miscellaneous.h>

typedef struct {
   char core[PATH_MAX_LENGTH];
   char content[PATH_MAX_LENGTH];
   char core_name[256];
   char save[PATH_MAX_LENGTH];
   bool pending;
} romm_session_t;

bool romm_session_load(romm_session_t *session);
bool romm_session_save_exists(const romm_session_t *session);

void romm_session_record_launch(const char *core_path, const char *content_path);
bool romm_session_has_pending(void);

#endif
