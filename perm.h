#ifndef PERM_H
#define PERM_H

// Права доступа (как в Unix)
#define S_IRUSR 0400   // User read
#define S_IWUSR 0200   // User write
#define S_IXUSR 0100   // User execute

#define S_IRGRP 0040   // Group read
#define S_IWGRP 0020   // Group write
#define S_IXGRP 0010   // Group execute

#define S_IROTH 0004   // Others read
#define S_IWOTH 0002   // Others write
#define S_IXOTH 0001   // Others execute

// Комбинации
#define S_IRWXU 0700   // User rwx
#define S_IRWXG 0070   // Group rwx
#define S_IRWXO 0007   // Others rwx

// Права по умолчанию
#define DEFAULT_FILE_MODE 0644    // rw-r--r--
#define DEFAULT_DIR_MODE  0755    // rwxr-xr-x

// Проверка прав
#define CAN_READ(mode, uid, gid, f_uid, f_gid) \
  (uid == 0 || \
   (uid == f_uid && (mode & S_IRUSR)) || \
   (gid == f_gid && (mode & S_IRGRP)) || \
   (mode & S_IROTH))

#define CAN_WRITE(mode, uid, gid, f_uid, f_gid) \
  (uid == 0 || \
   (uid == f_uid && (mode & S_IWUSR)) || \
   (gid == f_gid && (mode & S_IWGRP)) || \
   (mode & S_IWOTH))

#define CAN_EXECUTE(mode, uid, gid, f_uid, f_gid) \
  (uid == 0 || \
   (uid == f_uid && (mode & S_IXUSR)) || \
   (gid == f_gid && (mode & S_IXGRP)) || \
   (mode & S_IXOTH))

#endif
