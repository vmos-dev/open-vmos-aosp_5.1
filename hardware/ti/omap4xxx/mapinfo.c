#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mapinfo.h"

extern void *__real_malloc(size_t size);
extern void __real_free(void *ptr);

#if 0
    while (p <= end) {
         data = ptrace(PTRACE_PEEKTEXT, pid, (void*)p, NULL);
         _LOG(tfd, (sp_depth > 2) || only_in_tombstone,
              "    %08x  %08x  %s\n", p, data,
              map_to_name(map, data, ""));
         p += 4;
    }
#endif

// 6f000000-6f01e000 rwxp 00000000 00:0c 16389419   /system/lib/libcomposer.so
// 012345678901234567890123456789012345678901234567890123456789
// 0         1         2         3         4         5

static mapinfo *parse_maps_line(char *line)
{
    mapinfo *mi;
    int len = strlen(line);

    if(len < 1) return 0;
    line[--len] = 0;

    if(len < 50) return 0;
    if(line[20] != 'x') return 0;

    mi = __real_malloc(sizeof(mapinfo) + (len - 47));
    if(mi == 0) return 0;

    mi->start = strtoul(line, 0, 16);
    mi->end = strtoul(line + 9, 0, 16);
    /* To be filled in parse_elf_info if the mapped section starts with
     * elf_header
     */
    mi->next = 0;
    strcpy(mi->name, line + 49);

    return mi;
}

mapinfo *init_mapinfo(int pid)
{
    struct mapinfo *milist = NULL;
    char data[1024];
    sprintf(data, "/proc/%d/maps", pid);
    FILE *fp = fopen(data, "r");
    if(fp) {
        while(fgets(data, sizeof(data), fp)) {
            mapinfo *mi = parse_maps_line(data);
            if(mi) {
                mi->next = milist;
                milist = mi;
            }
        }
        fclose(fp);
    }

    return milist;
}

void deinit_mapinfo(mapinfo *mi)
{
   mapinfo *del;
   while(mi) {
       del = mi;
       mi = mi->next;
       __real_free(del);
   }
}

/* Map a pc address to the name of the containing ELF file */
const char *map_to_name(mapinfo *mi, unsigned pc, const char* def)
{
    while(mi) {
        if((pc >= mi->start) && (pc < mi->end)){
            return mi->name;
        }
        mi = mi->next;
    }
    return def;
}

/* Find the containing map info for the pc */
const mapinfo *pc_to_mapinfo(mapinfo *mi, unsigned pc, unsigned *rel_pc)
{
    *rel_pc = pc;
    while(mi) {
        if((pc >= mi->start) && (pc < mi->end)){
            // Only calculate the relative offset for shared libraries
            if (strstr(mi->name, ".so")) {
                *rel_pc -= mi->start;
            }
            return mi;
        }
        mi = mi->next;
    }
    return NULL;
}
