/*
* Copyright (c) 2011, The Linux Foundation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*    * Redistributions of source code must retain the above copyright
*      notice, this list of conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above
*      copyright notice, this list of conditions and the following
*      disclaimer in the documentation and/or other materials provided
*      with the distribution.
*    * Neither the name of The Linux Foundation nor the names of its
*      contributors may be used to endorse or promote products derived
*      from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MDP_WRAPPER_H
#define MDP_WRAPPER_H

#define ATRACE_TAG (ATRACE_TAG_GRAPHICS | ATRACE_TAG_HAL)

/*
* In order to make overlay::mdp_wrapper shorter, please do something like:
* namespace mdpwrap = overlay::mdp_wrapper;
* */

#include <linux/msm_mdp.h>
#include <linux/msm_rotator.h>
#include <sys/ioctl.h>
#include <utils/Log.h>
#include <utils/Trace.h>
#include <errno.h>
#include "overlayUtils.h"

#define IOCTL_DEBUG 0

namespace overlay{

namespace mdp_wrapper{
/* FBIOGET_FSCREENINFO */
bool getFScreenInfo(int fd, fb_fix_screeninfo& finfo);

/* FBIOGET_VSCREENINFO */
bool getVScreenInfo(int fd, fb_var_screeninfo& vinfo);

/* FBIOPUT_VSCREENINFO */
bool setVScreenInfo(int fd, fb_var_screeninfo& vinfo);

/* MSM_ROTATOR_IOCTL_START */
bool startRotator(int fd, msm_rotator_img_info& rot);

/* MSM_ROTATOR_IOCTL_ROTATE */
bool rotate(int fd, msm_rotator_data_info& rot);

/* MSMFB_OVERLAY_SET */
bool setOverlay(int fd, mdp_overlay& ov);

/* MSMFB_OVERLAY_PREPARE */
bool validateAndSet(const int& fd, mdp_overlay_list& list);

/* MSM_ROTATOR_IOCTL_FINISH */
bool endRotator(int fd, int sessionId);

/* MSMFB_OVERLAY_UNSET */
bool unsetOverlay(int fd, int ovId);

/* MSMFB_OVERLAY_GET */
bool getOverlay(int fd, mdp_overlay& ov);

/* MSMFB_OVERLAY_PLAY */
bool play(int fd, msmfb_overlay_data& od);

/* MSMFB_DISPLAY_COMMIT */
bool displayCommit(int fd);

/* MSMFB_WRITEBACK_INIT, MSMFB_WRITEBACK_START */
bool wbInitStart(int fbfd);

/* MSMFB_WRITEBACK_STOP, MSMFB_WRITEBACK_TERMINATE */
bool wbStopTerminate(int fbfd);

/* MSMFB_WRITEBACK_QUEUE_BUFFER */
bool wbQueueBuffer(int fbfd, struct msmfb_data& fbData);

/* MSMFB_WRITEBACK_DEQUEUE_BUFFER */
bool wbDequeueBuffer(int fbfd, struct msmfb_data& fbData);

/* the following are helper functions for dumping
 * msm_mdp and friends*/
void dump(const char* const s, const msmfb_overlay_data& ov);
void dump(const char* const s, const msmfb_data& ov);
void dump(const char* const s, const mdp_overlay& ov);
void dump(const char* const s, const msmfb_overlay_3d& ov);
void dump(const char* const s, const uint32_t u[], uint32_t cnt);
void dump(const char* const s, const msmfb_img& ov);
void dump(const char* const s, const mdp_rect& ov);

/* and rotator */
void dump(const char* const s, const msm_rotator_img_info& rot);
void dump(const char* const s, const msm_rotator_data_info& rot);

/* info */
void dump(const char* const s, const fb_fix_screeninfo& finfo);
void dump(const char* const s, const fb_var_screeninfo& vinfo);

//---------------Inlines -------------------------------------

inline bool getFScreenInfo(int fd, fb_fix_screeninfo& finfo) {
    ATRACE_CALL();
    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) < 0) {
        ALOGE("Failed to call ioctl FBIOGET_FSCREENINFO err=%s",
                strerror(errno));
        return false;
    }
    return true;
}

inline bool getVScreenInfo(int fd, fb_var_screeninfo& vinfo) {
    ATRACE_CALL();
    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        ALOGE("Failed to call ioctl FBIOGET_VSCREENINFO err=%s",
                strerror(errno));
        return false;
    }
    return true;
}

