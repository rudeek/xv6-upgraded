#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"

// Структура пользователя
struct user {
  int uid;              // User ID (0 = root)
  int gid;              // Group ID
  char username[16];    // Имя пользователя
  char password[32];    // Пароль (простой для начала)
  int active;           // 1 = пользователь существует
};

// Максимум 10 пользователей
#define NUSER 10

struct {
  struct spinlock lock;
  struct user users[NUSER];
} usertable;

// Инициализация системы пользователей
void
users_init(void)
{
  int i;
  
  initlock(&usertable.lock, "usertable");
  
  // Создаём root пользователя (uid=0)
  usertable.users[0].uid = 0;
  usertable.users[0].gid = 0;
  safestrcpy(usertable.users[0].username, "root", 16);
  safestrcpy(usertable.users[0].password, "toor", 32);
  usertable.users[0].active = 1;
  
  // Создаём обычного пользователя (uid=1000)
  usertable.users[1].uid = 1000;
  usertable.users[1].gid = 1000;
  safestrcpy(usertable.users[1].username, "user", 16);
  safestrcpy(usertable.users[1].password, "user", 32);
  usertable.users[1].active = 1;
  
  // Остальные слоты пустые
  for(i = 2; i < NUSER; i++) {
    usertable.users[i].active = 0;
  }
}

// Проверка пароля и получение UID
int
users_checkpassword(char *username, char *password)
{
  int i;
  
  acquire(&usertable.lock);
  
  for(i = 0; i < NUSER; i++) {
    if(usertable.users[i].active && 
       strncmp(usertable.users[i].username, username, 16) == 0) {
      
      // Нашли пользователя - проверяем пароль
      if(strncmp(usertable.users[i].password, password, 32) == 0) {
        int uid = usertable.users[i].uid;
        release(&usertable.lock);
        return uid;  // Возвращаем UID
      }
      
      release(&usertable.lock);
      return -1;  // Неверный пароль
    }
  }
  
  release(&usertable.lock);
  return -1;  // Пользователь не найден
}

// Получить имя пользователя по UID
void
users_getname(int uid, char *buf, int size)
{
  int i;
  
  acquire(&usertable.lock);
  
  for(i = 0; i < NUSER; i++) {
    if(usertable.users[i].active && usertable.users[i].uid == uid) {
      safestrcpy(buf, usertable.users[i].username, size);
      release(&usertable.lock);
      return;
    }
  }
  
  // Не нашли - возвращаем "unknown"
  safestrcpy(buf, "unknown", size);
  release(&usertable.lock);
}

// Добавить нового пользователя (только root может!)
int
users_add(char *username, char *password, int uid, int gid)
{
  int i;
  
  acquire(&usertable.lock);
  
  // Ищем свободный слот
  for(i = 0; i < NUSER; i++) {
    if(!usertable.users[i].active) {
      usertable.users[i].uid = uid;
      usertable.users[i].gid = gid;
      safestrcpy(usertable.users[i].username, username, 16);
      safestrcpy(usertable.users[i].password, password, 32);
      usertable.users[i].active = 1;
      release(&usertable.lock);
      return 0;  // Успех
    }
  }
  
  release(&usertable.lock);
  return -1;  // Нет места
}

// Удалить пользователя (только root может!)
int
users_delete(char *username)
{
  int i;
  
  acquire(&usertable.lock);
  
  for(i = 0; i < NUSER; i++) {
    if(usertable.users[i].active && 
       strncmp(usertable.users[i].username, username, 16) == 0) {
      
      // Нельзя удалить root!
      if(usertable.users[i].uid == 0) {
        release(&usertable.lock);
        return -1;
      }
      
      usertable.users[i].active = 0;
      release(&usertable.lock);
      return 0;  // Успех
    }
  }
  
  release(&usertable.lock);
  return -1;  // Не найден
}

// Вывести список всех пользователей
void
users_list(void)
{
  int i;
  
  acquire(&usertable.lock);
  
  cprintf("UID  GID  USERNAME\n");
  cprintf("---  ---  --------\n");
  
  for(i = 0; i < NUSER; i++) {
    if(usertable.users[i].active) {
      cprintf("%-4d %-4d %s\n", 
              usertable.users[i].uid,
              usertable.users[i].gid,
              usertable.users[i].username);
    }
  }
  
  release(&usertable.lock);
}
