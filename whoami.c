#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  char username[16];
  
  whoami(username, 16);
  
  printf(1, "%s (uid=%d)\n", username, getuid());
  
  exit();
}