inline bool setVScreenInfo(int fd, fb_var_screeninfo& vinfo) {
    ATRACE_CALL();
    if (ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo) < 0) {
        ALOGE("Failed to call ioctl FBIOPUT_VSCREENINFO err=%s",
                strerror(errno));
        return false;
    }
    return true;
}

inline bool startRotator(int fd, msm_rotator_img_info& rot) {
    ATRACE_CALL();
    if (ioctl(fd, MSM_ROTATOR_IOCTL_START, &rot) < 0){
        ALOGE("Failed to call ioctl MSM_ROTATOR_IOCTL_START err=%s",
                strerror(errno));
        return false;
    }
    return true;
}

inline bool rotate(int fd, msm_rotator_data_info& rot) {
    ATRACE_CALL();
    if (ioctl(fd, MSM_ROTATOR_IOCTL_ROTATE, &rot) < 0) {
        ALOGE("Failed to call ioctl MSM_ROTATOR_IOCTL_ROTATE err=%s",
                strerror(errno));
        return false;
    }
    return true;
}

inline bool setOverlay(int fd, mdp_overlay& ov) {
    ATRACE_CALL();
    if (ioctl(fd, MSMFB_OVERLAY_SET, &ov) < 0) {
        ALOGE("Failed to call ioctl MSMFB_OVERLAY_SET err=%s",
                strerror(errno));
        return false;
    }
    return true;
}

inline bool validateAndSet(const int& fd, mdp_overlay_list& list) {
    ATRACE_CALL();
    if (ioctl(fd, MSMFB_OVERLAY_PREPARE, &list) < 0) {
        ALOGD_IF(IOCTL_DEBUG, "Failed to call ioctl MSMFB_OVERLAY_PREPARE "
                "err=%s", strerror(errno));
        return false;
    }
    return true;
}

inline bool endRotator(int fd, uint32_t sessionId) {
    ATRACE_CALL();
    if (ioctl(fd, MSM_ROTATOR_IOCTL_FINISH, &sessionId) < 0) {
        ALOGE("Failed to call ioctl MSM_ROTATOR_IOCTL_FINISH err=%s",
                strerror(errno));
        return false;
    }
    return true;
}

inline bool unsetOverlay(int fd, int ovId) {
    ATRACE_CALL();
    if (ioctl(fd, MSMFB_OVERLAY_UNSET, &ovId) < 0) {
        ALOGE("Failed to call ioctl MSMFB_OVERLAY_UNSET err=%s",
                strerror(errno));
        return false;
    }
    return true;
}

inline bool getOverlay(int fd, mdp_overlay& ov) {
    ATRACE_CALL();
    if (ioctl(fd, MSMFB_OVERLAY_GET, &ov) < 0) {
        ALOGE("Failed to call ioctl MSMFB_OVERLAY_GET err=%s",
                strerror(errno));
        return false;
    }
    return true;
}

inline bool play(int fd, msmfb_overlay_data& od) {
    ATRACE_CALL();
    if (ioctl(fd, MSMFB_OVERLAY_PLAY, &od) < 0) {
        ALOGE("Failed to call ioctl MSMFB_OVERLAY_PLAY err=%s",
                strerror(errno));
        return false;
    }
    return true;
}

inline bool displayCommit(int fd, mdp_display_commit& info) {
    ATRACE_CALL();
    if(ioctl(fd, MSMFB_DISPLAY_COMMIT, &info) == -1) {
        ALOGE("Failed to call ioctl MSMFB_DISPLAY_COMMIT err=%s",
                strerror(errno));
        return false;
    }
    return true;
}

inline bool wbInitStart(int fbfd) {
    ATRACE_CALL();
    if(ioctl(fbfd, MSMFB_WRITEBACK_INIT, NULL) < 0) {
        ALOGE("Failed to call ioctl MSMFB_WRITEBACK_INIT err=%s",
                strerror(errno));
        return false;
    }
    if(ioctl(fbfd, MSMFB_WRITEBACK_START, NULL) < 0) {
        ALOGE("Failed to call ioctl MSMFB_WRITEBACK_START err=%s",
                strerror(errno));
        return false;
    }
    return true;
}

inline bool wbStopTerminate(int fbfd) {
    ATRACE_CALL();
    if(ioctl(fbfd, MSMFB_WRITEBACK_STOP, NULL) < 0) {
        ALOGE("Failed to call ioctl MSMFB_WRITEBACK_STOP err=%s",
                strerror(errno));
        return false;
    }
    if(ioctl(fbfd, MSMFB_WRITEBACK_TERMINATE, NULL) < 0) {
        ALOGE("Failed to call ioctl MSMFB_WRITEBACK_TERMINATE err=%s",
                strerror(errno));
        return false;
    }
    return true;
}

