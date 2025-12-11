#include "types.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

// Простое логирование, позволяющее одновременные системные вызовы ФС.
//
// Лог-транзакция содержит обновления от нескольких системных вызовов ФС.
// Система логирования делает commit только когда нет активных системных
// вызовов ФС. Таким образом никогда не требуется рассуждать о том, может ли
// commit записать незафиксированные обновления системного вызова на диск.
//
// Системный вызов должен вызывать begin_op()/end_op() чтобы отметить
// свои начало и конец. Обычно begin_op() просто увеличивает счётчик
// выполняющихся системных вызовов ФС и возвращается.
// Но если он думает что лог близок к переполнению, он засыпает
// до тех пор пока последний завершающийся end_op() не сделает commit.
//
// Лог - это физический re-do лог, содержащий блоки диска.
// Формат лога на диске:
//   заголовок блока, содержащий номера блоков для блока A, B, C, ...
//   блок A
//   блок B
//   блок C
//   ...
// Добавления в лог синхронные.

// Параметры для group commit
#define BATCH_THRESHOLD 3      // Минимум операций для группового commit
#define COMMIT_DELAY_TICKS 2   // Максимальная задержка в тиках таймера

// Содержимое блока заголовка, используется как для заголовка на диске
// так и для отслеживания в памяти номеров залогированных блоков до commit.
struct logheader {
  int n;
  int block[LOGSIZE];
};

struct log {
  struct spinlock lock;
  int start;
  int size;
  int outstanding;      // сколько системных вызовов ФС выполняется
  int committing;       // в процессе commit(), пожалуйста подождите
  int dev;
  struct logheader lh;
  
  // === НОВЫЕ ПОЛЯ ДЛЯ GROUP COMMIT ===
  int pending_writes;   // количество ожидающих записей
  int last_commit_tick; // последний тик когда был сделан commit
  int force_commit;     // флаг принудительного commit
};
struct log log;

static void recover_from_log(void);
static void commit();

void
initlog(int dev)
{
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  struct superblock sb;
  initlock(&log.lock, "log");
  readsb(dev, &sb);
  log.start = sb.logstart;
  log.size = sb.nlog;
  log.dev = dev;
  
  // Инициализация полей для group commit
  log.pending_writes = 0;
  log.last_commit_tick = 0;
  log.force_commit = 0;
  
  recover_from_log();
}

// Копирует зафиксированные блоки из лога в их домашнее расположение
static void
install_trans(void)
{
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *lbuf = bread(log.dev, log.start+tail+1); // читаем блок лога
    struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // читаем destination
    memmove(dbuf->data, lbuf->data, BSIZE);  // копируем блок в dst
    bwrite(dbuf);  // пишем dst на диск
    brelse(lbuf);
    brelse(dbuf);
  }
}

// Читает заголовок лога с диска в in-memory заголовок лога
static void
read_head(void)
{
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  log.lh.n = lh->n;
  for (i = 0; i < log.lh.n; i++) {
    log.lh.block[i] = lh->block[i];
  }
  brelse(buf);
}

// Пишет in-memory заголовок лога на диск.
// Это настоящая точка в которой текущая транзакция фиксируется.
static void
write_head(void)
{
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = log.lh.n;
  for (i = 0; i < log.lh.n; i++) {
    hb->block[i] = log.lh.block[i];
  }
  bwrite(buf);
  brelse(buf);
}

static void
recover_from_log(void)
{
  read_head();
  install_trans(); // если зафиксировано, копируем из лога на диск
  log.lh.n = 0;
  write_head(); // очищаем лог
}

// === НОВАЯ ФУНКЦИЯ: Проверка необходимости группового commit ===
// Возвращает 1 если нужно делать commit сейчас
static int
should_commit(void)
{
  int current_tick;
  
  // Если установлен флаг принудительного commit
  if(log.force_commit) {
    return 1;
  }
  
  // Если накопилось достаточно операций для группового commit
  if(log.pending_writes >= BATCH_THRESHOLD) {
    return 1;
  }
  
  // Если прошло слишком много времени с последнего commit
  // (проверяем через глобальный счётчик тиков)
  extern uint ticks;
  extern struct spinlock tickslock;
  
  acquire(&tickslock);
  current_tick = ticks;
  release(&tickslock);
  
  if(current_tick - log.last_commit_tick >= COMMIT_DELAY_TICKS && 
     log.pending_writes > 0) {
    return 1;
  }
  
  // Если лог почти заполнен - обязательно делаем commit
  if(log.lh.n + MAXOPBLOCKS > LOGSIZE - BATCH_THRESHOLD) {
    return 1;
  }
  
  return 0;
}

