#include "NV12_resize.h"

//#define LOG_NDEBUG 0
#define LOG_NIDEBUG 0
#define LOG_NDDEBUG 0

#define LOG_TAG "NV12_resize"
#define STRIDE 4096
#include <utils/Log.h>

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
 )
{
  ALOGV("VT_resizeFrame_Video_opt2_lp+");

  mmUint16 row,col;
  mmUint32 resizeFactorX;
  mmUint32 resizeFactorY;


  mmUint16 x, y;

  mmUchar* ptr8;
  mmUchar *ptr8Cb, *ptr8Cr;


  mmUint16 xf, yf;
  mmUchar* inImgPtrY;
  mmUchar* inImgPtrU;
  mmUchar* inImgPtrV;
  mmUint32 cox, coy, codx, cody;
  mmUint16 idx,idy, idxC;

  if(i_img_ptr->uWidth == o_img_ptr->uWidth)
	{
		if(i_img_ptr->uHeight == o_img_ptr->uHeight)
			{
				ALOGV("************************f(i_img_ptr->uHeight == o_img_ptr->uHeight) are same *********************\n");
				ALOGV("************************(i_img_ptr->width == %d" , i_img_ptr->uWidth );
				ALOGV("************************(i_img_ptr->uHeight == %d" , i_img_ptr->uHeight );
				ALOGV("************************(o_img_ptr->width == %d" ,o_img_ptr->uWidth );
				ALOGV("************************(o_img_ptr->uHeight == %d" , o_img_ptr->uHeight );
			}
	}

  if (!i_img_ptr || !i_img_ptr->imgPtr ||
    !o_img_ptr || !o_img_ptr->imgPtr)
  {
	ALOGE("Image Point NULL");
	ALOGV("VT_resizeFrame_Video_opt2_lp-");
	return FALSE;
  }

  inImgPtrY = (mmUchar *) i_img_ptr->imgPtr + i_img_ptr->uOffset;
  inImgPtrU = (mmUchar *) i_img_ptr->clrPtr + i_img_ptr->uOffset/2;
  inImgPtrV = (mmUchar*)inImgPtrU + 1;

  if (cropout == NULL)
  {
    cox = 0;
    coy = 0;
    codx = o_img_ptr->uWidth;
    cody = o_img_ptr->uHeight;
  }
  else
  {
    cox = cropout->x;
    coy = cropout->y;
    codx = cropout->uWidth;
    cody = cropout->uHeight;
  }
  idx = i_img_ptr->uWidth;
  idy = i_img_ptr->uHeight;

  /* make sure valid input size */
  if (idx < 1 || idy < 1 || i_img_ptr->uStride < 1)
	{
	ALOGE("idx or idy less then 1 idx = %d idy = %d stride = %d", idx, idy, i_img_ptr->uStride);
	ALOGV("VT_resizeFrame_Video_opt2_lp-");
	return FALSE;
	}

  resizeFactorX = ((idx-1)<<9) / codx;
  resizeFactorY = ((idy-1)<<9) / cody;

  if(i_img_ptr->eFormat == IC_FORMAT_YCbCr420_lp &&
    o_img_ptr->eFormat == IC_FORMAT_YCbCr420_lp)
  {
    ptr8 = (mmUchar*)o_img_ptr->imgPtr + cox + coy*o_img_ptr->uWidth;


    ////////////////////////////for Y//////////////////////////
    for (row=0; row < cody; row++)
    {
        mmUchar *pu8Yrow1 = NULL;
        mmUchar *pu8Yrow2 = NULL;
        y  = (mmUint16) ((mmUint32) (row*resizeFactorY) >> 9);
        yf = (mmUchar)  ((mmUint32)((row*resizeFactorY) >> 6) & 0x7);
        pu8Yrow1 = inImgPtrY + (y) * i_img_ptr->uStride;
        pu8Yrow2 = pu8Yrow1 + i_img_ptr->uStride;

        for (col=0; col < codx; col++)
        {
            mmUchar in11, in12, in21, in22;
            mmUchar *pu8ptr1 = NULL;
            mmUchar *pu8ptr2 = NULL;
            mmUchar w;
            mmUint16 accum_1;
            //mmUint32 accum_W;



            x  = (mmUint16) ((mmUint32)  (col*resizeFactorX) >> 9);
            xf = (mmUchar)  ((mmUint32) ((col*resizeFactorX) >> 6) & 0x7);


            //accum_W = 0;
            accum_1 =  0;

            pu8ptr1 = pu8Yrow1 + (x);
            pu8ptr2 = pu8Yrow2 + (x);

            /* A pixel */
            //in = *(inImgPtrY + (y)*idx + (x));
            in11 = *(pu8ptr1);

            w = bWeights[xf][yf][0];
            accum_1 = (w * in11);
            //accum_W += (w);

            /* B pixel */
            //in = *(inImgPtrY + (y)*idx + (x+1));
            in12 = *(pu8ptr1+1);
            w = bWeights[xf][yf][1];
            accum_1 += (w * in12);
            //accum_W += (w);

            /* C pixel */
            //in = *(inImgPtrY + (y+1)*idx + (x));
            in21 = *(pu8ptr2);
            w = bWeights[xf][yf][3];
            accum_1 += (w * in21);
            //accum_W += (w);

            /* D pixel */
            //in = *(inImgPtrY + (y+1)*idx + (x+1));
            in22 = *(pu8ptr2+1);
            w = bWeights[xf][yf][2];
            accum_1 += (w * in22);
            //accum_W += (w);

            /* divide by sum of the weights */
            //accum_1 /= (accum_W);
            //accum_1 = (accum_1/64);
            accum_1 = (accum_1>>6);
            *ptr8 = (mmUchar)accum_1 ;


            ptr8++;
        }
        ptr8 = ptr8 + (o_img_ptr->uStride - codx);
    }
    ////////////////////////////for Y//////////////////////////

    ///////////////////////////////for Cb-Cr//////////////////////

    ptr8Cb = (mmUchar*)o_img_ptr->clrPtr + cox + coy*o_img_ptr->uWidth;

    ptr8Cr = (mmUchar*)(ptr8Cb+1);

    idxC = (idx>>1);
    for (row=0; row < (((cody)>>1)); row++)
    {
        mmUchar *pu8Cbr1 = NULL;
        mmUchar *pu8Cbr2 = NULL;
        mmUchar *pu8Crr1 = NULL;
        mmUchar *pu8Crr2 = NULL;

        y  = (mmUint16) ((mmUint32) (row*resizeFactorY) >> 9);
        yf = (mmUchar)  ((mmUint32)((row*resizeFactorY) >> 6) & 0x7);

        pu8Cbr1 = inImgPtrU + (y) * i_img_ptr->uStride;
        pu8Cbr2 = pu8Cbr1 + i_img_ptr->uStride;
        pu8Crr1 = inImgPtrV + (y) * i_img_ptr->uStride;
        pu8Crr2 = pu8Crr1 + i_img_ptr->uStride;

        for (col=0; col < (((codx)>>1)); col++)
        {
            mmUchar in11, in12, in21, in22;
            mmUchar *pu8Cbc1 = NULL;
            mmUchar *pu8Cbc2 = NULL;
            mmUchar *pu8Crc1 = NULL;
            mmUchar *pu8Crc2 = NULL;

            mmUchar w;
            mmUint16 accum_1Cb, accum_1Cr;
            //mmUint32 accum_WCb, accum_WCr;


            x  = (mmUint16) ((mmUint32)  (col*resizeFactorX) >> 9);
            xf = (mmUchar)  ((mmUint32) ((col*resizeFactorX) >> 6) & 0x7);


            //accum_WCb = accum_WCr =  0;
            accum_1Cb = accum_1Cr =  0;

            pu8Cbc1 = pu8Cbr1 + (x*2);
            pu8Cbc2 = pu8Cbr2 + (x*2);
	    pu8Crc1 = pu8Crr1 + (x*2);
            pu8Crc2 = pu8Crr2 + (x*2);



            /* A pixel */
            w = bWeights[xf][yf][0];

            in11 = *(pu8Cbc1);
            accum_1Cb = (w * in11);
            //    accum_WCb += (w);

			in11 = *(pu8Crc1);
            accum_1Cr = (w * in11);
            //accum_WCr += (w);

            /* B pixel */
            w = bWeights[xf][yf][1];

            in12 = *(pu8Cbc1+2);
            accum_1Cb += (w * in12);
            //accum_WCb += (w);

            in12 = *(pu8Crc1+2);
            accum_1Cr += (w * in12);
            //accum_WCr += (w);

            /* C pixel */
            w = bWeights[xf][yf][3];

            in21 = *(pu8Cbc2);
            accum_1Cb += (w * in21);
            //accum_WCb += (w);

			in21 = *(pu8Crc2);
            accum_1Cr += (w * in21);
            //accum_WCr += (w);

            /* D pixel */
            w = bWeights[xf][yf][2];

            in22 = *(pu8Cbc2+2);
            accum_1Cb += (w * in22);
            //accum_WCb += (w);

            in22 = *(pu8Crc2+2);
            accum_1Cr += (w * in22);
            //accum_WCr += (w);

            /* divide by sum of the weights */
            //accum_1Cb /= (accum_WCb);
            accum_1Cb = (accum_1Cb>>6);
            *ptr8Cb = (mmUchar)accum_1Cb ;


            accum_1Cr = (accum_1Cr >> 6);
            *ptr8Cr = (mmUchar)accum_1Cr ;

            ptr8Cb++;
            ptr8Cr++;

            ptr8Cb++;
            ptr8Cr++;
        }
        ptr8Cb = ptr8Cb + (o_img_ptr->uStride-codx);
        ptr8Cr = ptr8Cr + (o_img_ptr->uStride-codx);
    }
    ///////////////////For Cb- Cr////////////////////////////////////////
  }
  else
  {
	ALOGE("eFormat not supported");
	ALOGV("VT_resizeFrame_Video_opt2_lp-");
	return FALSE;
  }
  ALOGV("success");
  ALOGV("VT_resizeFrame_Video_opt2_lp-");
  return TRUE;
}
