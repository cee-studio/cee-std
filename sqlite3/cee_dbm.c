#define _BSD_SOURCE
#define _GNU_SOURCE
#include <stdlib.h>
#include "cee_dbm.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "cee-utils.h"
#include "cee-sqlite3.h"
#include "cee-json.h"
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <libgen.h>
#include <time.h>

/*
 *
 * the same as "mkdir -p <path>"
 * the created directory permission is (mode & ~umask & 0777)
 *
 */
int mkdir_p_ext(const char * dir, mode_t mode, int *dir_p) {
  struct stat sb;

  if (!dir) {
    errno = EINVAL;
    return 1;
  }

  if (!stat(dir, &sb))
    return 0;

  mkdir_p_ext(dirname(strdupa(dir)), mode, NULL);
  int ret = mkdir(dir, mode);
  if( ret < 0 ){
    perror(dir);
    cee_segfault();
  }
  if( ret != -1 && dir_p ){
    *dir_p = open(dir, O_PATH);
  }
  return ret;
}

/*
 *
 * the same as "mkdir -p <path>"
 * the created directory permission is (mode & ~umask & 0777)
 *
 */
int mkdir_p(const char * dir, mode_t mode) {
  return mkdir_p_ext(dir, mode, NULL);
}

#if 0
int cee_dbm_path_info_load(char *start, size_t size, struct cee_dbm_path_info *info){
  json_extract(start, size,
               "(db_file_path):s"
               "(data_script_path):s"
               "(schema_script_path):s",
               info->db_file_path,
               info->data_script_path,
               info->schema_script_path);
  return 1;
}

int cee_dbm_db_info_load(char *start, size_t size, struct cee_dbm_db_info *info){
  json_extract(start, size,
               "(db_kind):s"
               "(create_version):d"
               "(recreate_db):b"
               "(restore_db):b"
               "(customize_db):b",
               info->kind,
               &info->create_version,
               &info->recreate_db,
               &info->restore_db,
               &info->customize_db);
  snprintf(info->file, sizeof info->file, "%s.db", info->kind);
  return 1;
}
#endif

size_t cee_dbm_path_info_snprint(char *buf, size_t size, struct cee_dbm_path_info *s){
  size_t offset = 0;
  offset += snprintf(buf+offset, size-offset, "schema_script_path:%s, ", s->schema_script_path);
  offset += snprintf(buf+offset, size-offset, "data_script_path:%s, ", s->data_script_path);
  offset += snprintf(buf+offset, size-offset, "db_file_path:%s, ", s->db_file_path);
  return offset;
}

size_t cee_dbm_settings_snprint(char *buf, size_t size, struct cee_dbm_db_info *s){
  size_t offset = 0;
  offset += snprintf(buf, size, "db_kind:%s, ", s->kind);
  offset += snprintf(buf+offset, size-offset, "create_version:%d, ", s->create_version);
  offset += snprintf(buf+offset, size-offset, "recreate_db:%s, ", s->recreate_db ? "true":"false");
  offset += snprintf(buf+offset, size-offset, "restore_db:%s, ", s->restore_db ? "true":"false");
  return offset;
}

static void replace_home_env(char path[128], char *home_dir){
  char buf[128];
  if( strstr(path, "$HOME") ){
    int n = snprintf(buf, sizeof buf, "%s%s", home_dir, path + strlen("$HOME"));
    memcpy(path, buf, n+1);
  }
}

void cee_dbm_path_info_normalize (struct cee_dbm_path_info *db, char *home_dir){
  replace_home_env(db->schema_script_path, home_dir);
  replace_home_env(db->data_script_path, home_dir);
  replace_home_env(db->db_file_path, home_dir);
}

void cee_dbm_open_db(struct cee_state *state, struct cee_sqlite3 *cs,
                     struct cee_dbm_path_info *path_info,
                     char *db_kind, char *db_file,
                     sqlite3* (*open_db)(const char *db_file)){
  char file[128];
  snprintf(file, sizeof file, "%s/%s", path_info->db_file_path, db_file);
  if( access(file, F_OK) != 0 ){
    fprintf(stderr, "db file %s does not exist\n", file);
    cee_segfault();
  }
  cee_sqlite3_init(cs, db_kind, file, state, open_db(file));
}

void cee_dbm_attach_db(struct cee_sqlite3 *cs,
                       struct cee_dbm_path_info *path_info,
                       char *db_file, char *as_name){
  char file[256];
  snprintf(file, sizeof file, "%s/%s", path_info->db_file_path, db_file);
  if( access(file, F_OK) != 0 ){
    fprintf(stderr, "db file %s does not exist\n", file);
    cee_segfault();
  }

  snprintf(file, sizeof file, "attach 'file:%s/%s?mode=ro' as %s;",
           path_info->db_file_path, db_file, as_name);

  char *err_msg=NULL;
  int rc = sqlite3_exec(cs->db, file, NULL, NULL, &err_msg);
  if( rc != SQLITE_OK ){
    fprintf(stderr, "SQL error: %s\n", err_msg);
    cee_segfault();
  }
}

