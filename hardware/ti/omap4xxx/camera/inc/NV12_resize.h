#ifndef NV12_RESIZE_H_
#define NV12_RESIZE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char       mmBool;
typedef unsigned char       mmUchar;
typedef unsigned char       mmUint8;
typedef unsigned char       mmByte;
typedef unsigned short      mmUint16;
typedef unsigned int        mmUint32;
typedef unsigned long       mmUint64;
typedef signed char         mmInt8;
typedef char		        mmChar;
typedef signed short        mmInt16;
typedef signed int          mmInt32;
typedef signed long         mmLong;
typedef signed int          mmHandle;
typedef float        mmFloat;
typedef double       mmDouble;
typedef int 		    HObj;
typedef HObj		    HFile;
typedef int 		    HDir;
typedef void* mmMutexHandle;
typedef struct _fstat
{
      mmInt32 fileSize;
}VE_FileAttribute;

typedef struct
{
	mmInt32		second;
	mmInt32 	millisecond;
}tsVE_Time;

typedef struct
{
	mmInt32 	year;
	mmInt32 	month;
	mmInt32 	day;
	mmInt32 	hour;
	mmInt32 	minute;
	mmInt32 	second;
} TmDateTime;

/*----------------------------------------------------------------------------
    Define : TRUE/FALSE for boolean operations
----------------------------------------------------------------------------*/

#ifndef TRUE
    #define TRUE    1
#endif

#ifndef FALSE
    #define FALSE   0
#endif

#ifndef NULL
   #define NULL        0
#endif

const mmUint8 bWeights[8][8][4] = {
  {{64, 0, 0, 0}, {56, 0, 0, 8}, {48, 0, 0,16}, {40, 0, 0,24},
   {32, 0, 0,32}, {24, 0, 0,40}, {16, 0, 0,48}, { 8, 0, 0,56}},

  {{56, 8, 0, 0}, {49, 7, 1, 7}, {42, 6, 2,14}, {35, 5, 3,21},
   {28, 4, 4,28}, {21, 3, 5,35}, {14, 2, 6,42}, { 7, 1, 7,49}},

  {{48,16, 0, 0}, {42,14, 2, 6}, {36,12,4 ,12}, {30,10,6 ,18},
   {24, 8, 8,24}, {18, 6,10,30}, {12,4 ,12,36}, { 6, 2,14,42}},

  {{40,24,0 ,0 }, {35,21, 3, 5}, {30,18, 6,10}, {25,15, 9,15},
   {20,12,12,20}, {15, 9,15,25}, {10, 6,18,30}, { 5, 3,21,35}},

  {{32,32, 0,0 }, {28,28, 4, 4}, {24,24, 8, 8}, {20,20,12,12},
   {16,16,16,16}, {12,12,20,20}, { 8, 8,24,24}, { 4, 4,28,28}},

  {{24,40,0 ,0 }, {21,35, 5, 3}, {18,30,10, 6}, {15,25,15, 9},
   {12,20,20,12}, { 9,15,25,15}, { 6,10,30,18}, { 3, 5,35,21}},

  {{16,48, 0,0 }, {14,42, 6, 2}, {12,36,12, 4}, {10,30,18, 6},
   {8 ,24,24,8 }, { 6,18,30,10}, { 4,12,36,12}, { 2, 6,42,14}},

  {{ 8,56, 0,0 }, { 7,49, 7, 1}, { 6,42,14, 2}, { 5,35,21, 3},
   { 4,28,28,4 }, { 3,21,35, 5}, { 2,14,42, 6}, { 1,7 ,49, 7}}
};

typedef enum
{
    IC_FORMAT_NONE,
    IC_FORMAT_RGB565,
    IC_FORMAT_RGB888,
    IC_FORMAT_YCbCr420_lp,
    IC_FORMAT_YCbCr,
    IC_FORMAT_YCbCr420_FRAME_PK,
    IC_FORMAT_MAX
}enumImageFormat;

/* This structure defines the format of an image */
typedef struct
{
  mmInt32                       uWidth;
  mmInt32                       uHeight;
  mmInt32                       uStride;
  enumImageFormat               eFormat;
  mmByte                        *imgPtr;
  mmByte                        *clrPtr;
  mmInt32                       uOffset;
} structConvImage;

typedef struct IC_crop_struct
{
  mmUint32 x;             /* x pos of rectangle                              */
  mmUint32 y;             /* y pos of rectangle                              */
  mmUint32 uWidth;        /* dx of rectangle                                 */
  mmUint32 uHeight;       /* dy of rectangle                                 */
} IC_rect_type;

/*==========================================================================
* Function Name  : VT_resizeFrame_Video_opt2_lp
*
* Description    : Resize a yuv frame.
*
* Input(s)       : input_img_ptr        -> Input Image Structure
*                : output_img_ptr       -> Output Image Structure
*                : cropout             -> crop structure
*
* Value Returned : mmBool               -> FALSE on error TRUE on success
* NOTE:
*            Not tested for crop funtionallity.
*            faster version.
============================================================================*/
mmBool
VT_resizeFrame_Video_opt2_lp
(
 structConvImage* i_img_ptr,        /* Points to the input image           */
 structConvImage* o_img_ptr,        /* Points to the output image          */
 IC_rect_type*  cropout,          /* how much to resize to in final image */
 mmUint16 dummy                         /* Transparent pixel value              */
 );

#ifdef __cplusplus
}
#endif

#endif //#define NV12_RESIZE_H_
