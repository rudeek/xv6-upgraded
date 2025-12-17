#include "types.h"
#include "stat.h"
#include "user.h"

// Конвертирует строку типа "755" в число
int
parse_mode(char *str)
{
  int mode = 0;
  int i;
  
  for(i = 0; str[i]; i++) {
    if(str[i] < '0' || str[i] > '7')
      return -1;
    mode = mode * 8 + (str[i] - '0');
  }
  
  return mode;
}

int
main(int argc, char *argv[])
{
  int mode;
  int i;

  if(argc < 3){
    printf(2, "Usage: chmod <mode> <file...>\n");
    printf(2, "Example: chmod 755 myfile\n");
    printf(2, "         chmod 644 file1 file2\n");
    exit();
  }

  mode = parse_mode(argv[1]);
  
  if(mode < 0 || mode > 0777) {
    printf(2, "chmod: invalid mode '%s'\n", argv[1]);
    exit();
  }

  for(i = 2; i < argc; i++) {
    if(chmod(argv[i], mode) < 0) {
      printf(2, "chmod: cannot change '%s': Permission denied\n", argv[i]);
    } else {
      printf(1, "Changed permissions of '%s' to %o\n", argv[i], mode);
    }
  }

  exit();
}