static int get_user_version(const char *db_file){
  sqlite3 *db = NULL;
  sqlite3_open(db_file, &db);
  int current_version = cee_sqlite3_get_pragma_variable(db, "user_version");
  sqlite3_close(db);
  return current_version;
}


void open_dbm_log(struct cee_dbm_path_info *path_info, sqlite3 **db){
  char db_file[128];
  snprintf(db_file, sizeof db_file, "%s/cee_dbm_log.db", path_info->db_file_path);
  cee_sqlite3_init_db(db, db_file,
                      "create table if not exists log("
                      "kind TEXT NOT NULL,"
                      "action TEXT NOT NULL,"
                      "info TEXT NOT NULL,"
                      "created DATETIME DEFAULT CURRENT_TIMESTAMP);",
                      true);
  return;
}

void add_log(struct cee_sqlite3 *cs, char *kind, char *action, char *row){
  static struct cee_sqlite3_bind_info info[] = {
    {.var_name = "@kind", .col_name = "kind", .type = CEE_SQLITE3_TEXT },
    {.var_name = "@action", .col_name = "action", .type = CEE_SQLITE3_TEXT },
    {.var_name = "@info", .col_name = "info", .type = CEE_SQLITE3_TEXT },
    {.var_name = NULL, }
  };
  struct cee_sqlite3_bind_data data[] = {
    { .value = kind, .has_value = 1 },
    { .value = action, .has_value = 1 },
    { .value = row, .has_value = 1 },
  };
  int ret = cee_sqlite3_insert(cs, info, data,
                               "insert into log "
                               "(kind, action, info) values"
                               "(@kind, @action, @info)", NULL, NULL, NULL);
  if( ret != SQLITE_DONE )
    fprintf(stderr, "%s %s %s\n", kind, action, row);
}

static
void my_apply_script(char *kind, const char *db_file, const char *script_file, struct cee_sqlite3 *cs){
  char log[64];
  size_t len;
  snprintf(log, sizeof log, "%s to %s", script_file, db_file);
  add_log(cs, kind, "apply", log);
  char *schema_text = cee_load_whole_file(script_file, &len);
  cee_sqlite3_init_db(NULL, (char *) db_file, schema_text, false);
  free(schema_text);
}

bool cee_dbm_db_exist(struct cee_dbm_path_info *path_info, const char *db_file_){
  char db_file[128];
  snprintf(db_file, sizeof db_file, "%s/%s", path_info->db_file_path, db_file_);
  if( access(db_file, F_OK) == 0 )
    return true;
  else
    return false;
}

bool cee_dbm_db_init(struct cee_dbm_path_info *path_info,
                     struct cee_dbm_db_info *s,
                     const char *db_file_,
                     const char *restore_data_prefix,
                     int *db_version){
  struct cee_sqlite3 log_cs = {0};
  char log[256];
  open_dbm_log(path_info, &log_cs.db);

  char db_script[128];
  char db_file[128], *kind = s->kind;
  bool is_new_db = false;
  int current_version, migrated_version;

  if( db_file_ )
    snprintf(db_file, sizeof db_file, "%s/%s", path_info->db_file_path, db_file_);
  else
    snprintf(db_file, sizeof db_file, "%s/%s", path_info->db_file_path, s->file);

  snprintf(log, sizeof log, "%s", db_file);
  add_log(&log_cs, kind, "open", log);

  char *x, dir[128];
  if( (x = rindex(db_file, '/')) ){
    snprintf(dir, sizeof dir, "%.*s", x - db_file, db_file);
    //extern int mkdir_p(char *dir, mode_t mode);
    if( mkdir_p(dir, 0755) ){
      perror(dir);
      exit (1);
    }
  }

  snprintf(db_script, sizeof db_script, "%s/%s_create.%d.sql",
           path_info->schema_script_path, s->kind, s->create_version);
  if( access(db_file, F_OK) == 0 ){
    if( s->recreate_db ){
      snprintf(log, sizeof log, "drop all tables of %s to RECREATE a fresh db", db_file);
      add_log(&log_cs, kind, "recreate", log);
      cee_sqlite3_drop_all_tables(NULL, (char *)db_file);
      my_apply_script(kind, db_file, db_script, &log_cs);
      is_new_db = true;
    }
  }else{
    snprintf(log, sizeof log, "create a new %s with %s", db_file, db_script);
    add_log(&log_cs, kind, "create", log);
    my_apply_script(kind, db_file, db_script, &log_cs);
    is_new_db = true;
  }
  current_version = get_user_version(db_file);
  snprintf(log, sizeof log, "user_version %d", current_version);
  add_log(&log_cs, kind, "open", log);

  /* restore backup data */
  if( is_new_db && s->restore_db ){
    if( restore_data_prefix )
      snprintf(db_script, sizeof db_script,
               "%s/%s_insert.%d.sql",
               path_info->data_script_path,
               restore_data_prefix, current_version);
    else
      snprintf(db_script, sizeof db_script,
               "%s/%s_insert.%d.sql", path_info->data_script_path,
               s->kind, current_version);

    if( access(db_script, F_OK) == 0 ){
      snprintf(log, sizeof log, "run %s to restore data %s",
               db_script, db_file);
      add_log(&log_cs, kind, "restore", log);
      my_apply_script(kind, db_file, db_script, &log_cs);
    }else{
      snprintf(log, sizeof log, "cannot find restoration script %s",
               db_script);
      add_log(&log_cs, kind, "restore", log);
    }

    if( s->customize_db ){
      snprintf(db_script, sizeof db_script,
               "%s/%s_customize.%d.sql", path_info->data_script_path,
               s->kind, current_version);

      if( access(db_script, F_OK) == 0 ){
        snprintf(log, sizeof log, "run %s to customize data %s",
                 db_script, db_file);
        add_log(&log_cs, kind, "restore", log);
        my_apply_script(kind, db_file, db_script, &log_cs);
      }else{
        snprintf(log, sizeof log, "cannot find customization script %s",
                 db_script);
        add_log(&log_cs, kind, "restore", log);
      }
    }
  }

  /* migrate database */
  snprintf(db_script, sizeof db_script, "%s/%s_migrate.%d.sql",
           path_info->schema_script_path, s->kind, current_version);
  if( access(db_script, F_OK) == 0 ){
    my_apply_script(kind, db_file, db_script, &log_cs);
    migrated_version = get_user_version(db_file);
    if( migrated_version != current_version ){
      snprintf(log, sizeof log, "ran %s to migrate %s to version %d",
               db_script, db_file, migrated_version);
      add_log(&log_cs, kind, "migrate", log);
    }
  }else{
    snprintf(log, sizeof log, "cannot find %s, no migration", db_script);
    add_log(&log_cs, kind, "migrate", log);
  }

  *db_version = get_user_version(db_file);
  sqlite3_close(log_cs.db);
  return is_new_db;
}


