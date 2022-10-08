#ifndef _IMR_TOC_H_
#define _IMR_TOC_H_

/*
 * IMR Table of Contents format
 */
typedef struct {
    uint32_t start_offset;
    uint32_t size;
    uint32_t reserved1;
    uint32_t reserved2;
} imr_toc_entry_t;

typedef struct {
    imr_toc_entry_t entries[8];    /* pick reasonable size to make gcc happy */
} imr_toc_t;

#define MAKE_TOC_VERSION(maj, min)  ((min) << 16 | (maj))
typedef struct {
    uint16_t toc_maj_ver;
    uint16_t toc_min_ver;
} imr_toc_entry_version_t;

/*
 * IMR6 values
 */

#define IMR6_TOC_MAGIC       0x6CD96EDB

#define IMR6_TOC_VERSION_MAJ 0x0001
#define IMR6_TOC_VERSION_MIN 0x0000

/* ToC entry order for IMR6 */
enum imr6_entries {
    IMR_TOC_ENTRY_TOC = 0,
    IMR_TOC_ENTRY_MTX_WB_BUF,
    IMR_TOC_ENTRY_VXE_FW,
    IMR_TOC_ENTRY_VXE_CTX_BUF,
    IMR_TOC_ENTRY_VXE_SEC_PGTBLS,
    IMR_TOC_ENTRY_PC_BUFS,
    IMR_TOC_ENTRY_VXE_SHADOW_PGTBLS,
    IMR_TOC_ENTRY_XEN_EXTRA,
};


/*
 * IMR7 values
 */

#define IMR7_TOC_MAGIC       0x6ED96CDB

#define IMR7_TOC_VERSION_MAJ 0x0001
#define IMR7_TOC_VERSION_MIN 0x0000

/* ToC entry order for IMR7 */
enum imr7_entries {
    /* IMR_TOC_ENTRY_TOC = 0, */
    IMR_TOC_ENTRY_MAILBOXES = 1,     /* contents per imr_ia_chaabi_mailbox_t */
    IMR_TOC_ENTRY_IA_RUNTIME_FW,
    IMR_TOC_ENTRY_XEN
};

/* entry-specific data structures */

#define IMR6_PC_BUFS_START_VADDR  0x11223344

typedef struct {
    uint32_t hdcp_sess_status;
    union {
        struct {
	    uint32_t hdcp_sess_key_00_31;
	    uint32_t hdcp_sess_key_32_63;
	    uint32_t hdcp_sess_key_64_95;
	    uint32_t hdcp_sess_key_96_127;
	};
        uint8_t hdcp_sess_key[16];
    };
    union {
        struct {
	    uint32_t hdcp_iv_00_31;
	    uint32_t hdcp_iv_32_63;
	};
        uint64_t hdcp_iv;
    };
} imr_ia_chaabi_mailbox_t;

#endif
