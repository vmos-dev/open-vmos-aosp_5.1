/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
* Redistribution and use in source and binary forms, with or without
* * modification, are permitted provided that the following conditions are
* met:
*   * Redistributions of source code must retain the above copyrigh
*     notice, this list of conditions and the following disclaimer
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of The Linux Foundation nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "cb_utils.h"
#include "cb_swap_rect.h"
/* get union of two rects into 3rd rect */
void getUnion(hwc_rect_t& rect1,hwc_rect_t& rect2, hwc_rect_t& irect) {

    irect.left   = min(rect1.left, rect2.left);
    irect.top    = min(rect1.top, rect2.top);
    irect.right  = max(rect1.right, rect2.right);
    irect.bottom = max(rect1.bottom, rect2.bottom);
}

using namespace android;
using namespace qhwc;
namespace qdutils {

int CBUtils::getuiClearRegion(hwc_display_contents_1_t* list,
          hwc_rect_t &clearWormholeRect, LayerProp *layerProp) {

    size_t last = list->numHwLayers - 1;
    hwc_rect_t fbFrame = list->hwLayers[last].displayFrame;
    Rect fbFrameRect(fbFrame.left,fbFrame.top,fbFrame.right,fbFrame.bottom);
    Region wormholeRegion(fbFrameRect);

    if(cb_swap_rect::getInstance().checkSwapRectFeature_on() == true){
      wormholeRegion.set(0,0);
      for(size_t i = 0 ; i < last; i++) {
         if((list->hwLayers[i].blending == HWC_BLENDING_NONE) ||
           !(layerProp[i].mFlags & HWC_COPYBIT) ||
           (list->hwLayers[i].flags  & HWC_SKIP_HWC_COMPOSITION))
              continue ;
         hwc_rect_t displayFrame = list->hwLayers[i].displayFrame;
         Rect tmpRect(displayFrame.left,displayFrame.top,
                      displayFrame.right,displayFrame.bottom);
         wormholeRegion.set(tmpRect);
      }
   }else{
     for (size_t i = 0 ; i < last; i++) {
        // need to take care only in per pixel blending.
        // Restrict calculation only for copybit layers.
        if((list->hwLayers[i].blending != HWC_BLENDING_NONE) ||
           !(layerProp[i].mFlags & HWC_COPYBIT))
            continue ;
        hwc_rect_t displayFrame = list->hwLayers[i].displayFrame;
        Rect tmpRect(displayFrame.left,displayFrame.top,displayFrame.right,
        displayFrame.bottom);
        Region tmpRegion(tmpRect);
        wormholeRegion.subtractSelf(wormholeRegion.intersect(tmpRegion));
     }
   }
   if(wormholeRegion.isEmpty()){
        return 0;
   }
   //TO DO :- 1. remove union and call clear for each rect.
   Region::const_iterator it = wormholeRegion.begin();
   Region::const_iterator const end = wormholeRegion.end();
   while (it != end) {
       const Rect& r = *it++;
       hwc_rect_t tmpWormRect = {r.left,r.top,r.right,r.bottom};
       int dst_w =  clearWormholeRect.right -  clearWormholeRect.left;
       int dst_h =  clearWormholeRect.bottom -  clearWormholeRect.top;

       if (!(dst_w || dst_h))
             clearWormholeRect = tmpWormRect;
       else
             getUnion(clearWormholeRect, tmpWormRect, clearWormholeRect);

   }
   return 1;
}

}//namespace qdutils