void cee_dbm_path_info2str(struct cee_dbm_path_info *info, size_t buf_size, char *buf){
  size_t remaining_size = buf_size, n;
  char *next = buf;
  n = snprintf(next, remaining_size, "schema_script_path:%s, ",  info->schema_script_path);
  remaining_size -= n;
  next += n;

  n = snprintf(next, remaining_size, "data_script_path:%s, ",  info->data_script_path);
  remaining_size -= n;
  next += n;

  n = snprintf(next, remaining_size, "db_file_path:%s",  info->db_file_path);
  remaining_size -= n;
  next += n;
}

void cee_dbm_db_info2str(struct cee_dbm_db_info *info, size_t buf_size, char *buf){
  size_t remaining_size = buf_size, n;
  char *next = buf;
  n = snprintf(next, remaining_size, "kind:%s, ",  info->kind);
  remaining_size -= n;
  next += n;

  n = snprintf(next, remaining_size, "file:%s, ",  info->file);
  remaining_size -= n;
  next += n;

  n = snprintf(next, remaining_size, "recreate_db:%s, ",
               info->recreate_db ? "true":"false");
  remaining_size -= n;
  next += n;

  n = snprintf(next, remaining_size, "restore_db:%s, ",
               info->restore_db ? "true":"false");
  remaining_size -= n;
  next += n;

  n = snprintf(next, remaining_size, "create_version:%d, ",
               info->create_version);
  remaining_size -= n;
  next += n;

  n = snprintf(next, remaining_size, "final_version:%d",
               info->final_version);
  remaining_size -= n;
  next += n;
}


bool cee_dbm_db_backup(struct cee_dbm_path_info *path_info,
                       struct cee_dbm_db_info *s,
                       const char *db_file_,
                       const char *restore_data_prefix){
  struct cee_sqlite3 log_cs = {0};
  char log[256];
  open_dbm_log(path_info, &log_cs.db);

  char db_script[128];
  char db_file[128], *kind = s->kind;
  int current_version;

  if( db_file_ )
    snprintf(db_file, sizeof db_file, "%s/%s", path_info->db_file_path, db_file_);
  else
    snprintf(db_file, sizeof db_file, "%s/%s", path_info->db_file_path, s->file);

  snprintf(log, sizeof log, "%s", db_file);
  add_log(&log_cs, kind, "open", log);

  if( access(db_file, F_OK) != 0 ){
    snprintf(log, sizeof log, "%s does not exist", db_file);
    add_log(&log_cs, kind, "backup_failed", log);
    return false;
  }

  current_version = get_user_version(db_file);
  snprintf(log, sizeof log, "user_version %d", current_version);
  add_log(&log_cs, kind, "open", log);

  /* backup data */
  if( restore_data_prefix )
    snprintf(db_script, sizeof db_script,
             "/codata/code/bin/dump_insert.sh %s > %s/%s_insert.%d.sql",
             db_file,
             path_info->data_script_path,
             restore_data_prefix, current_version);
  else
    snprintf(db_script, sizeof db_script,
             "/codata/code/bin/dump_insert.sh %s > %s/%s_insert.%d.sql",
             db_file,
             path_info->data_script_path,
             s->kind, current_version);

  int ret = system(db_script);
  if( ret == 0 )
    add_log(&log_cs, kind, "backup", db_script);
  else
    add_log(&log_cs, kind, "backup_failed", db_script);

  sqlite3_close(log_cs.db);
  return true;
}