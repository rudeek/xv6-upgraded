#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

// Конвертирует права в строку типа "rwxr-xr-x"
void
mode_to_string(ushort mode, char *str)
{
  str[0] = (mode & 0400) ? 'r' : '-';
  str[1] = (mode & 0200) ? 'w' : '-';
  str[2] = (mode & 0100) ? 'x' : '-';
  str[3] = (mode & 0040) ? 'r' : '-';
  str[4] = (mode & 0020) ? 'w' : '-';
  str[5] = (mode & 0010) ? 'x' : '-';
  str[6] = (mode & 0004) ? 'r' : '-';
  str[7] = (mode & 0002) ? 'w' : '-';
  str[8] = (mode & 0001) ? 'x' : '-';
  str[9] = '\0';
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  char mode_str[10];

  if((fd = open(path, 0)) < 0){
    printf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    mode_to_string(st.mode, mode_str);
    printf(1, "%s %d %d %d %s\n", mode_str, st.uid, st.gid, st.size, path);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
      mode_to_string(st.mode, mode_str);
      printf(1, "%s %d %d %d %s\n", mode_str, st.uid, st.gid, st.size, buf);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    ls(".");
    exit();
  }
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit();
}
