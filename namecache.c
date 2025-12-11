#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "fs.h"
#include "namecache.h"

// Проверка на соответствие размеров
#if NAMECACHE_DIRSIZ != DIRSIZ
#error "NAMECACHE_DIRSIZ must equal DIRSIZ"
#endif

// Глобальная структура кэша
struct {
  struct spinlock lock;
  struct namecache_entry entries[NAMECACHE_SIZE];
  int next_slot;              // следующий слот для замены (round-robin)
  
  // Статистика (опционально, для отладки)
  uint total_lookups;
  uint cache_hits;
  uint cache_misses;
} namecache;

// Инициализация кэша
void
namecache_init(void)
{
  int i;
  
  initlock(&namecache.lock, "namecache");
  
  for(i = 0; i < NAMECACHE_SIZE; i++) {
    namecache.entries[i].valid = 0;
    namecache.entries[i].hits = 0;
  }
  
  namecache.next_slot = 0;
  namecache.total_lookups = 0;
  namecache.cache_hits = 0;
  namecache.cache_misses = 0;
}

// Поиск в кэше
// Возвращает inum если найдено, 0 если не найдено
uint
namecache_lookup(uint dev, uint parent_inum, char *name)
{
  int i;
  struct namecache_entry *e;
  uint result;
  
  acquire(&namecache.lock);
  
  namecache.total_lookups++;
  
  // Перебираем все записи в кэше
  for(i = 0; i < NAMECACHE_SIZE; i++) {
    e = &namecache.entries[i];
    
    // Проверяем совпадение всех полей
    if(e->valid && 
       e->dev == dev && 
       e->parent_inum == parent_inum &&
       strncmp(e->name, name, DIRSIZ) == 0) {
      
      // Нашли в кэше!
      e->hits++;
      namecache.cache_hits++;
      result = e->inum;
      
      release(&namecache.lock);
      return result;
    }
  }
  
  // Не нашли в кэше
  namecache.cache_misses++;
  release(&namecache.lock);
  return 0;
}

// Добавление записи в кэш
void
namecache_add(uint dev, uint parent_inum, char *name, uint inum)
{
  int i;
  struct namecache_entry *e;
  
  acquire(&namecache.lock);
  
  // Проверяем, не существует ли уже такая запись
  for(i = 0; i < NAMECACHE_SIZE; i++) {
    e = &namecache.entries[i];
    
    if(e->valid && 
       e->dev == dev && 
       e->parent_inum == parent_inum &&
       strncmp(e->name, name, DIRSIZ) == 0) {
      // Уже есть, просто обновляем inum
      e->inum = inum;
      release(&namecache.lock);
      return;
    }
  }
  
  // Добавляем новую запись (вытесняем старую по round-robin)
  e = &namecache.entries[namecache.next_slot];
  
  e->valid = 1;
  e->dev = dev;
  e->parent_inum = parent_inum;
  e->inum = inum;
  e->hits = 0;
  strncpy(e->name, name, DIRSIZ);
  
  // Переходим к следующему слоту
  namecache.next_slot = (namecache.next_slot + 1) % NAMECACHE_SIZE;
  
  release(&namecache.lock);
}

// Инвалидация записей при удалении/переименовании
void
namecache_invalidate(uint dev, uint inum)
{
  int i;
  struct namecache_entry *e;
  
  acquire(&namecache.lock);
  
  // Удаляем все записи, связанные с этим inode
  for(i = 0; i < NAMECACHE_SIZE; i++) {
    e = &namecache.entries[i];
    
    if(e->valid && e->dev == dev) {
      // Инвалидируем если это сам файл или его родитель
      if(e->inum == inum || e->parent_inum == inum) {
        e->valid = 0;
      }
    }
  }
  
  release(&namecache.lock);
}

// Полная очистка кэша
void
namecache_clear(void)
{
  int i;
  
  acquire(&namecache.lock);
  
  for(i = 0; i < NAMECACHE_SIZE; i++) {
    namecache.entries[i].valid = 0;
  }
  
  namecache.next_slot = 0;
  
  release(&namecache.lock);
}

// Вывод статистики (для отладки)
void
namecache_stats(void)
{
  int i;
  struct namecache_entry *e;
  uint hit_rate;
  
  acquire(&namecache.lock);
  
  cprintf("Name Cache Statistics:\n");
  cprintf("  Total lookups: %d\n", namecache.total_lookups);
  cprintf("  Cache hits:    %d\n", namecache.cache_hits);
  cprintf("  Cache misses:  %d\n", namecache.cache_misses);
  
  if(namecache.total_lookups > 0) {
    hit_rate = (namecache.cache_hits * 100) / namecache.total_lookups;
    cprintf("  Hit rate:      %d%%\n", hit_rate);
  }
  
  cprintf("\nActive cache entries:\n");
  for(i = 0; i < NAMECACHE_SIZE; i++) {
    e = &namecache.entries[i];
    if(e->valid) {
      cprintf("  [%d] parent=%d name=%s inum=%d hits=%d\n", 
              i, e->parent_inum, e->name, e->inum, e->hits);
    }
  }
  
  release(&namecache.lock);
}