inline bool wbQueueBuffer(int fbfd, struct msmfb_data& fbData) {
    ATRACE_CALL();
    if(ioctl(fbfd, MSMFB_WRITEBACK_QUEUE_BUFFER, &fbData) < 0) {
        ALOGE("Failed to call ioctl MSMFB_WRITEBACK_QUEUE_BUFFER err=%s",
                strerror(errno));
        return false;
    }
    return true;
}

inline bool wbDequeueBuffer(int fbfd, struct msmfb_data& fbData) {
    ATRACE_CALL();
    if(ioctl(fbfd, MSMFB_WRITEBACK_DEQUEUE_BUFFER, &fbData) < 0) {
        ALOGE("Failed to call ioctl MSMFB_WRITEBACK_DEQUEUE_BUFFER err=%s",
                strerror(errno));
        return false;
    }
    return true;
}

/* dump funcs */
inline void dump(const char* const s, const msmfb_overlay_data& ov) {
    ALOGE("%s msmfb_overlay_data id=%d",
            s, ov.id);
    dump("data", ov.data);
}
inline void dump(const char* const s, const msmfb_data& ov) {
    ALOGE("%s msmfb_data offset=%d memid=%d id=%d flags=0x%x priv=%d",
            s, ov.offset, ov.memory_id, ov.id, ov.flags, ov.priv);
}
inline void dump(const char* const s, const mdp_overlay& ov) {
    ALOGE("%s mdp_overlay z=%d fg=%d alpha=%d mask=%d flags=0x%x id=%d",
            s, ov.z_order, ov.is_fg, ov.alpha,
            ov.transp_mask, ov.flags, ov.id);
    dump("src", ov.src);
    dump("src_rect", ov.src_rect);
    dump("dst_rect", ov.dst_rect);
    /*
    Commented off to prevent verbose logging, since user_data could have 8 or so
    fields which are mostly 0
    dump("user_data", ov.user_data,
            sizeof(ov.user_data)/sizeof(ov.user_data[0]));
    */
}
inline void dump(const char* const s, const msmfb_img& ov) {
    ALOGE("%s msmfb_img w=%d h=%d format=%d %s",
            s, ov.width, ov.height, ov.format,
            overlay::utils::getFormatString(ov.format));
}
inline void dump(const char* const s, const mdp_rect& ov) {
    ALOGE("%s mdp_rect x=%d y=%d w=%d h=%d",
            s, ov.x, ov.y, ov.w, ov.h);
}

inline void dump(const char* const s, const msmfb_overlay_3d& ov) {
    ALOGE("%s msmfb_overlay_3d 3d=%d w=%d h=%d",
            s, ov.is_3d, ov.width, ov.height);

}
inline void dump(const char* const s, const uint32_t u[], uint32_t cnt) {
    ALOGE("%s user_data cnt=%d", s, cnt);
    for(uint32_t i=0; i < cnt; ++i) {
        ALOGE("i=%d val=%d", i, u[i]);
    }
}
inline void dump(const char* const s, const msm_rotator_img_info& rot) {
    ALOGE("%s msm_rotator_img_info sessid=%u dstx=%d dsty=%d rot=%d, ena=%d scale=%d",
            s, rot.session_id, rot.dst_x, rot.dst_y,
            rot.rotations, rot.enable, rot.downscale_ratio);
    dump("src", rot.src);
    dump("dst", rot.dst);
    dump("src_rect", rot.src_rect);
}
inline void dump(const char* const s, const msm_rotator_data_info& rot) {
    ALOGE("%s msm_rotator_data_info sessid=%u verkey=%d",
            s, rot.session_id, rot.version_key);
    dump("src", rot.src);
    dump("dst", rot.dst);
    dump("src_chroma", rot.src_chroma);
    dump("dst_chroma", rot.dst_chroma);
}
inline void dump(const char* const s, const fb_fix_screeninfo& finfo) {
    ALOGE("%s fb_fix_screeninfo type=%d", s, finfo.type);
}
inline void dump(const char* const s, const fb_var_screeninfo& vinfo) {
    ALOGE("%s fb_var_screeninfo xres=%d yres=%d",
            s, vinfo.xres, vinfo.yres);
}

} // mdp_wrapper

} // overlay

#endif // MDP_WRAPPER_H
