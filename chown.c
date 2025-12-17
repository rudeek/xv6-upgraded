#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int uid;
  int i;

  if(argc < 3){
    printf(2, "Usage: chown <uid> <file...>\n");
    printf(2, "Example: chown 1000 myfile\n");
    exit();
  }

  uid = atoi(argv[1]);

  for(i = 2; i < argc; i++) {
    if(chown(argv[i], uid) < 0) {
      printf(2, "chown: cannot change '%s': Permission denied\n", argv[i]);
    } else {
      printf(1, "Changed owner of '%s' to uid %d\n", argv[i], uid);
    }
  }

  exit();
}
