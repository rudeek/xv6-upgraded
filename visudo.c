#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  int uid;
  
  // Только root может управлять sudoers
  if(getuid() != 0) {
    printf(2, "visudo: only root can modify sudoers\n");
    exit();
  }
  
  if(argc < 3) {
    printf(2, "Usage: visudo <add|remove> <uid>\n");
    printf(2, "Examples:\n");
    printf(2, "  visudo add 1000      # add user to sudoers\n");
    printf(2, "  visudo remove 1000   # remove user from sudoers\n");
    exit();
  }
  
  uid = atoi(argv[2]);
  
  if(strcmp(argv[1], "add") == 0) {
    if(addsudoer(uid) == 0) {  // ИЗМЕНЕНО: используем системный вызов
      printf(1, "Added uid %d to sudoers\n", uid);
    } else {
      printf(2, "Failed to add uid %d to sudoers\n", uid);
    }
  }
  else if(strcmp(argv[1], "remove") == 0) {
    if(removesudoer(uid) == 0) {  // ИЗМЕНЕНО: используем системный вызов
      printf(1, "Removed uid %d from sudoers\n", uid);
    } else {
      printf(2, "Failed to remove uid %d from sudoers\n", uid);
    }
  }
  else {
    printf(2, "visudo: unknown command '%s'\n", argv[1]);
    printf(2, "Use 'add' or 'remove'\n");
  }
  
  exit();
}
