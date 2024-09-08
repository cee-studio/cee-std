#ifndef CEE_DBM_H
#define CEE_DBM_H
#include <stdbool.h>
#include <stdint.h>
#include "cee-sqlite3.h"
#include "cee-json.h"
/* the code to manage sqlite3 db for creation, restoration, migration, and population */

struct cee_dbm_path_info{
  char schema_script_path[128];
  char data_script_path[128];
  char db_file_path[128];
};

/* filename without path */
typedef char db_file_t[32];

struct cee_dbm_db_info{
  char kind[16];
  db_file_t file;
  bool recreate_db; /* */
  bool restore_db; /* */
  bool customize_db;
  int create_version;
  int final_version;
};

extern int cee_dbm_path_info_load(char *start, uintptr_t size, struct cee_dbm_path_info *info);
extern uintptr_t cee_dbm_path_info_snprint(char *buf, uintptr_t size, struct cee_dbm_path_info *s);

extern int cee_dbm_db_info_load(char *start, uintptr_t size, struct cee_dbm_db_info *info);
extern uintptr_t cee_dbm_settings_snprint(char *buf, uintptr_t size, struct cee_dbm_db_info *s);
extern void cee_dbm_path_info_normalize (struct cee_dbm_path_info *db, char *home_dir);

/* db_file has to exist, otherwise segfault will be caused */
extern void cee_dbm_open_db(struct cee_state *state, struct cee_sqlite3 * cs,
                            struct cee_dbm_path_info *path_info,
                            char *db_kind,
                            char *db_file,
                            sqlite3* (*open_db)(const char *db_file));

/* attatch db as <as_name> to support join tables in multiple dbs */
void cee_dbm_attach_db(struct cee_sqlite3 *cs,
                       struct cee_dbm_path_info *path_info,
                       char *db_file, char *as_name);

/* db_file does not have to exist */
extern bool cee_dbm_db_init(struct cee_dbm_path_info *path_info,
                            struct cee_dbm_db_info *s,
                            const char *db_file_,
                            const char *restore_data_prefix,
                            int *db_version);
extern bool cee_dbm_db_exist(struct cee_dbm_path_info *path_info, const char *db_file);


void cee_dbm_path_info2str(struct cee_dbm_path_info *info, size_t buf_size, char *buf);
void cee_dbm_db_info2str(struct cee_dbm_db_info *info, size_t buf_size, char *buf);

bool cee_dbm_db_backup(struct cee_dbm_path_info *path_info,
                       struct cee_dbm_db_info *s,
                       const char *db_file_,
                       const char *restore_data_prefix);
#endif //CEE_DBM_H