// вызывается в начале каждого системного вызова ФС
void
begin_op(void)
{
  acquire(&log.lock);
  while(1){
    if(log.committing){
      // Если идёт commit, ждём его завершения
      sleep(&log, &log.lock);
    } else if(log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE){
      // эта операция может исчерпать место в логе; ждём commit
      sleep(&log, &log.lock);
    } else {
      // Можем начинать операцию
      log.outstanding += 1;
      log.pending_writes += 1;  // увеличиваем счётчик ожидающих записей
      release(&log.lock);
      break;
    }
  }
}

// === НОВАЯ ФУНКЦИЯ: Принудительный commit ===
// Используется когда нужно гарантировать что данные на диске
void
force_commit(void)
{
  acquire(&log.lock);
  log.force_commit = 1;
  release(&log.lock);
}

// вызывается в конце каждого системного вызова ФС
// делает commit если это была последняя незавершённая операция
void
end_op(void)
{
  int do_commit = 0;

  acquire(&log.lock);
  log.outstanding -= 1;
  
  if(log.committing)
    panic("log.committing");
  
  // === ИЗМЕНЕНО: Используем новую логику group commit ===
  // Проверяем нужно ли делать commit сейчас
  if(log.outstanding == 0){
    // Нет активных операций - проверяем условия для group commit
    if(should_commit()) {
      do_commit = 1;
      log.committing = 1;
      log.pending_writes = 0;  // сбрасываем счётчик
      log.force_commit = 0;    // сбрасываем флаг
    }
  } else {
    // Есть активные операции - будим ожидающие begin_op(),
    // так как уменьшение log.outstanding освободило место
    wakeup(&log);
  }
  
  release(&log.lock);

  if(do_commit){
    // вызываем commit без удержания блокировок, так как нельзя
    // засыпать с блокировками
    commit();
    
    acquire(&log.lock);
    log.committing = 0;
    
    // Обновляем время последнего commit
    extern uint ticks;
    extern struct spinlock tickslock;
    acquire(&tickslock);
    log.last_commit_tick = ticks;
    release(&tickslock);
    
    wakeup(&log);
    release(&log.lock);
  }
}

// Копирует изменённые блоки из кэша в лог
static void
write_log(void)
{
  int tail;

  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *to = bread(log.dev, log.start+tail+1); // блок лога
    struct buf *from = bread(log.dev, log.lh.block[tail]); // блок кэша
    memmove(to->data, from->data, BSIZE);
    bwrite(to);  // пишем лог
    brelse(from);
    brelse(to);
  }
}

static void
commit()
{
  if (log.lh.n > 0) {
    write_log();     // Пишем изменённые блоки из кэша в лог
    write_head();    // Пишем заголовок на диск -- настоящий commit
    install_trans(); // Теперь устанавливаем записи в домашние расположения
    log.lh.n = 0;
    write_head();    // Стираем транзакцию из лога
  }
}

// Вызывающий изменил b->data и закончил с буфером.
// Записывает номер блока и закрепляет в кэше флагом B_DIRTY.
// commit()/write_log() сделает запись на диск.
//
// log_write() заменяет bwrite(); типичное использование:
//   bp = bread(...)
//   modify bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{
  int i;

  if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
    panic("too big a transaction");
  if (log.outstanding < 1)
    panic("log_write outside of trans");

  acquire(&log.lock);
  for (i = 0; i < log.lh.n; i++) {
    if (log.lh.block[i] == b->blockno)   // абсорбция лога
      break;
  }
  log.lh.block[i] = b->blockno;
  if (i == log.lh.n)
    log.lh.n++;
  b->flags |= B_DIRTY; // предотвращаем вытеснение
  release(&log.lock);
}
