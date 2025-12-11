// Тест производительности group commit оптимизации
// Сравнивает скорость записи множества мелких файлов

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define NUM_FILES 50        // количество файлов для теста
#define FILE_SIZE 512       // размер каждого файла в байтах
#define NUM_ITERATIONS 3    // количество повторений теста

// Функция для получения текущего времени в тиках
int
gettime(void)
{
  return uptime();
}

// Тест 1: Создание множества мелких файлов
void
test_create_many_files(void)
{
  int i, fd;
  char name[16];
  char data[FILE_SIZE];
  int start_time, end_time;
  
  printf(1, "\n=== ТЕСТ 1: Создание %d файлов по %d байт ===\n", NUM_FILES, FILE_SIZE);
  
  // Подготовка данных для записи
  for(i = 0; i < FILE_SIZE; i++) {
    data[i] = 'A' + (i % 26);
  }
  
  start_time = gettime();
  
  // Создаём и пишем файлы
  for(i = 0; i < NUM_FILES; i++) {
    name[0] = 'f';
    name[1] = 'i';
    name[2] = 'l';
    name[3] = 'e';
    name[4] = '0' + (i / 10);
    name[5] = '0' + (i % 10);
    name[6] = '\0';
    
    fd = open(name, O_CREATE | O_RDWR);
    if(fd < 0) {
      printf(1, "ОШИБКА: не удалось создать %s\n", name);
      exit();
    }
    
    if(write(fd, data, FILE_SIZE) != FILE_SIZE) {
      printf(1, "ОШИБКА: не удалось записать в %s\n", name);
      close(fd);
      exit();
    }
    
    close(fd);
    
    // Выводим прогресс каждые 10 файлов
    if((i + 1) % 10 == 0) {
      printf(1, "  Создано файлов: %d/%d\n", i + 1, NUM_FILES);
    }
  }
  
  end_time = gettime();
  
  printf(1, "Время выполнения: %d тиков\n", end_time - start_time);
  printf(1, "Средняя скорость: %d файлов/тик\n", 
         NUM_FILES * 100 / (end_time - start_time + 1));
  
  // Очистка - удаляем созданные файлы
  for(i = 0; i < NUM_FILES; i++) {
    name[0] = 'f';
    name[1] = 'i';
    name[2] = 'l';
    name[3] = 'e';
    name[4] = '0' + (i / 10);
    name[5] = '0' + (i % 10);
    name[6] = '\0';
    unlink(name);
  }
}

// Тест 2: Множество мелких записей в один файл
void
test_many_small_writes(void)
{
  int i, fd;
  char data[64];
  int start_time, end_time;
  int num_writes = 100;
  
  printf(1, "\n=== ТЕСТ 2: %d мелких записей в один файл ===\n", num_writes);
  
  // Подготовка данных
  for(i = 0; i < 64; i++) {
    data[i] = 'B' + (i % 20);
  }
  
  unlink("testfile");
  
  fd = open("testfile", O_CREATE | O_RDWR);
  if(fd < 0) {
    printf(1, "ОШИБКА: не удалось создать testfile\n");
    exit();
  }
  
  start_time = gettime();
  
  // Делаем много мелких записей
  for(i = 0; i < num_writes; i++) {
    if(write(fd, data, 64) != 64) {
      printf(1, "ОШИБКА: запись %d не удалась\n", i);
      close(fd);
      exit();
    }
    
    if((i + 1) % 20 == 0) {
      printf(1, "  Записей выполнено: %d/%d\n", i + 1, num_writes);
    }
  }
  
  end_time = gettime();
  close(fd);
  
  printf(1, "Время выполнения: %d тиков\n", end_time - start_time);
  printf(1, "Средняя скорость: %d записей/тик\n", 
         num_writes * 100 / (end_time - start_time + 1));
  
  unlink("testfile");
}

// Тест 3: Создание и удаление файлов (тест транзакций)
void
test_create_delete_cycle(void)
{
  int i, j, fd;
  char name[16];
  char data[256];
  int start_time, end_time;
  int cycles = 10;
  
  printf(1, "\n=== ТЕСТ 3: %d циклов создания/удаления 5 файлов ===\n", cycles);
  
  for(i = 0; i < 256; i++) {
    data[i] = 'C' + (i % 15);
  }
  
  start_time = gettime();
  
  for(j = 0; j < cycles; j++) {
    // Создаём 5 файлов
    for(i = 0; i < 5; i++) {
      name[0] = 't';
      name[1] = 'm';
      name[2] = 'p';
      name[3] = '0' + i;
      name[4] = '\0';
      
      fd = open(name, O_CREATE | O_RDWR);
      if(fd < 0) {
        printf(1, "ОШИБКА: создание %s\n", name);
        exit();
      }
      write(fd, data, 256);
      close(fd);
    }
    
    // Удаляем эти 5 файлов
    for(i = 0; i < 5; i++) {
      name[0] = 't';
      name[1] = 'm';
      name[2] = 'p';
      name[3] = '0' + i;
      name[4] = '\0';
      unlink(name);
    }
    
    if((j + 1) % 2 == 0) {
      printf(1, "  Циклов выполнено: %d/%d\n", j + 1, cycles);
    }
  }
  
  end_time = gettime();
  
  printf(1, "Время выполнения: %d тиков\n", end_time - start_time);
  printf(1, "Средняя скорость: %d циклов/тик\n", 
         cycles * 100 / (end_time - start_time + 1));
}

