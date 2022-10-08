/* bootparms.h base on Linux kernel include/asm-x86/bootparam.h */

#include "types.h"

#define EDD_MBR_SIG_MAX 16
#define E820MAX 128
#define EDDMAXNR 6

/* extensible setup data list node */
struct setup_data {
	__u64 next;
	__u32 type;
	__u32 len;
	__u8 data[0];
};

struct screen_info {
	__u8  orig_x;	   /* 0x00 */
	__u8  orig_y;	   /* 0x01 */
	__u16 ext_mem_k;	/* 0x02 */
	__u16 orig_video_page;  /* 0x04 */
	__u8  orig_video_mode;  /* 0x06 */
	__u8  orig_video_cols;  /* 0x07 */
	__u16 unused2;	  /* 0x08 */
	__u16 orig_video_ega_bx;/* 0x0a */
	__u16 unused3;	  /* 0x0c */
	__u8  orig_video_lines; /* 0x0e */
	__u8  orig_video_isVGA; /* 0x0f */
	__u16 orig_video_points;/* 0x10 */

	/* VESA graphic mode -- linear frame buffer */
	__u16 lfb_width;	/* 0x12 */
	__u16 lfb_height;       /* 0x14 */
	__u16 lfb_depth;	/* 0x16 */
	__u32 lfb_base;	 /* 0x18 */
	__u32 lfb_size;	 /* 0x1c */
	__u16 cl_magic, cl_offset; /* 0x20 */
	__u16 lfb_linelength;   /* 0x24 */
	__u8  red_size;	 /* 0x26 */
	__u8  red_pos;	  /* 0x27 */
	__u8  green_size;       /* 0x28 */
	__u8  green_pos;	/* 0x29 */
	__u8  blue_size;	/* 0x2a */
	__u8  blue_pos;	 /* 0x2b */
	__u8  rsvd_size;	/* 0x2c */
	__u8  rsvd_pos;	 /* 0x2d */
	__u16 vesapm_seg;       /* 0x2e */
	__u16 vesapm_off;       /* 0x30 */
	__u16 pages;	    /* 0x32 */
	__u16 vesa_attributes;  /* 0x34 */
	__u32 capabilities;     /* 0x36 */
	__u8  _reserved[6];     /* 0x3a */
} __attribute__((packed));

struct apm_bios_info {
	__u16   version;
	__u16   cseg;
	__u32   offset;
	__u16   cseg_16;
	__u16   dseg;
	__u16   flags;
	__u16   cseg_len;
	__u16   cseg_16_len;    
	__u16   dseg_len;
};

struct ist_info {
	__u32 signature;
	__u32 command;
	__u32 event;
	__u32 perf_level;
};

struct sys_desc_table {
	__u16 length;
	__u8  table[14];
};

struct efi_info {
	__u32 efi_loader_signature;
	__u32 efi_systab;
	__u32 efi_memdesc_size;
	__u32 efi_memdesc_version;
	__u32 efi_memmap;
	__u32 efi_memmap_size;
	__u32 efi_systab_hi;
	__u32 efi_memmap_hi;
};

struct setup_header {
	__u8	setup_sects;
	__u16	root_flags;
	__u32	syssize;
	__u16	ram_size;
	__u16	vid_mode;
	__u16	root_dev;
	__u16	boot_flag;
	__u16	jump;
	__u32	header;
	__u16	version;
	__u32	realmode_swtch;
	__u16	start_sys;
	__u16	kernel_version;
	__u8	type_of_loader;
	__u8	loadflags;
	__u16	setup_move_size;
	__u32	code32_start;
	__u32	ramdisk_image;
	__u32	ramdisk_size;
	__u32	bootsect_kludge;
	__u16	heap_end_ptr;
	__u16	_pad1;
	__u32	cmd_line_ptr;
	__u32	initrd_addr_max;
	__u32	kernel_alignment;
	__u8	relocatable_kernel;
	__u8	_pad2[3];
	__u32	cmdline_size;
	__u32	hardware_subarch;
	__u64	hardware_subarch_data;
	__u32	payload_offset;
	__u32	payload_length;
	__u64	setup_data;
} __attribute__((packed));

struct edid_info {
	unsigned char dummy[128];
};

struct e820entry { 
	__u64 addr;     /* start of memory segment */   
	__u64 size;     /* size of memory segment */    
	__u32 type;     /* type of memory segment */    
} __attribute__((packed));

