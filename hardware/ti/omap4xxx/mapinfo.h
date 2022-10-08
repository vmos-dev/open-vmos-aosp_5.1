#ifndef MAPINFO_H
#define MAPINFO_H

typedef struct mapinfo {
    struct mapinfo *next;
    unsigned start;
    unsigned end;
    char name[];
} mapinfo;

mapinfo *init_mapinfo(int pid);
void deinit_mapinfo(mapinfo *mi);
const char *map_to_name(mapinfo *mi, unsigned pc, const char* def);
const mapinfo *pc_to_mapinfo(mapinfo *mi, unsigned pc, unsigned *rel_pc);

#endif
