#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

int
main(void)
{
  printf(1, "Block size test:\n");
  printf(1, "  BSIZE = %d bytes\n", BSIZE);
  printf(1, "  DIRSIZ = %d\n", DIRSIZ);
  
  // Выделяем память в куче, а не на стеке
  char *buf = malloc(BSIZE);
  char *readbuf = malloc(BSIZE);
  
  if(buf == 0 || readbuf == 0) {
    printf(2, "malloc failed\n");
    exit();
  }
  
  // Создаём файл и пишем данные
  int fd = open("testfile", O_CREATE | O_RDWR);
  if(fd < 0) {
    printf(2, "Failed to create file\n");
    exit();
  }
  
  int i;
  for(i = 0; i < BSIZE; i++)
    buf[i] = 'A' + (i % 26);
  
  int written = write(fd, buf, BSIZE);
  printf(1, "  Written %d bytes in one write\n", written);
  
  close(fd);
  
  // Читаем обратно
  fd = open("testfile", O_RDONLY);
  int readed = read(fd, readbuf, BSIZE);
  printf(1, "  Read %d bytes in one read\n", readed);
  
  // Проверяем
  int errors = 0;
  for(i = 0; i < BSIZE; i++) {
    if(readbuf[i] != buf[i])
      errors++;
  }
  
  if(errors == 0)
    printf(1, "  SUCCESS: Block I/O works correctly!\n");
  else
    printf(1, "  FAILED: %d errors found\n", errors);
  
  close(fd);
  unlink("testfile");
  
  free(buf);
  free(readbuf);
  
  exit();
}
