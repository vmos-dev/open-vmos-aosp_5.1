#include <stdint.h>
#include "imr_toc.h"

imr_toc_t imr6_toc = { {
    /* Table of Contents */
    { 0x00000000,
      0x00001000,
      IMR6_TOC_MAGIC,
      MAKE_TOC_VERSION(IMR6_TOC_VERSION_MAJ, IMR6_TOC_VERSION_MIN)
    },
    /* MTX writeback buffer */
    { 0x00002000,
      0x00001000,
      0, 0
    },
    /* VXE FW */
    { 0x00003000,
      0x00080000,
      0, 0
    },
    /* VXE context buffer */
    { 0x00083000,
      0x0000C000,
      0, 0
    },
    /* VXE secure page tables */
    { 0x0008F000,
      0x00020000,
      0, 0
    },
    /* protected content bufs */
    { 0x000AF000,
      0x01551000,
      IMR6_PC_BUFS_START_VADDR,
      0
    },
    /* shadow page table */
    { 0x00060000,
      0x00020000,
      0,
      0
    },
    /* memory for Xen */
    { 0x01600000,
      0x00C00000,
      0, 0
    }}
};

imr_toc_t imr7_toc = { {
    /* Table of Contents */
    { 0x00000000,
      0x00001000,
      IMR7_TOC_MAGIC,
      MAKE_TOC_VERSION(IMR7_TOC_VERSION_MAJ, IMR7_TOC_VERSION_MIN)
    },
    /* platform svcs/Chaabi mailboxes */
    { 0x00001000,
      0x00001000,
      0, 0
    },
    /* IA runtime FW */
    { 0x00002000,
      0x00020000,
      0, 0
    },
    /* Xen */
    { 0x00022000,
      0x00300000,
      0 ,0
    } }
};
