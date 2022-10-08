/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef KCT_H_
#define KCT_H_
#include <linux/netlink.h>
#define EV_FLAGS_PRIORITY_LOW (1<<0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#ifndef MAX_SB_N
#define MAX_SB_N 32
#endif
#ifndef MAX_EV_N
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define MAX_EV_N 32
#endif
#define NETLINK_CRASHTOOL 27
#define ATTCHMT_ALIGN 4U
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
enum ct_ev_type {
 CT_EV_STAT,
 CT_EV_INFO,
 CT_EV_ERROR,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CT_EV_CRASH,
 CT_EV_LAST
};
enum ct_attchmt_type {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CT_ATTCHMT_DATA0,
 CT_ATTCHMT_DATA1,
 CT_ATTCHMT_DATA2,
 CT_ATTCHMT_DATA3,
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 CT_ATTCHMT_DATA4,
 CT_ATTCHMT_DATA5,
 CT_ATTCHMT_BINARY,
 CT_ATTCHMT_FILELIST
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
};
struct ct_attchmt {
 __u32 size;
 enum ct_attchmt_type type;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 char data[];
} __aligned(4);
struct ct_event {
 __u64 timestamp;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 char submitter_name[MAX_SB_N];
 char ev_name[MAX_EV_N];
 enum ct_ev_type type;
 __u32 attchmt_size;
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 __u32 flags;
 struct ct_attchmt attachments[];
} __aligned(4);
enum kct_nlmsg_type {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 KCT_EVENT,
 KCT_SET_PID = 4200,
};
struct kct_packet {
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
 struct nlmsghdr nlh;
 struct ct_event event;
};
#define ATTCHMT_ALIGNMENT 4
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#ifndef KCT_ALIGN
#define __KCT_ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define __KCT_ALIGN(x, a) __KCT_ALIGN_MASK(x, (typeof(x))(a) - 1)
#define KCT_ALIGN(x, a) __KCT_ALIGN((x), (a))
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#endif
#define foreach_attchmt(Event, Attchmt)   if ((Event)->attchmt_size)   for ((Attchmt) = (Event)->attachments;   (Attchmt) < (typeof(Attchmt))(((char *)   (Event)->attachments) +   (Event)->attchmt_size);   (Attchmt) = (typeof(Attchmt))KCT_ALIGN(((size_t)(Attchmt))   + sizeof(*(Attchmt)) +   (Attchmt)->size, ATTCHMT_ALIGNMENT))
#define MKFN(fn, ...) MKFN_N(fn, ##__VA_ARGS__, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)(__VA_ARGS__)
#define MKFN_N(fn, n0, n1, n2, n3, n4, n5, n6, n7, n8, n9, n10, n11, n, ...) fn##n
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define kct_log(...) MKFN(__kct_log_, ##__VA_ARGS__)
#define __kct_log_4(Type, Submitter_name, Ev_name, flags)   do { if (kct_alloc_event) {   struct ct_event *__ev =   kct_alloc_event(Submitter_name, Ev_name, Type,   GFP_ATOMIC, flags);   if (__ev) {   kct_log_event(__ev, GFP_ATOMIC);   }   } } while (0)
#define __kct_log_5(Type, Submitter_name, Ev_name, flags, Data0)   do { if (kct_alloc_event) {   struct ct_event *__ev =   kct_alloc_event(Submitter_name, Ev_name, Type,   GFP_ATOMIC, flags);   if (__ev) {   if (Data0)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA0,   strlen(Data0) + 1, Data0, GFP_ATOMIC);   kct_log_event(__ev, GFP_ATOMIC);   }   } } while (0)
#define __kct_log_6(Type, Submitter_name, Ev_name, flags, Data0, Data1)   do { if (kct_alloc_event) {   struct ct_event *__ev =   kct_alloc_event(Submitter_name, Ev_name, Type,   GFP_ATOMIC, flags);   if (__ev) {   if (Data0)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA0,   strlen(Data0) + 1, Data0, GFP_ATOMIC);   if (Data1)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA1,   strlen(Data1) + 1, Data1, GFP_ATOMIC);   kct_log_event(__ev, GFP_ATOMIC);   }   } } while (0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define __kct_log_7(Type, Submitter_name, Ev_name, flags, Data0, Data1, Data2)   do { if (kct_alloc_event) {   struct ct_event *__ev =   kct_alloc_event(Submitter_name, Ev_name, Type,   GFP_ATOMIC, flags);   if (__ev) {   if (Data0)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA0,   strlen(Data0) + 1, Data0, GFP_ATOMIC);   if (Data1)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA1,   strlen(Data1) + 1, Data1, GFP_ATOMIC);   if (Data2)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA2,   strlen(Data2) + 1, Data2, GFP_ATOMIC);   kct_log_event(__ev, GFP_ATOMIC);   }   } } while (0)
#define __kct_log_8(Type, Submitter_name, Ev_name, flags, Data0, Data1, Data2,   Data3)   do { if (kct_alloc_event) {   struct ct_event *__ev =   kct_alloc_event(Submitter_name, Ev_name, Type,   GFP_ATOMIC, flags);   if (__ev) {   if (Data0)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA0,   strlen(Data0) + 1, Data0, GFP_ATOMIC);   if (Data1)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA1,   strlen(Data1) + 1, Data1, GFP_ATOMIC);   if (Data2)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA2,   strlen(Data2) + 1, Data2, GFP_ATOMIC);   if (Data3)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA3,   strlen(Data3) + 1, Data3, GFP_ATOMIC);   kct_log_event(__ev, GFP_ATOMIC);   }   } } while (0)
#define __kct_log_9(Type, Submitter_name, Ev_name, flags, Data0, Data1, Data2,   Data3, Data4)   do { if (kct_alloc_event) {   struct ct_event *__ev =   kct_alloc_event(Submitter_name, Ev_name, Type,   GFP_ATOMIC, flags);   if (__ev) {   if (Data0)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA0,   strlen(Data0) + 1, Data0, GFP_ATOMIC);   if (Data1)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA1,   strlen(Data1) + 1, Data1, GFP_ATOMIC);   if (Data2)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA2,   strlen(Data2) + 1, Data2, GFP_ATOMIC);   if (Data3)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA3,   strlen(Data3) + 1, Data3, GFP_ATOMIC);   if (Data4)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA4,   strlen(Data4) + 1, Data4, GFP_ATOMIC);   kct_log_event(__ev, GFP_ATOMIC);   }   } } while (0)
#define __kct_log_10(Type, Submitter_name, Ev_name, flags, Data0, Data1, Data2,   Data3, Data4, Data5)   do { if (kct_alloc_event) {   struct ct_event *__ev =   kct_alloc_event(Submitter_name, Ev_name, Type,   GFP_ATOMIC, flags);   if (__ev) {   if (Data0)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA0,   strlen(Data0) + 1, Data0, GFP_ATOMIC);   if (Data1)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA1,   strlen(Data1) + 1, Data1, GFP_ATOMIC);   if (Data2)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA2,   strlen(Data2) + 1, Data2, GFP_ATOMIC);   if (Data3)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA3,   strlen(Data3) + 1, Data3, GFP_ATOMIC);   if (Data4)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA4,   strlen(Data4) + 1, Data4, GFP_ATOMIC);   if (Data5)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA5,   strlen(Data5) + 1, Data5, GFP_ATOMIC);   kct_log_event(__ev, GFP_ATOMIC);   }   } } while (0)
/* WARNING: DO NOT EDIT, AUTO-GENERATED CODE - SEE TOP FOR INSTRUCTIONS */
#define __kct_log_11(Type, Submitter_name, Ev_name, flags, Data0, Data1, Data2,   Data3, Data4, Data5, filelist)   do { if (kct_alloc_event) {   struct ct_event *__ev =   kct_alloc_event(Submitter_name, Ev_name, Type,   GFP_ATOMIC, flags);   if (__ev) {   if (Data0)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA0,   strlen(Data0) + 1, Data0, GFP_ATOMIC);   if (Data1)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA1,   strlen(Data1) + 1, Data1, GFP_ATOMIC);   if (Data2)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA2,   strlen(Data2) + 1, Data2, GFP_ATOMIC);   if (Data3)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA3,   strlen(Data3) + 1, Data3, GFP_ATOMIC);   if (Data4)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA4,   strlen(Data4) + 1, Data4, GFP_ATOMIC);   if (Data5)   kct_add_attchmt(&__ev, CT_ATTCHMT_DATA5,   strlen(Data5) + 1, Data5, GFP_ATOMIC);   if (filelist)   kct_add_attchmt(&__ev, CT_ATTCHMT_FILELIST,   strlen(filelist) + 1, filelist, GFP_ATOMIC);   kct_log_event(__ev, GFP_ATOMIC);   }   } } while (0)
#endif

