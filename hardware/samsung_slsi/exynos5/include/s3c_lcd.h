/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _S3CFB_LCD_
#define _S3CFB_LCD_

/*
 * S T R U C T U R E S  F O R  C U S T O M  I O C T L S
 *
*/
struct s3cfb_user_window {
    int x;
    int y;
};

struct s3cfb_user_plane_alpha {
    int             channel;
    unsigned char   red;
    unsigned char   green;
    unsigned char   blue;
};

struct s3cfb_user_chroma {
    int             enabled;
    unsigned char   red;
    unsigned char   green;
    unsigned char   blue;
};

typedef struct {
    unsigned int phy_start_addr;
    unsigned int xres;      /* visible resolution*/
    unsigned int yres;
    unsigned int xres_virtual;  /* virtual resolution*/
    unsigned int yres_virtual;
    unsigned int xoffset;   /* offset from virtual to visible */
    unsigned int yoffset;   /* resolution   */
    unsigned int lcd_offset_x;
    unsigned int lcd_offset_y;
} s3c_fb_next_info_t;

struct s3c_fb_user_ion_client {
    int fd;
    int offset;
};

/*
 * C U S T O M  I O C T L S
 *
*/

#define S3CFB_WIN_POSITION          _IOW ('F', 203, struct s3cfb_user_window)
#define S3CFB_WIN_SET_PLANE_ALPHA   _IOW ('F', 204, struct s3cfb_user_plane_alpha)
#define S3CFB_WIN_SET_CHROMA        _IOW ('F', 205, struct s3cfb_user_chroma)
#define S3CFB_SET_VSYNC_INT         _IOW ('F', 206, unsigned int)
#define S3CFB_SET_SUSPEND_FIFO      _IOW ('F', 300, unsigned long)
#define S3CFB_SET_RESUME_FIFO       _IOW ('F', 301, unsigned long)
#define S3CFB_GET_LCD_WIDTH         _IOR ('F', 302, int)
#define S3CFB_GET_LCD_HEIGHT        _IOR ('F', 303, int)
#define S3CFB_GET_FB_PHY_ADDR       _IOR ('F', 310, unsigned int)
#define S3C_FB_GET_CURR_FB_INFO     _IOR ('F', 305, s3c_fb_next_info_t)
#define S3CFB_GET_ION_USER_HANDLE   _IOWR('F', 208, struct s3c_fb_user_ion_client)

/***************** LCD frame buffer *****************/
#define FB0_NAME    "/dev/fb0"
#define FB1_NAME    "/dev/fb1"
#define FB2_NAME    "/dev/fb2"
#define FB3_NAME    "/dev/fb3"
#define FB4_NAME    "/dev/fb4"

#endif