struct edd_device_params {
	__u16 length;
	__u16 info_flags;
	__u32 num_default_cylinders;
	__u32 num_default_heads;
	__u32 sectors_per_track;
	__u64 number_of_sectors;
	__u16 bytes_per_sector;
	__u32 dpte_ptr;	 /* 0xFFFFFFFF for our purposes */
	__u16 key;	      /* = 0xBEDD */
	__u8 device_path_info_length;   /* = 44 */
	__u8 reserved2;
	__u16 reserved3;
	__u8 host_bus_type[4];
	__u8 interface_type[8];
	union {
		struct {
			__u16 base_address;
			__u16 reserved1;
			__u32 reserved2;
		} __attribute__ ((packed)) isa;
		struct {
			__u8 bus;
			__u8 slot;
			__u8 function;
			__u8 channel;
			__u32 reserved;
		} __attribute__ ((packed)) pci;
		/* pcix is same as pci */
		struct {
			__u64 reserved;
		} __attribute__ ((packed)) ibnd;
		struct {
			__u64 reserved;
		} __attribute__ ((packed)) xprs;
		struct {
			__u64 reserved;
		} __attribute__ ((packed)) htpt;
		struct {
			__u64 reserved;
		} __attribute__ ((packed)) unknown;
	} interface_path;
	union {
		struct {
			__u8 device;
			__u8 reserved1;
			__u16 reserved2;
			__u32 reserved3;
			__u64 reserved4;
		} __attribute__ ((packed)) ata;
		struct {
			__u8 device;
			__u8 lun;
			__u8 reserved1;
			__u8 reserved2;
			__u32 reserved3;
			__u64 reserved4;
		} __attribute__ ((packed)) atapi;
		struct {
			__u16 id;
			__u64 lun;
			__u16 reserved1;
			__u32 reserved2;
		} __attribute__ ((packed)) scsi;
		struct {
			__u64 serial_number;
			__u64 reserved;
		} __attribute__ ((packed)) usb;
		struct {
			__u64 eui;
			__u64 reserved;
		} __attribute__ ((packed)) i1394;
		struct {
			__u64 wwid;
			__u64 lun;
		} __attribute__ ((packed)) fibre;
		struct {
			__u64 identity_tag;
			__u64 reserved;
		} __attribute__ ((packed)) i2o;
	       struct {
			__u32 array_number;
			__u32 reserved1;
			__u64 reserved2;
		} __attribute__ ((packed)) raid;
		struct {
			__u8 device;
			__u8 reserved1;
			__u16 reserved2;
			__u32 reserved3;
			__u64 reserved4;
		} __attribute__ ((packed)) sata;
		struct {
			__u64 reserved1;
			__u64 reserved2;
		} __attribute__ ((packed)) unknown;
	} device_path;
	__u8 reserved4;
	__u8 checksum;
} __attribute__ ((packed));

struct edd_info {
	__u8 device;
	__u8 version;
	__u16 interface_support;
	__u16 legacy_max_cylinder;
	__u8 legacy_max_head;
	__u8 legacy_sectors_per_track;
	struct edd_device_params params;		
} __attribute__ ((packed)); 

/* The so-called "zeropage" */
struct boot_params {
	struct screen_info screen_info;			/* 0x000 */
	struct apm_bios_info apm_bios_info;		/* 0x040 */
	__u8  _pad2[12];				/* 0x054 */
	struct ist_info ist_info;			/* 0x060 */
	__u8  _pad3[16];				/* 0x070 */
	__u8  hd0_info[16];	/* obsolete! */		/* 0x080 */
	__u8  hd1_info[16];	/* obsolete! */		/* 0x090 */
	struct sys_desc_table sys_desc_table;		/* 0x0a0 */
	__u8  _pad4[144];				/* 0x0b0 */
	struct edid_info edid_info;			/* 0x140 */
	struct efi_info efi_info;			/* 0x1c0 */
	__u32 alt_mem_k;				/* 0x1e0 */
	__u32 scratch;		/* Scratch field! */	/* 0x1e4 */
	__u8  e820_entries;				/* 0x1e8 */
	__u8  eddbuf_entries;				/* 0x1e9 */
	__u8  edd_mbr_sig_buf_entries;			/* 0x1ea */
	__u8  _pad6[6];					/* 0x1eb */
	struct setup_header hdr;    /* setup header */	/* 0x1f1 */
	__u8  _pad7[0x290-0x1f1-sizeof(struct setup_header)];
	__u32 edd_mbr_sig_buffer[EDD_MBR_SIG_MAX];	/* 0x290 */
	struct e820entry e820_map[E820MAX];		/* 0x2d0 */
	__u8  _pad8[48];				/* 0xcd0 */
	struct edd_info eddbuf[EDDMAXNR];		/* 0xd00 */
	__u8  _pad9[276];				/* 0xeec */
} __attribute__((packed));
#define X86_SUBARCH_PC			0
#define X86_SUBARCH_LGUEST 		1
#define X86_SUBARCH_XEN			2
#define X86_SUBARCH_MRST		3