// Тест 4: Параллельная запись (форк процессов)
void
test_concurrent_writes(void)
{
  int i, fd, pid;
  char name[16];
  char data[512];
  int start_time, end_time;
  int num_processes = 4;
  int files_per_process = 10;
  
  printf(1, "\n=== ТЕСТ 4: %d параллельных процессов по %d файлов ===\n", 
         num_processes, files_per_process);
  
  for(i = 0; i < 512; i++) {
    data[i] = 'D' + (i % 18);
  }
  
  start_time = gettime();
  
  // Создаём дочерние процессы
  for(i = 0; i < num_processes; i++) {
    pid = fork();
    
    if(pid < 0) {
      printf(1, "ОШИБКА: fork не удался\n");
      exit();
    }
    
    if(pid == 0) {
      // Дочерний процесс - пишем файлы
      int j;
      for(j = 0; j < files_per_process; j++) {
        name[0] = 'p';
        name[1] = '0' + i;
        name[2] = 'f';
        name[3] = '0' + (j / 10);
        name[4] = '0' + (j % 10);
        name[5] = '\0';
        
        fd = open(name, O_CREATE | O_RDWR);
        if(fd >= 0) {
          write(fd, data, 512);
          close(fd);
        }
      }
      exit(); // Дочерний процесс завершается
    }
  }
  
  // Родительский процесс ждёт всех детей
  for(i = 0; i < num_processes; i++) {
    wait();
  }
  
  end_time = gettime();
  
  printf(1, "Время выполнения: %d тиков\n", end_time - start_time);
  printf(1, "Всего файлов создано: %d\n", num_processes * files_per_process);
  printf(1, "Средняя скорость: %d файлов/тик\n", 
         (num_processes * files_per_process * 100) / (end_time - start_time + 1));
  
  // Очистка
  for(i = 0; i < num_processes; i++) {
    int j;
    for(j = 0; j < files_per_process; j++) {
      name[0] = 'p';
      name[1] = '0' + i;
      name[2] = 'f';
      name[3] = '0' + (j / 10);
      name[4] = '0' + (j % 10);
      name[5] = '\0';
      unlink(name);
    }
  }
}

// Тест 5: Бенчмарк - общее время всех операций
void
run_benchmark(void)
{
  int total_start, total_end;
  int i;
  
  printf(1, "\n========================================\n");
  printf(1, "   БЕНЧМАРК GROUP COMMIT ОПТИМИЗАЦИИ\n");
  printf(1, "========================================\n");
  
  total_start = gettime();
  
  for(i = 0; i < NUM_ITERATIONS; i++) {
    printf(1, "\n--- Итерация %d/%d ---\n", i + 1, NUM_ITERATIONS);
    
    test_create_many_files();
    test_many_small_writes();
    test_create_delete_cycle();
    test_concurrent_writes();
  }
  
  total_end = gettime();
  
  printf(1, "\n========================================\n");
  printf(1, "ОБЩЕЕ ВРЕМЯ ВСЕХ ТЕСТОВ: %d тиков\n", total_end - total_start);
  printf(1, "Среднее время на итерацию: %d тиков\n", 
         (total_end - total_start) / NUM_ITERATIONS);
  printf(1, "========================================\n");
}

int
main(int argc, char *argv[])
{
  printf(1, "\n");
  printf(1, "╔════════════════════════════════════════╗\n");
  printf(1, "║  ТЕСТ ПРОИЗВОДИТЕЛЬНОСТИ GROUP COMMIT  ║\n");
  printf(1, "╚════════════════════════════════════════╝\n");
  
  // Проверяем что мы в правильной директории
  if(open("README", 0) < 0) {
    printf(1, "ПРЕДУПРЕЖДЕНИЕ: README не найден\n");
  }
  
  run_benchmark();
  
  printf(1, "\n✓ Все тесты завершены успешно!\n\n");
  
  exit();
}
