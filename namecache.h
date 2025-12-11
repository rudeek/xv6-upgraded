#ifndef NAMECACHE_H
#define NAMECACHE_H

#define NAMECACHE_DIRSIZ 14
#define NAMECACHE_SIZE 32

struct namecache_entry {
  int valid;
  uint dev;
  uint parent_inum;
  char name[NAMECACHE_DIRSIZ];
  uint inum;
  uint hits;
};

void namecache_init(void);
uint namecache_lookup(uint dev, uint parent_inum, char *name);
void namecache_add(uint dev, uint parent_inum, char *name, uint inum);
void namecache_invalidate(uint dev, uint inum);
void namecache_clear(void);
void namecache_stats(void);

#endif
