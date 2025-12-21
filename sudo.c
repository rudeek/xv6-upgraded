#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  char password[32];
  char username[16];
  int i;
  
  if(argc < 2) {
    printf(2, "Usage: sudo <command> [args...]\n");
    printf(2, "Example: sudo cat /secret.txt\n");
    exit();
  }
  
  // Проверяем: может ли пользователь использовать sudo?
  if(!cansudo()) {
    whoami(username, 16);
    printf(2, "%s is not in the sudoers file. This incident will be reported.\n", username);
    exit();
  }
  
  // Запрашиваем пароль
  whoami(username, 16);
  printf(1, "[sudo] password for %s: ", username);
  
  // Читаем пароль (без echo на экран - упрощённо)
  gets(password, 32);
  
  // Убираем \n в конце
  for(i = 0; password[i]; i++) {
    if(password[i] == '\n') {
      password[i] = '\0';
      break;
    }
  }
  
  // Проверяем пароль
  int original_uid = getuid();
  
  if(login(username, password) < 0) {
    printf(2, "sudo: authentication failure\n");
    exit();
  }
  
  // Пароль верный - временно становимся root
  if(setsuid(0) < 0) {
    printf(2, "sudo: failed to set uid\n");
    exit();
  }
  
  printf(1, "[sudo] executing as root...\n");
  
  // Создаём массив аргументов для exec
  char *exec_argv[16];  // максимум 16 аргументов
  int arg_count = 0;
  
  for(i = 1; i < argc && arg_count < 15; i++) {
    exec_argv[arg_count++] = argv[i];
  }
  exec_argv[arg_count] = 0;  // NULL-терминатор
  
  // Запускаем команду с правами root
  if(exec(argv[1], exec_argv) < 0) {
    printf(2, "sudo: %s: command not found\n", argv[1]);
    
    // Возвращаем оригинальный UID
    setsuid(original_uid);
    exit();
  }
  
  // exec не возвращает управление если успешен
  exit();
}
