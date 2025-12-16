#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  char password[32];
  
  if(argc < 2) {
    printf(2, "Usage: su <username>\n");
    exit();
  }
  
  printf(1, "Password: ");
  gets(password, 32);
  
  // Убираем \n в конце
  int i;
  for(i = 0; password[i]; i++) {
    if(password[i] == '\n') {
      password[i] = '\0';
      break;
    }
  }
  
  // Пытаемся залогиниться
  if(login(argv[1], password) < 0) {
    printf(2, "su: authentication failure\n");
    exit();
  }
  
  printf(1, "Switched to user: %s\n", argv[1]);
  
  // Запускаем новый shell
  char *sh_argv[] = { "sh", 0 };
  exec("sh", sh_argv);
  
  printf(2, "su: exec sh failed\n");
  exit();
}
