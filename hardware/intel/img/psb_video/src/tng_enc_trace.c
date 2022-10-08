/*
* Copyright (c) 2011 Intel Corporation. All Rights Reserved.
* Copyright (c) Imagination Technologies Limited, UK
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sub license, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice (including the
* next paragraph) shall be included in all copies or substantial portions
* of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
* IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
* ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>

#ifdef PDUMP_TEST
#include "topazscfwif.h"
#else
#include "psb_drv_debug.h"
#include "tng_hostcode.h"
#include "tng_hostheader.h"
#include "tng_picmgmt.h"
#include "tng_jpegES.h"
#endif

#include "tng_trace.h"

#define PRINT_ARRAY_NEW( FEILD, NUM)            \
    for(i=0;i< NUM;i++) {                       \
        if(i%6==0)                              \
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t");                   \
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t0x%x", data->FEILD[i]); } \
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t}\n");

#define PRINT_ARRAY_INT( FEILD, NUM)            \
do {                                            \
    int tmp;                                    \
                                                \
    for(tmp=0;tmp< NUM;tmp++) {                 \
        if(tmp%6==0)                            \
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t");                   \
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t0x%08x", FEILD[tmp]);         \
    }                                           \
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t}\n");                        \
} while (0)


#define PRINT_ARRAY_BYTE( FEILD, NUM)           \
do {                                            \
    int tmp;                                    \
                                                \
    for(tmp=0;tmp< NUM;tmp++) {                 \
        if(tmp%8==0)                           \
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t");                   \
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t0x%02x", FEILD[tmp]);         \
    }                                           \
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t}\n");                        \
} while (0)


/*
#define PRINT_ARRAY( FEILD, NUM)                \
    for(i=0;i< NUM;i++)                         \
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t0x%x", data->FEILD[i]);       \
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t}\n");
*/

#define PRINT_ARRAY( FEILD, NUM)  PRINT_ARRAY_NEW(FEILD, NUM)                

#define PRINT_ARRAY_ADDR(STR, FEILD, NUM)                       \
do {                                                            \
    int i = 0;                                                  \
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n");                                               \
    for (i=0;i< NUM;i++)  {                                     \
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t%s[%02d]=x%08x = {\t", STR, i, data->FEILD[i]); \
        if (dump_address_content && data->FEILD[i]) {           \
            unsigned char *virt = phy2virt(data->FEILD[i]);     \
            PRINT_ARRAY_BYTE( virt, 64);                        \
        } else {                                                \
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"}\n");                                      \
        }                                                       \
    }                                                           \
} while (0)


unsigned int duplicate_setvideo_dump = 0;
unsigned int dump_address_content = 1;
static unsigned int last_setvideo_dump = 0;
static unsigned int hide_setvideo_dump = 0;

static unsigned int linear_fb = 0;
static unsigned int linear_mmio_topaz = 0;
static unsigned int phy_fb, phy_mmio;
static IMG_MTX_VIDEO_CONTEXT *mtx_ctx = NULL; /* MTX context */

static int setup_device()
{
    unsigned int linear_mmio;
    int fd;
    
    /* Allow read/write to ALL io ports */
    ioperm(0, 1024, 1);
    iopl(3);
    
    phy_mmio = pci_get_long(1,0<<3, PCI_BASE_ADDRESS_1);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"MMIO:  PCI base1 for MMIO is 0x%08x\n", phy_mmio);
    phy_fb = pci_get_long(1,0<<3, PCI_BASE_ADDRESS_2);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"DDRM:  PCI base2 for FB   is 0x%08x\n", phy_fb);
        
    phy_mmio &= 0xfff80000;
    phy_fb &= 0xfffff000;

    fd = open("/dev/mem", O_RDWR);
    if (fd == -1) {
        perror("open");
        exit(-1);
    }
    
    /* map frame buffer to user space(map 128M) */
    linear_fb = (unsigned int)mmap(NULL,128<<20,PROT_READ | PROT_WRITE, 
                             MAP_SHARED,fd, phy_fb);

    /* map mmio to user space(1M) */
    linear_mmio = (unsigned int)mmap(NULL,0x100000,PROT_READ | PROT_WRITE, 
                              MAP_SHARED,fd, phy_mmio);
    linear_mmio_topaz = linear_mmio;

    close(fd);

    return 0;
}

/* convert physicall address to virtual by search MMU */
static void *phy2virt_mmu(unsigned int phyaddr)
{
    unsigned int fb_start, fb_end;
    int pd_index, pt_index, pg_offset;
    unsigned int pd_phyaddr, pt_phyaddr; /* phyaddrss of page directory/table */
    unsigned int mem_start; /* memory start physicall address */
    unsigned int *pd, *pt;/* virtual of page directory/table */
    void *mem_virt;

    if (phy_mmio == 0 || phy_fb == 0 || linear_fb == 0 || linear_mmio_topaz == 0) {
        setup_device();
    }
#ifdef _TOPAZHP_DEBUG_TRACE_
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"INFO: phy_mmio 0x%08x, phy_fb 0x%08x, linear_fb 0x%08x, linear_mmio_topaz 0x%08x\n", phy_mmio, phy_fb, linear_fb, linear_mmio_topaz);
#endif

    if (phy_mmio == 0 || phy_fb == 0 || linear_fb == 0 || linear_mmio_topaz == 0) {
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"ERROR:setup_device failed!\n");
        exit(-1);
    }
#ifdef _TOPAZHP_DEBUG_TRACE_
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"INFO: phy_mmio 0x%08x, phy_fb 0x%08x, linear_fb 0x%08x, linear_mmio_topaz 0x%08x\n", phy_mmio, phy_fb, linear_fb, linear_mmio_topaz);
#endif

    /* first map page directory */
    MULTICORE_READ32(REGNUM_TOPAZ_CR_MMU_DIR_LIST_BASE_ADDR, &pd_phyaddr);
#ifdef _TOPAZHP_DEBUG_TRACE_
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"INFO: page directory 0x%08x, phy addr is 0x%08x\n", pd_phyaddr, phyaddr);
#endif

    pd_phyaddr &= 0xfffff000;
    fb_start = phy_fb;
    fb_end = fb_start + (128<<20);
#ifdef _TOPAZHP_DEBUG_TRACE_
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"INFO: pd_phyaddr 0x%08x, fb_start 0x%08x, fb_end 0x%08x\n", pd_phyaddr, fb_start, fb_end);
#endif

    if ((pd_phyaddr < fb_start) || (pd_phyaddr > fb_end)) {
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"ERROR: page directory 0x%08x is not fb range [0x%08x, 0x%08x]\n",
               pd_phyaddr, fb_start, fb_end);
        exit(-1);
    }

    pd_index = phyaddr >> 22; /* the top 10bits are pd index */
    pt_index = (phyaddr >> 12) & 0x3ff; /* the middle 10 bits are pt index */
    pg_offset = phyaddr & 0xfff;
#ifdef _TOPAZHP_DEBUG_TRACE_
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"INFO: phyaddr 0x%08x, pd_index 0x%08x, pt_index 0x%08x, pg_offset 0x%08x\n", phyaddr, pd_index, pt_index, pg_offset);
#endif    

    /* find page directory entry */
    pd = (unsigned int *)(linear_fb + (pd_phyaddr - phy_fb));
#ifdef _TOPAZHP_DEBUG_TRACE_
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"INFO: pd_index 0x%08x, pd 0x%08x, pd[pd_index] 0x%08x\n", pd_index, pd, pd[pd_index]);
#endif
    if ((pd[pd_index] & 1) == 0) {/* not valid */
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"Error: the page directory index is invalid, not mapped\n");
        exit(-1);
    }
    pt_phyaddr = pd[pd_index] & 0xfffff000;

    /* process page table entry */
    if ((pt_phyaddr < fb_start) || (pt_phyaddr > fb_end)) {
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"ERROR: page table 0x%08x is not fb range [0x%08x, 0x%08x]\n",
               pt_phyaddr, fb_start, fb_end);
        exit(-1);
    }
    pt = (unsigned int *)(linear_fb + (pt_phyaddr - phy_fb));
#ifdef _TOPAZHP_DEBUG_TRACE_
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"INFO: pt_index 0x%08x, pt 0x%08x, pt[pt_index] 0x%08x\n", pt_index, pt, pt[pt_index]);
#endif
    if ((pt[pt_index] & 1) == 0) {
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"Error: the page table index is invalid, not mapped\n");
        exit(-1);
    }

    mem_start = pt[pt_index] & 0xfffff000;
    
#if 0
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"Phy=0x%08x(PD index=%d, PT index=%d, Page offset=%d)\n",
           phyaddr, pd_index, pt_index, pg_offset);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"MMU PD Phy=0x%08x, PT Phy=PD[%d]=0x%08x, PT[%d]=0x%08x, means mem real phy(start)=0x%08x\n",
           pd_phyaddr, pd_index, pd[pd_index], pt_index, pt[pt_index], mem_start);
#endif    
    mem_virt = (void *)(linear_fb + (mem_start - phy_fb));

    return mem_virt;
}

static void *phy2virt(unsigned int phyaddr)
{
#ifdef PDUMP_TEST
    void* phy2virt_pdump(unsigned int phy);

    (void)phy2virt_mmu; /* silence the warning */
    return phy2virt_pdump(phyaddr);
#else
    return phy2virt_mmu(phyaddr);
#endif
}


static void JPEG_MTX_DMA_dump(JPEG_MTX_DMA_SETUP *data)
{
    int i;
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ComponentPlane{\n");
    for(i=0;i<MTX_MAX_COMPONENTS ;i++)
    {
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\t   ui32PhysAddr=%d\n",data->ComponentPlane[i].ui32PhysAddr);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t   ui32Stride=%d",data->ComponentPlane[i].ui32Stride);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t   ui32Height=%d\n",data->ComponentPlane[i].ui32Height);
    }
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	}\n");
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	MCUComponent{\n");
    for(i=0;i<MTX_MAX_COMPONENTS ;i++)
    {
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\t   ui32WidthBlocks=%d",data->MCUComponent[i].ui32WidthBlocks);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t   ui32HeightBlocks=%d",data->MCUComponent[i].ui32HeightBlocks);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t   ui32XLimit=%d\n",data->MCUComponent[i].ui32XLimit);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t   ui32YLimit=%d\n",data->MCUComponent[i].ui32YLimit);
    }
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	}\n");
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui32ComponentsInScan =%d\n", data->ui32ComponentsInScan);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui32TableA =%d\n", data->ui32TableA);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16DataInterleaveStatus =%d\n", data->ui16DataInterleaveStatus);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16MaxPipes =%d\n", data->ui16MaxPipes);
    //drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	apWritebackRegions  {");
    //PRINT_ARRAY(	apWritebackRegions, WB_FIFO_SIZE);
}

static void ISSUE_BUFFER_dump(MTX_ISSUE_BUFFERS *data)
{
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui32MCUPositionOfScanAndPipeNo =%d\n", data->ui32MCUPositionOfScanAndPipeNo);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui32MCUCntAndResetFlag =%d\n", data->ui32MCUCntAndResetFlag);
}

static void JPEG_TABLE_dump(JPEG_MTX_QUANT_TABLE *data)
{
    int i;
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	aui8LumaQuantParams  {");
    PRINT_ARRAY(	aui8LumaQuantParams, QUANT_TABLE_SIZE_BYTES);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	aui8ChromaQuantParams  {");
    PRINT_ARRAY(	aui8ChromaQuantParams, QUANT_TABLE_SIZE_BYTES);
}


static int SETVIDEO_ui32MVSettingsBTable_dump(unsigned int phyaddr)
{
    IMG_UINT32 ui32DistanceB, ui32Position;
    IMG_MV_SETTINGS * pHostMVSettingsBTable;
        
    pHostMVSettingsBTable = (IMG_MV_SETTINGS *) phy2virt(phyaddr);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t(====ui32MVSettingsBTable====)\n");
    
    for (ui32DistanceB = 0; ui32DistanceB < MAX_BFRAMES; ui32DistanceB++)
    {
        for (ui32Position = 1; ui32Position <= ui32DistanceB + 1; ui32Position++)
        {
            IMG_MV_SETTINGS * pMvElement = (IMG_MV_SETTINGS * ) ((IMG_UINT8 *) pHostMVSettingsBTable + MV_OFFSET_IN_TABLE(ui32DistanceB, ui32Position - 1));
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t[ui32DistanceB=%d][ui32Position=%d].ui32MVCalc_Config=0x%08x\n",
                   ui32DistanceB, ui32Position, pMvElement->ui32MVCalc_Config);
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t[ui32DistanceB=%d][ui32Position=%d].ui32MVCalc_Colocated=0x%08x\n",
                   ui32DistanceB, ui32Position, pMvElement->ui32MVCalc_Colocated);
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t[ui32DistanceB=%d][ui32Position=%d].ui32MVCalc_Below=0x%08x\n",
                   ui32DistanceB, ui32Position, pMvElement->ui32MVCalc_Below);
        }
    }
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t(====ui32MVSettingsBTable====)\n");

    return 0;
}

static int SETVIDEO_ui32MVSettingsHierarchical_dump(unsigned int phyaddr)
{
    IMG_UINT32 ui32DistanceB;
    IMG_MV_SETTINGS * pHostMVSettingsHierarchical;
        
    pHostMVSettingsHierarchical = (IMG_MV_SETTINGS *) phy2virt(phyaddr);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t(====ui32MVSettingsHierarchical====)\n");
    
    for (ui32DistanceB = 0; ui32DistanceB < MAX_BFRAMES; ui32DistanceB++) {
        IMG_MV_SETTINGS *pMvElement = pHostMVSettingsHierarchical + ui32DistanceB;

        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t[ui32DistanceB=%d].ui32MVCalc_Config=0x%08x\n",
               ui32DistanceB, pMvElement->ui32MVCalc_Config);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t[ui32DistanceB=%d].ui32MVCalc_Colocated=0x%08x\n",
               ui32DistanceB, pMvElement->ui32MVCalc_Colocated);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t[ui32DistanceB=%d].ui32MVCalc_Below=0x%08x\n",
               ui32DistanceB, pMvElement->ui32MVCalc_Below);
        
    }
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t(====ui32MVSettingsHierarchical====)\n");

    return 0;
}


static int SETVIDEO_ui32FlatGopStruct_dump(unsigned int phyaddr)
{
    IMG_UINT16 * psGopStructure = (IMG_UINT16 * )phy2virt(phyaddr);
    int ui8EncodeOrderPos;
    
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t(====ui32FlatGopStruct====)\n");

    /* refer to DDK:MiniGop_GenerateFlat */
    for (ui8EncodeOrderPos = 0; ui8EncodeOrderPos < MAX_GOP_SIZE; ui8EncodeOrderPos++){
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32FlatGopStruct[%d]=0x%04x\n",ui8EncodeOrderPos, psGopStructure[ui8EncodeOrderPos]);
    }
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t(====ui32FlatGopStruct====)\n");

    return 0;
}


static int SETVIDEO_ui32HierarGopStruct_dump(unsigned int phyaddr)
{
    IMG_UINT16 * psGopStructure = (IMG_UINT16 * )phy2virt(phyaddr);
    int ui8EncodeOrderPos;
    
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t(====ui32HierarGopStruct====)\n");

    /* refer to DDK:MiniGop_GenerateFlat */
    for (ui8EncodeOrderPos = 0; ui8EncodeOrderPos < MAX_GOP_SIZE; ui8EncodeOrderPos++){
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32HierarGopStruct[%d]=0x%04x\n",ui8EncodeOrderPos, psGopStructure[ui8EncodeOrderPos]);
    }
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t(====ui32HierarGopStruct====)\n");

    return 0;
}


static char *IMG_FRAME_TEMPLATE_TYPE2Str(IMG_FRAME_TEMPLATE_TYPE tmp)
{
    switch (tmp){
    case IMG_FRAME_IDR:return "IMG_FRAME_IDR";
    case IMG_FRAME_INTRA:return "IMG_FRAME_INTRA";
    case IMG_FRAME_INTER_P:return "IMG_FRAME_INTER_P";
    case IMG_FRAME_INTER_B:return "IMG_FRAME_INTER_B";
    case IMG_FRAME_INTER_P_IDR:return "IMG_FRAME_INTER_P_IDR";
    case IMG_FRAME_UNDEFINED:return "IMG_FRAME_UNDEFINED";
    }

    return "Undefined";
}

static int MTX_HEADER_PARAMS_dump(MTX_HEADER_PARAMS *p);
static int apSliceParamsTemplates_dump(SLICE_PARAMS *p)
{
    unsigned char *ptmp = (unsigned char*)&p->sSliceHdrTmpl;
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32Flags=0x%08x\n", p->ui32Flags);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32SliceConfig=0x%08x\n", p->ui32SliceConfig);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32IPEControl=0x%08x\n", p->ui32IPEControl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32SeqConfig=0x%08x\n", p->ui32SeqConfig);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\teTemplateType=%s\n", IMG_FRAME_TEMPLATE_TYPE2Str(p->eTemplateType));

    //PRINT_ARRAY_BYTE(ptmp, 64);

    MTX_HEADER_PARAMS_dump(&p->sSliceHdrTmpl);
    
    return 0;
}

static int DO_HEADER_dump(MTX_HEADER_PARAMS *data)
{
    MTX_HEADER_PARAMS *p = data;
    unsigned char *q=(unsigned char *)data;

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t(===RawBits===)");
    PRINT_ARRAY_BYTE(q, 128);

    MTX_HEADER_PARAMS_dump(p);
        
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\n");
    
    return 0;
}

static void SETVIDEO_dump(IMG_MTX_VIDEO_CONTEXT *data)
{
    unsigned int i;
    mtx_ctx = data;

    if(hide_setvideo_dump == 1)
        return ;
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t==========IMG_MTX_VIDEO_CONTEXT=============\n");
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui64ClockDivBitrate=%lld\n", data->ui64ClockDivBitrate);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32WidthInMbs=%d\n", data->ui32WidthInMbs);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32PictureHeightInMbs=%d\n", data->ui32PictureHeightInMbs);
#ifdef FORCED_REFERENCE
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apTmpReconstructured  {");
    PRINT_ARRAY_ADDR("apTmpReconstructured",	apTmpReconstructured, MAX_PIC_NODES);
#endif
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apReconstructured  {");
    PRINT_ARRAY_ADDR("apReconstructured",	apReconstructured, MAX_PIC_NODES);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apColocated  {");
    PRINT_ARRAY_ADDR("apColocated",	apColocated, MAX_PIC_NODES);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apMV  {");
    PRINT_ARRAY_ADDR("apMV",	apMV, MAX_MV);	
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apInterViewMV  {");
//    PRINT_ARRAY(	apInterViewMV, 2 );
    PRINT_ARRAY_ADDR("apInterViewMV", apInterViewMV, 2);	

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32DebugCRCs=0x%x\n", data->ui32DebugCRCs);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apWritebackRegions  {");
    PRINT_ARRAY_ADDR("apWritebackRegions",	apWritebackRegions, WB_FIFO_SIZE);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32InitialCPBremovaldelayoffset=0x%x\n", data->ui32InitialCPBremovaldelayoffset);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32MaxBufferMultClockDivBitrate=0x%x\n", data->ui32MaxBufferMultClockDivBitrate);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	pSEIBufferingPeriodTemplate=0x%x\n", data->pSEIBufferingPeriodTemplate);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	pSEIPictureTimingTemplate=0x%x\n", data->pSEIPictureTimingTemplate);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b16EnableMvc=%d\n", data->b16EnableMvc);
    //drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b16EnableInterViewReference=%d\n", data->b16EnableInterViewReference);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui16MvcViewIdx=0x%x\n", data->ui16MvcViewIdx);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apSliceParamsTemplates  {\n");
    //PRINT_ARRAY_ADDR( apSliceParamsTemplates, 5);
    for (i=0; i<5; i++) {
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tapSliceParamsTemplates[%d]=0x%08x  {\n", i, data->apSliceParamsTemplates[i]);
        apSliceParamsTemplates_dump(phy2virt(data->apSliceParamsTemplates[i]));
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t}\n");
    }

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apPicHdrTemplates  {");
    PRINT_ARRAY_ADDR("apPicHdrTemplates", apPicHdrTemplates, 4);
    MTX_HEADER_PARAMS_dump(phy2virt(data->apPicHdrTemplates[0]));
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui32SliceMap  {");
    PRINT_ARRAY_ADDR("aui32SliceMap", 	aui32SliceMap, MAX_SOURCE_SLOTS_SL);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32FlatGopStruct=0x%x\n", data->ui32FlatGopStruct);
    SETVIDEO_ui32FlatGopStruct_dump(data->ui32FlatGopStruct);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apSeqHeader        =0x%x\n", data->apSeqHeader);
    if (data->apSeqHeader != 0)
	DO_HEADER_dump((MTX_HEADER_PARAMS *)(phy2virt(data->apSeqHeader)));
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apSubSetSeqHeader  =0x%x\n", data->apSubSetSeqHeader);
    if(data->apSubSetSeqHeader != 0)
        DO_HEADER_dump((MTX_HEADER_PARAMS *)(phy2virt(data->apSubSetSeqHeader)));
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b16NoSequenceHeaders =0x%x\n", data->b16NoSequenceHeaders);
    
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8WeightedPredictionEnabled=%d\n", data->b8WeightedPredictionEnabled);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8MTXWeightedImplicitBiPred=0x%x\n", data->ui8MTXWeightedImplicitBiPred);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui32WeightedPredictionVirtAddr  {");
    PRINT_ARRAY(aui32WeightedPredictionVirtAddr, MAX_SOURCE_SLOTS_SL);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32HierarGopStruct=0x%x\n", data->ui32HierarGopStruct);
    if(data->ui32HierarGopStruct != 0)
        SETVIDEO_ui32HierarGopStruct_dump(data->ui32HierarGopStruct);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	pFirstPassOutParamAddr  {");
    PRINT_ARRAY_ADDR("pFirstPassOutParamAddr",pFirstPassOutParamAddr, MAX_SOURCE_SLOTS_SL);
#ifndef EXCLUDE_BEST_MP_DECISION_DATA
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	pFirstPassOutBestMultipassParamAddr  {");
    PRINT_ARRAY_ADDR("pFirstPassOutBestMultipassParamAddr", pFirstPassOutBestMultipassParamAddr, MAX_SOURCE_SLOTS_SL);
#endif
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	pMBCtrlInParamsAddr  {");
    PRINT_ARRAY_ADDR("pMBCtrlInParamsAddr", pMBCtrlInParamsAddr, MAX_SOURCE_SLOTS_SL);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32InterIntraScale{");
    PRINT_ARRAY( ui32InterIntraScale, SCALE_TBL_SZ);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32SkippedCodedScale  {");
    PRINT_ARRAY( ui32SkippedCodedScale, SCALE_TBL_SZ);


    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32PicRowStride=0x%x\n", data->ui32PicRowStride);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	apAboveParams  {");
    PRINT_ARRAY_ADDR("apAboveParams", apAboveParams, TOPAZHP_NUM_PIPES);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32IdrPeriod =0x%x\n ", data->ui32IdrPeriod);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32IntraLoopCnt =0x%x\n", data->ui32IntraLoopCnt);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32BFrameCount =0x%x\n", data->ui32BFrameCount);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8Hierarchical=%d\n", data->b8Hierarchical);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8MPEG2IntraDCPrecision =0x%x\n", data->ui8MPEG2IntraDCPrecision);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui8PicOnLevel  {");
    PRINT_ARRAY(aui8PicOnLevel, MAX_REF_LEVELS);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32VopTimeResolution=0x%x\n", data->ui32VopTimeResolution);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32InitialQp=0x%x\n", data->ui32InitialQp);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32BUSize=0x%x\n", data->ui32BUSize);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	sMVSettingsIdr { \n\t\t\tui32MVCalc_Below=0x%x\n \t\t\tui32MVCalc_Colocated=0x%x\n \t\t\tui32MVCalc_Config=0x%x\n \t\t}\n", data->sMVSettingsIdr.ui32MVCalc_Below,data->sMVSettingsIdr.ui32MVCalc_Colocated, data->sMVSettingsIdr.ui32MVCalc_Config);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	sMVSettingsNonB { \n");
    for(i=0;i<MAX_BFRAMES +1;i++)
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\tui32MVCalc_Below=0x%x   ui32MVCalc_Colocated=0x%x   ui32MVCalc_Config=0x%x }\n", data->sMVSettingsNonB[i].ui32MVCalc_Below,data->sMVSettingsNonB[i].ui32MVCalc_Colocated, data->sMVSettingsNonB[i].ui32MVCalc_Config);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t}\n");

    drv_debug_msg(VIDEO_ENCODE_PDUMP," \t	ui32MVSettingsBTable=0x%x\n", data->ui32MVSettingsBTable);
    SETVIDEO_ui32MVSettingsBTable_dump(data->ui32MVSettingsBTable);
    
    drv_debug_msg(VIDEO_ENCODE_PDUMP," \t	ui32MVSettingsHierarchical=0x%x\n", data->ui32MVSettingsHierarchical);
    if(data->ui32MVSettingsHierarchical != 0)
        SETVIDEO_ui32MVSettingsHierarchical_dump(data->ui32MVSettingsHierarchical);
    
#ifdef FIRMWARE_BIAS
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui32DirectBias_P  {");
    PRINT_ARRAY_NEW(aui32DirectBias_P,27 );
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui32InterBias_P  {");
    PRINT_ARRAY_NEW(aui32InterBias_P,27 );
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui32DirectBias_B  {");
    PRINT_ARRAY_NEW(aui32DirectBias_B,27 );
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui32InterBias_B  {");
    PRINT_ARRAY_NEW(aui32InterBias_B,27 );
#endif
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	eFormat=%d\n", data->eFormat);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	eStandard=%d\n", data->eStandard);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	eRCMode=%d\n", data->eRCMode);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8FirstPic=%d\n", data->b8FirstPic);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8IsInterlaced=%d\n", data->b8IsInterlaced);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8TopFieldFirst=%d\n", data->b8TopFieldFirst);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8ArbitrarySO=%d\n", data->b8ArbitrarySO);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	bOutputReconstructed=%d\n", data->bOutputReconstructed);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8DisableBitStuffing=%d\n", data->b8DisableBitStuffing);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8InsertHRDparams=%d\n", data->b8InsertHRDparams);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8MaxSlicesPerPicture=%d\n", data->ui8MaxSlicesPerPicture);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8NumPipes=%d\n", data->ui8NumPipes);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	bCARC=%d\n", data->bCARC);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	iCARCBaseline=%d\n", data->iCARCBaseline);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	uCARCThreshold=%d\n", data->uCARCThreshold);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	uCARCCutoff=%d\n", data->uCARCCutoff);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	uCARCNegRange=%d\n", data->uCARCNegRange);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	uCARCNegScale=%d\n", data->uCARCNegScale);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	uCARCPosRange=%d\n", data->uCARCPosRange);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	uCARCPosScale=%d\n", data->uCARCPosScale);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	uCARCShift=%d\n", data->uCARCShift);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32MVClip_Config=%d\n", data->ui32MVClip_Config);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32PredCombControl=%d\n", data->ui32PredCombControl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32LRITC_Tile_Use_Config=%d\n", data->ui32LRITC_Tile_Use_Config);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32LRITC_Cache_Chunk_Config=%d\n", data->ui32LRITC_Cache_Chunk_Config);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32IPEVectorClipping=%d\n", data->ui32IPEVectorClipping);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32H264CompControl=%d\n", data->ui32H264CompControl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32H264CompIntraPredModes=%d\n", data->ui32H264CompIntraPredModes);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32IPCM_0_Config=%d\n", data->ui32IPCM_0_Config);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32IPCM_1_Config=%d\n", data->ui32IPCM_1_Config);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32SPEMvdClipRange=%d\n", data->ui32SPEMvdClipRange);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32JMCompControl=%d\n", data->ui32JMCompControl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32MBHostCtrl=%d\n", data->ui32MBHostCtrl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32DeblockCtrl=%d\n", data->ui32DeblockCtrl);
    
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32SkipCodedInterIntra=%d\n", data->ui32SkipCodedInterIntra);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32VLCControl=%d\n", data->ui32VLCControl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32VLCSliceControl=%d\n", data->ui32VLCSliceControl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32VLCSliceMBControl=%d\n", data->ui32VLCSliceMBControl);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui16CQPOffset=%d\n", data->ui16CQPOffset);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8CodedHeaderPerSlice=%d\n", data->b8CodedHeaderPerSlice);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32FirstPicFlags=%d\n", data->ui32FirstPicFlags);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32NonFirstPicFlags=%d\n", data->ui32NonFirstPicFlags);

#ifndef EXCLUDE_ADAPTIVE_ROUNDING
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	bMCAdaptiveRoundingDisable=%d\n",data->bMCAdaptiveRoundingDisable);
    int j;
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui16MCAdaptiveRoundingOffsets[18][4]");
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n");
    for(i=0;i<18;i++){
        for(j=0;j<4;j++)
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\t0x%x", data-> ui16MCAdaptiveRoundingOffsets[i][j]);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n");
    }
#endif

#ifdef FORCED_REFERENCE
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32PatchedReconAddress=0x%x\n", data->ui32PatchedReconAddress);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32PatchedRef0Address=0x%x\n", data->ui32PatchedRef0Address);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui32PatchedRef1Address=0x%x\n", data->ui32PatchedRef1Address);
#endif
#ifdef LTREFHEADER
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	aui32LTRefHeader  {");
    PRINT_ARRAY_ADDR("aui32LTRefHeader",aui32LTRefHeader, MAX_SOURCE_SLOTS_SL);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	i8SliceHeaderSlotNum=%d\n",data->i8SliceHeaderSlotNum);
#endif
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8ReconIsLongTerm=%d\n", data->b8ReconIsLongTerm);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8Ref0IsLongTerm=%d\n", data->b8Ref0IsLongTerm);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	b8Ref1IsLongTerm=%d\n", data->b8Ref1IsLongTerm);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8RefSpacing=0x%x\n", data->ui8RefSpacing);

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8FirstPipe=0x%x\n", data->ui8FirstPipe);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8LastPipe=0x%x\n", data->ui8LastPipe);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	ui8PipesToUseFlags=0x%x\n", data->ui8PipesToUseFlags);
    
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t	sInParams {\n");
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16MBPerFrm=%d\n",data->sInParams.ui16MBPerFrm);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16MBPerBU=%d\n", data->sInParams.ui16MBPerBU);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16BUPerFrm=%d\n",data->sInParams.ui16BUPerFrm);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16IntraPerio=%d\n",data->sInParams.ui16IntraPeriod);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16BFrames=%d\n", data->sInParams.ui16BFrames);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	bHierarchicalMode=%d\n",data->sInParams.mode.h264.bHierarchicalMode);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32BitsPerFrm=%d\n",   data->sInParams.i32BitsPerFrm);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32BitsPerBU=%d\n",    data->sInParams.i32BitsPerBU);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32BitsPerMB=%d\n",    data->sInParams.mode.other.i32BitsPerMB);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32BitRate=%d\n",data->sInParams.i32BitRate);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32BufferSiz=%d\n",data->sInParams.i32BufferSize );
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32InitialLevel=%d\n", data->sInParams.i32InitialLevel);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32InitialDelay=%d\n", data->sInParams.i32InitialDelay);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32BitsPerGOP=%d\n",   data->sInParams.mode.other.i32BitsPerGOP);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16AvQPVal=%d\n", data->sInParams.mode.other.ui16AvQPVal);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui16MyInitQP=%d\n",data->sInParams.mode.other.ui16MyInitQP);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui32RCScaleFactor=%d\n",data->sInParams.mode.h264.ui32RCScaleFactor);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	bScDetectDis;=%d\n", data->sInParams.mode.h264.bScDetectDisable);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	bFrmSkipDisable=%d\n",data->sInParams.bFrmSkipDisable);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	bBUSkipDisable=%d\n",data->sInParams.mode.other.bBUSkipDisable);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui8SeInitQP=%d\n",    data->sInParams.ui8SeInitQP	);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui8MinQPVal=%d\n",    data->sInParams.ui8MinQPVal	);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui8MaxQPVal=%d\n",    data->sInParams.ui8MaxQPVal	);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui8MBPerRow=%d\n",    data->sInParams.ui8MBPerRow	);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui8ScaleFactor=%d\n",  data->sInParams.ui8ScaleFactor);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui8HalfFrame=%d\n",    data->sInParams.mode.other.ui8HalfFrameRate);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	ui8FCode=%d\n",        data->sInParams.mode.other.ui8FCode);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t	i32TransferRate=%d\n",data->sInParams.mode.h264.i32TransferRate);
    
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t}\n");
}


struct header_token {
    int token;
    char *str;
} header_tokens[] = {
    {ELEMENT_STARTCODE_RAWDATA,"ELEMENT_STARTCODE_RAWDATA=0"},
    {ELEMENT_STARTCODE_MIDHDR,"ELEMENT_STARTCODE_MIDHDR"},
    {ELEMENT_RAWDATA,"ELEMENT_RAWDATA"},
    {ELEMENT_QP,"ELEMENT_QP"},
    {ELEMENT_SQP,"ELEMENT_SQP"},
    {ELEMENT_FRAMEQSCALE,"ELEMENT_FRAMEQSCALE"},
    {ELEMENT_SLICEQSCALE,"ELEMENT_SLICEQSCALE"},
    {ELEMENT_INSERTBYTEALIGN_H264,"ELEMENT_INSERTBYTEALIGN_H264"},
    {ELEMENT_INSERTBYTEALIGN_MPG4,"ELEMENT_INSERTBYTEALIGN_MPG4"},
    {ELEMENT_INSERTBYTEALIGN_MPG2,"ELEMENT_INSERTBYTEALIGN_MPG2"},
    {ELEMENT_VBV_MPG2,"ELEMENT_VBV_MPG2"},
    {ELEMENT_TEMPORAL_REF_MPG2,"ELEMENT_TEMPORAL_REF_MPG2"},
    {ELEMENT_CURRMBNR,"ELEMENT_CURRMBNR"},
    {ELEMENT_FRAME_NUM,"ELEMENT_FRAME_NUM"},
    {ELEMENT_TEMPORAL_REFERENCE,"ELEMENT_TEMPORAL_REFERENCE"},
    {ELEMENT_EXTENDED_TR,"ELEMENT_EXTENDED_TR"},
    {ELEMENT_IDR_PIC_ID,"ELEMENT_IDR_PIC_ID"},
    {ELEMENT_PIC_ORDER_CNT,"ELEMENT_PIC_ORDER_CNT"},
    {ELEMENT_GOB_FRAME_ID,"ELEMENT_GOB_FRAME_ID"},
    {ELEMENT_VOP_TIME_INCREMENT,"ELEMENT_VOP_TIME_INCREMENT"},
    {ELEMENT_MODULO_TIME_BASE,"ELEMENT_MODULO_TIME_BASE"},
    {ELEMENT_BOTTOM_FIELD,"ELEMENT_BOTTOM_FIELD"},
    {ELEMENT_SLICE_NUM,"ELEMENT_SLICE_NUM"},
    {ELEMENT_MPEG2_SLICE_VERTICAL_POS,"ELEMENT_MPEG2_SLICE_VERTICAL_POS"},
    {ELEMENT_MPEG2_IS_INTRA_SLICE,"ELEMENT_MPEG2_IS_INTRA_SLICE"},
    {ELEMENT_MPEG2_PICTURE_STRUCTURE,"ELEMENT_MPEG2_PICTURE_STRUCTURE"},
    {ELEMENT_REFERENCE,"ELEMENT_REFERENCE"},
    {ELEMENT_ADAPTIVE,"ELEMENT_ADAPTIVE"},
    {ELEMENT_DIRECT_SPATIAL_MV_FLAG,"ELEMENT_DIRECT_SPATIAL_MV_FLAG"},
    {ELEMENT_NUM_REF_IDX_ACTIVE,"ELEMENT_NUM_REF_IDX_ACTIVE"},
    {ELEMENT_REORDER_L0,"ELEMENT_REORDER_L0"},
    {ELEMENT_REORDER_L1,"ELEMENT_REORDER_L1"},
    {ELEMENT_TEMPORAL_ID,"ELEMENT_TEMPORAL_ID"},
    {ELEMENT_ANCHOR_PIC_FLAG,"ELEMENT_ANCHOR_PIC_FLAG"},
    {BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY,"BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY"},
    {BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY_OFFSET,"BPH_SEI_NAL_INITIAL_CPB_REMOVAL_DELAY_OFFSET"},
    {PTH_SEI_NAL_CPB_REMOVAL_DELAY,"PTH_SEI_NAL_CPB_REMOVAL_DELAY"},
    {PTH_SEI_NAL_DPB_OUTPUT_DELAY,"PTH_SEI_NAL_DPB_OUTPUT_DELAY"},
    {ELEMENT_SLICEWEIGHTEDPREDICTIONSTRUCT,"ELEMENT_SLICEWEIGHTEDPREDICTIONSTRUCT"},
    {ELEMENT_CUSTOM_QUANT,"ELEMENT_CUSTOM_QUANT"}
};

static char *header_to_str(int token)
{
    int i;
    struct header_token *p;
    
    for (i=0; i<sizeof(header_tokens)/sizeof(struct header_token); i++) {
        p = &header_tokens[i];
        if (p->token == token)
            return p->str;
    }

    return "Invalid header token";
}

static int MTX_HEADER_PARAMS_dump(MTX_HEADER_PARAMS *p)
{
    MTX_HEADER_ELEMENT *last_element=NULL;
    int i;
    
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32Elements=%d\n", p->ui32Elements);
    for (i=0; i<p->ui32Elements; i++) {
        MTX_HEADER_ELEMENT *q = &(p->asElementStream[0]);

        if (last_element) {
            int ui8Offset = 0;
            IMG_UINT8 *ui8P;
            
            if (last_element->Element_Type==ELEMENT_STARTCODE_RAWDATA ||
                last_element->Element_Type==ELEMENT_RAWDATA ||
                last_element->Element_Type==ELEMENT_STARTCODE_MIDHDR)
            {
                //Add a new element aligned to word boundary
                //Find RAWBit size in bytes (rounded to word boundary))
                ui8Offset=last_element->ui8Size+8+31; // NumberofRawbits (excluding size of bit count field)+ size of the bitcount field
                ui8Offset/=32; //Now contains rawbits size in words
                ui8Offset+=1; //Now contains rawbits+element_type size in words
                ui8Offset*=4; //Convert to number of bytes (total size of structure in bytes, aligned to word boundary).
            }
            else
            {
                ui8Offset=4;
            }
            ui8P=(IMG_UINT8 *) last_element;
            ui8P+=ui8Offset;
            q=(MTX_HEADER_ELEMENT *) ui8P;
        }
        
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t----Head %d----\n",i);
        drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\tElement_Type=%d(0x%x:%s)\n",
               q->Element_Type, q->Element_Type, header_to_str(q->Element_Type));

        if (q->Element_Type==ELEMENT_STARTCODE_RAWDATA ||
            q->Element_Type==ELEMENT_RAWDATA ||
            q->Element_Type==ELEMENT_STARTCODE_MIDHDR) {
            int i, ui8Offset = 0;
            IMG_UINT8 *ui8P;
            
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\tui8Size=%d(0x%x)\n", q->ui8Size, q->ui8Size);
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\t(====aui8Bits===)");
            
            //Find RAWBit size in bytes (rounded to word boundary))
            ui8Offset=q->ui8Size+8+31; // NumberofRawbits (excluding size of bit count field)+ size of the bitcount field
            ui8Offset/=32; //Now contains rawbits size in words
            //ui8Offset+=1; //Now contains rawbits+element_type size in words
            ui8Offset*=4; //Convert to number of bytes (total size of structure in bytes, aligned to word boundar
            
            ui8P = &q->aui8Bits;
            for (i=0; i<ui8Offset; i++) {
                if ((i%8) == 0)
                    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\t\t\t");
                drv_debug_msg(VIDEO_ENCODE_PDUMP,"0x%02x\t", *ui8P);
                ui8P++;
            }
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n");
        } else {
            drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\t\t(no ui8Size/aui8Bits for this type header)\n");
        }
        
        last_element = q;
    }

    return 0;
}

static char *eBufferType2str(IMG_REF_BUFFER_TYPE tmp)
{
    switch (tmp) {
    case IMG_BUFFER_REF0:
        return "IMG_BUFFER_REF0";
    case IMG_BUFFER_REF1:
        return "IMG_BUFFER_REF1";
    case IMG_BUFFER_RECON:
        return "IMG_BUFFER_RECON";
    default:
        return "Unknown Buffer Type";
    }
}

static void PROVIDEBUFFER_SOURCE_dump(void *data)
{
    IMG_SOURCE_BUFFER_PARAMS *source = (IMG_SOURCE_BUFFER_PARAMS*) data;

    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32PhysAddrYPlane_Field0=0x%x\n",source->ui32PhysAddrYPlane_Field0);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32PhysAddrUPlane_Field0=0x%x\n",source->ui32PhysAddrUPlane_Field0);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32PhysAddrVPlane_Field0=0x%x\n",source->ui32PhysAddrVPlane_Field0);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32PhysAddrYPlane_Field1=0x%x\n",source->ui32PhysAddrYPlane_Field1);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32PhysAddrUPlane_Field1=0x%x\n",source->ui32PhysAddrUPlane_Field1);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32PhysAddrVPlane_Field1=0x%x\n",source->ui32PhysAddrVPlane_Field1);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui32HostContext=%d\n",source->ui32HostContext);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui8DisplayOrderNum=%d\n",source->ui8DisplayOrderNum);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tui8SlotNum=%d\n",source->ui8SlotNum);
    return ;
}

static int PROVIDEBUFFER_dump(unsigned int data)
{
    IMG_REF_BUFFER_TYPE eBufType = (data & MASK_MTX_MSG_PROVIDE_REF_BUFFER_USE) >> SHIFT_MTX_MSG_PROVIDE_REF_BUFFER_USE;
    //IMG_BUFFER_DATA *bufdata = p->sData;
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\teBufferType=(%s)\n",  eBufferType2str(eBufType));
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\n");
    
    return 0;
}

static char *eSubtype2str(IMG_PICMGMT_TYPE eSubtype)
{
    switch (eSubtype) {
        case IMG_PICMGMT_REF_TYPE:return "IMG_PICMGMT_REF_TYPE";
        case IMG_PICMGMT_GOP_STRUCT:return "IMG_PICMGMT_GOP_STRUCT";
        case IMG_PICMGMT_SKIP_FRAME:return "IMG_PICMGMT_SKIP_FRAME";
        case IMG_PICMGMT_EOS:return "IMG_PICMGMT_EOS";
        case IMG_PICMGMT_FLUSH:return "IMG_PICMGMT_FLUSH";
        case IMG_PICMGMT_QUANT:return "IMG_PICMGMT_QUANT";
        default: return "Unknow";
    }
}
int PICMGMT_dump(IMG_UINT32 data)
{
    IMG_PICMGMT_TYPE eSubType = (data & MASK_MTX_MSG_PICMGMT_SUBTYPE) >> SHIFT_MTX_MSG_PICMGMT_SUBTYPE;
    IMG_FRAME_TYPE eFrameType = 0;
    IMG_UINT32 ui32FrameCount = 0;

    drv_debug_msg(VIDEO_ENCODE_PDUMP,
        "\t\teSubtype=%d(%s)\n", eSubType, eSubtype2str(eSubType));
    drv_debug_msg(VIDEO_ENCODE_PDUMP,
        "\t\t(=====(additional data)=====\n");
    switch (eSubType) {
        case IMG_PICMGMT_REF_TYPE:
            eFrameType = (data & MASK_MTX_MSG_PICMGMT_DATA) >> SHIFT_MTX_MSG_PICMGMT_DATA;
            switch (eFrameType) {
                case IMG_INTRA_IDR:
                    drv_debug_msg(VIDEO_ENCODE_PDUMP,
                        "\t\teFrameType=IMG_INTRA_IDR\n");
                    break;
                case IMG_INTRA_FRAME:
                    drv_debug_msg(VIDEO_ENCODE_PDUMP,
                        "\t\teFrameType=IMG_INTRA_FRAME\n");
                    break;
                case IMG_INTER_P:
                    drv_debug_msg(VIDEO_ENCODE_PDUMP,
                        "\t\teFrameType=IMG_INTER_P\n");
                    break;
                case IMG_INTER_B:
                    drv_debug_msg(VIDEO_ENCODE_PDUMP,
                        "\t\teFrameType=IMG_INTER_B\n");
                    break;
            }
            break;
        case IMG_PICMGMT_EOS:
             ui32FrameCount = (data & MASK_MTX_MSG_PICMGMT_DATA) >> SHIFT_MTX_MSG_PICMGMT_DATA;
             drv_debug_msg(VIDEO_ENCODE_PDUMP,
                 "\t\tui32FrameCount=%d\n", ui32FrameCount);
             break;
        default:
             break;
    }
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\n\n");

    return 0;
}


static char * cmd2str(int cmdid)
{
    switch (cmdid) {
    case MTX_CMDID_NULL:        return "MTX_CMDID_NULL";
    case MTX_CMDID_SHUTDOWN:    return "MTX_CMDID_SHUTDOWN"; 
	// Video Commands
    case MTX_CMDID_DO_HEADER:   return "MTX_CMDID_DO_HEADER";
    case MTX_CMDID_ENCODE_FRAME:return "MTX_CMDID_ENCODE_FRAME";
    case MTX_CMDID_START_FRAME: return "MTX_CMDID_START_FRAME";
    case MTX_CMDID_ENCODE_SLICE:return "MTX_CMDID_ENCODE_SLICE";
    case MTX_CMDID_END_FRAME:   return "MTX_CMDID_END_FRAME";
    case MTX_CMDID_SETVIDEO:    return "MTX_CMDID_SETVIDEO";
    case MTX_CMDID_GETVIDEO:    return "MTX_CMDID_GETVIDEO";
    case MTX_CMDID_PICMGMT:     return "MTX_CMDID_PICMGMT";
    case MTX_CMDID_RC_UPDATE:   return "MTX_CMDID_RC_UPDATE";
    case MTX_CMDID_PROVIDE_SOURCE_BUFFER:return "MTX_CMDID_PROVIDE_SOURCE_BUFFER";
    case MTX_CMDID_PROVIDE_REF_BUFFER:   return "MTX_CMDID_PROVIDE_REF_BUFFER";
    case MTX_CMDID_PROVIDE_CODED_BUFFER: return "MTX_CMDID_PROVIDE_CODED_BUFFER";
    case MTX_CMDID_ABORT:       return "MTX_CMDID_ABORT";
	// JPEG commands
    case MTX_CMDID_SETQUANT:    return "MTX_CMDID_SETQUANT";         
    case MTX_CMDID_SETUP_INTERFACE:      return "MTX_CMDID_SETUP_INTERFACE"; 
    case MTX_CMDID_ISSUEBUFF:   return "MTX_CMDID_ISSUEBUFF";       
    case MTX_CMDID_SETUP:       return "MTX_CMDID_SETUP";           
    case MTX_CMDID_ENDMARKER:
    default:
        return "Invalid Command (%d)";
    }
}

static int command_parameter_dump(int cmdid, unsigned int cmd_data, unsigned int cmd_addr)
{
    MTX_HEADER_PARAMS *header_para;
    IMG_MTX_VIDEO_CONTEXT *context;
    JPEG_MTX_QUANT_TABLE *jpeg_table;
    MTX_ISSUE_BUFFERS *issue_buffer;
    JPEG_MTX_DMA_SETUP *jpeg_mtx_dma_setup;
    void *virt_addr = 0;
    if (cmd_addr != 0)
        virt_addr = phy2virt(cmd_addr);

    switch (cmdid) {
        case MTX_CMDID_NULL:
        case MTX_CMDID_SHUTDOWN:
        case MTX_CMDID_ENDMARKER :         //!< end marker for enum
        case MTX_CMDID_GETVIDEO:
            break;
        case MTX_CMDID_DO_HEADER:
            header_para = (MTX_HEADER_PARAMS *)virt_addr;
            DO_HEADER_dump(header_para);
            if (duplicate_setvideo_dump)
                SETVIDEO_dump(mtx_ctx);
            break;
        case MTX_CMDID_ENCODE_FRAME:
            if (duplicate_setvideo_dump)
                SETVIDEO_dump(mtx_ctx);
            if (last_setvideo_dump == 1)
                SETVIDEO_dump(mtx_ctx);
            break;
        case MTX_CMDID_SETVIDEO:
            context = (IMG_MTX_VIDEO_CONTEXT *)virt_addr;
            if (last_setvideo_dump == 1)
                mtx_ctx = virt_addr;
            else
                SETVIDEO_dump(context);
            break;
        case MTX_CMDID_PICMGMT :            
            PICMGMT_dump(cmd_data);
            break;
        case  MTX_CMDID_PROVIDE_SOURCE_BUFFER:
            PROVIDEBUFFER_SOURCE_dump((IMG_SOURCE_BUFFER_PARAMS *)virt_addr);
            break;
        case  MTX_CMDID_PROVIDE_REF_BUFFER:
            PROVIDEBUFFER_dump(cmd_data);
            break;
        case MTX_CMDID_PROVIDE_CODED_BUFFER:
            break;

            // JPEG commands
        case MTX_CMDID_SETQUANT:          //!< (data: #JPEG_MTX_QUANT_TABLE)\n
            jpeg_table = (JPEG_MTX_QUANT_TABLE *)virt_addr;
            JPEG_TABLE_dump(jpeg_table);
            break;
        case MTX_CMDID_ISSUEBUFF:         //!< (data: #MTX_ISSUE_BUFFERS)\n
            issue_buffer = (MTX_ISSUE_BUFFERS *)virt_addr;
            ISSUE_BUFFER_dump(issue_buffer);
            break;
        case MTX_CMDID_SETUP:             //!< (data: #JPEG_MTX_DMA_SETUP)\n\n
            jpeg_mtx_dma_setup = (JPEG_MTX_DMA_SETUP *)virt_addr;
            JPEG_MTX_DMA_dump(jpeg_mtx_dma_setup);
            break;
        default:
            break;
    }
    return 0;
}
static struct RegisterInfomation multicore_regs[] = {
    {"MULTICORE_SRST",0x0000},
    {"MULTICORE_INT_STAT",0x0004},
    {"MULTICORE_MTX_INT_ENAB",0x0008},
    {"MULTICORE_HOST_INT_ENAB",0x000C},
    {"MULTICORE_INT_CLEAR",0x0010},
    {"MULTICORE_MAN_CLK_GATE",0x0014},
    {"TOPAZ_MTX_C_RATIO",0x0018},
    {"MMU_STATUS",0x001C},
    {"MMU_MEM_REQ",0x0020},
    {"MMU_CONTROL0",0x0024},
    {"MMU_CONTROL1",0x0028},
    {"MMU_CONTROL2",0x002C},
    {"MMU_DIR_LIST_BASE",0x0030},
    {"MMU_TILE",0x0038},
    {"MTX_DEBUG_MSTR",0x0044},
    {"MTX_DEBUG_SLV",0x0048},
    {"MULTICORE_CORE_SEL_0",0x0050},
    {"MULTICORE_CORE_SEL_1",0x0054},
    {"MULTICORE_HW_CFG",0x0058},
    {"MULTICORE_CMD_FIFO_WRITE",0x0060},
    {"MULTICORE_CMD_FIFO_WRITE_SPACE",0x0064},
    {"TOPAZ_CMD_FIFO_READ",0x0070},
    {"TOPAZ_CMD_FIFO_READ_AVAILABLE",0x0074},
    {"TOPAZ_CMD_FIFO_FLUSH",0x0078},
    {"MMU_TILE_EXT",0x0080},
    {"FIRMWARE_REG_1",0x0100},
    {"FIRMWARE_REG_2",0x0104},
    {"FIRMWARE_REG_3",0x0108},
    {"CYCLE_COUNTER",0x0110},
    {"CYCLE_COUNTER_CTRL",0x0114},
    {"MULTICORE_IDLE_PWR_MAN",0x0118},
    {"DIRECT_BIAS_TABLE",0x0124},
    {"INTRA_BIAS_TABLE",0x0128},
    {"INTER_BIAS_TABLE",0x012C},
    {"INTRA_SCALE_TABLE",0x0130},
    {"QPCB_QPCR_OFFSET",0x0134},
    {"INTER_INTRA_SCALE_TABLE",0x0140},
    {"SKIPPED_CODED_SCALE_TABLE",0x0144},
    {"POLYNOM_ALPHA_COEFF_CORE0",0x0148},
    {"POLYNOM_GAMMA_COEFF_CORE0",0x014C},
    {"POLYNOM_CUTOFF_CORE0",0x0150},
    {"POLYNOM_ALPHA_COEFF_CORE1",0x0154},
    {"POLYNOM_GAMMA_COEFF_CORE1",0x0158},
    {"POLYNOM_CUTOFF_CORE1",0x015C},
    {"POLYNOM_ALPHA_COEFF_CORE2",0x0160},
    {"POLYNOM_GAMMA_COEFF_CORE2",0x0164},
    {"POLYNOM_CUTOFF_CORE2",0x0168},
    {"FIRMWARE_REG_4",0x0300},
    {"FIRMWARE_REG_5",0x0304},
    {"FIRMWARE_REG_6",0x0308},
    {"FIRMWARE_REG_7",0x030C},
    {"MULTICORE_RSVD0",0x03B0},
    {"TOPAZHP_CORE_ID",0x03C0},
    {"TOPAZHP_CORE_REV",0x03D0},
    {"TOPAZHP_CORE_DES1",0x03E0},
    {"TOPAZHP_CORE_DES2",0x03F0},
};


static struct RegisterInfomation core_regs[] = {
    {"TOPAZHP_SRST",0x0000},
    {"TOPAZHP_INTSTAT",0x0004},
    {"TOPAZHP_MTX_INTENAB",0x0008},
    {"TOPAZHP_HOST_INTENAB",0x000C},
    {"TOPAZHP_INTCLEAR",0x0010},
    {"TOPAZHP_INT_COMB_SEL",0x0014},
    {"TOPAZHP_BUSY",0x0018},
    {"TOPAZHP_AUTO_CLOCK_GATING",0x0024},
    {"TOPAZHP_MAN_CLOCK_GATING",0x0028},
    {"TOPAZHP_RTM",0x0030},
    {"TOPAZHP_RTM_VALUE",0x0034},
    {"TOPAZHP_MB_PERFORMANCE_RESULT",0x0038},
    {"TOPAZHP_MB_PERFORMANCE_MB_NUMBER",0x003C},
    {"FIELD_PARITY",0x0188},
    {"WEIGHTED_PRED_CONTROL",0x03D0},
    {"WEIGHTED_PRED_COEFFS",0x03D4},
    {"WEIGHTED_PRED_INV_WEIGHT",0x03E0},
    {"TOPAZHP_RSVD0",0x03F0},
    {"TOPAZHP_CRC_CLEAR",0x03F4},
    {"SPE_ZERO_THRESH",0x0344},
    {"SPE0_BEST_SAD_SIGNATURE",0x0348},
    {"SPE1_BEST_SAD_SIGNATURE",0x034C},
    {"SPE0_BEST_INDEX_SIGNATURE",0x0350},
    {"SPE1_BEST_INDEX_SIGNATURE",0x0354},
    {"SPE_INTRA_COST_SIGNATURE",0x0358},
    {"SPE_MVD_CLIP_RANGE",0x0360},
    {"SPE_SUBPEL_RESOLUTION",0x0364},
    {"SPE0_MV_SIZE_SIGNATURE",0x0368},
    {"SPE1_MV_SIZE_SIGNATURE",0x036C},
    {"SPE_MB_PERFORMANCE_RESULT",0x0370},
    {"SPE_MB_PERFORMANCE_MB_NUMBER",0x0374},
    {"SPE_MB_PERFORMANCE_CLEAR",0x0378},
    {"MEM_SIGNATURE_CONTROL",0x0060},
    {"MEM_SIGNATURE_ENC_WDATA",0x0064},
    {"MEM_SIGNATURE_ENC_RDATA",0x0068},
    {"MEM_SIGNATURE_ENC_ADDR",0x006C},
    {"PREFETCH_LRITC_SIGNATURE",0x0070},
    {"PROC_DMA_CONTROL",0x00E0},
    {"PROC_DMA_STATUS",0x00E4},
    {"PROC_ESB_ACCESS_CONTROL",0x00EC},
    {"PROC_ESB_ACCESS_WORD0",0x00F0},
    {"PROC_ESB_ACCESS_WORD1",0x00F4},
    {"PROC_ESB_ACCESS_WORD2",0x00F8},
    {"PROC_ESB_ACCESS_WORD3",0x00FC},

    {"LRITC_TILE_USE_CONFIG",0x0040},
    {"LRITC_TILE_USE_STATUS",0x0048},
    {"LRITC_TILE_FREE_STATUS",0x004C},
    {"LRITC_CACHE_CHUNK_CONFIG",0x0050},
    {"LRITC_CACHE_CHUNK_STATUS",0x0054},
    {"LRITC_SIGNATURE_ADDR",0x0058},
    {"LRITC_SIGNATURE_RDATA",0x005C},
    
    {"SEQ_CUR_PIC_LUMA_BASE_ADDR",0x0100},
    {"SEQ_CUR_PIC_CB_BASE_ADDR",0x0104},
    {"SEQ_CUR_PIC_CR_BASE_ADDR",0x0108},
    {"SEQ_CUR_PIC_ROW_STRIDE",0x010C},
    {"SEQ_REF_PIC0_LUMA_BASE_ADDR",0x0110},
    {"SEQ_REF_PIC0_CHROMA_BASE_ADDR",0x0114},
    {"SEQ_REF_PIC1_LUMA_BASE_ADDR",0x0118},
    {"SEQ_REF_PIC1_CHROMA_BASE_ADDR",0x011C},
    {"SEQ_CUR_PIC_CONFIG",0x0120},
    {"SEQ_CUR_PIC_SIZE",0x0124},
    {"SEQ_RECON_LUMA_BASE_ADDR",0x0128},
    {"SEQ_RECON_CHROMA_BASE_ADDR",0x012C},
    {"SEQ_ABOVE_PARAM_BASE_ADDR",0x0130},
    {"SEQ_TEMPORAL_COLOCATED_IN_ADDR",0x0134},
    {"SEQ_TEMPORAL_PIC0_MV_IN_ADDR",0x0138},
    {"SEQ_TEMPORAL_PIC1_MV_IN_ADDR",0x013C},
    {"SEQ_TEMPORAL_COLOCATED_OUT_ADDR",0x0140},
    {"SEQ_TEMPORAL_PIC0_MV_OUT_ADDR",0x0144},
    {"SEQ_TEMPORAL_PIC1_MV_OUT_ADDR",0x0148},
    {"SEQ_MB_FIRST_STAGE_OUT_ADDR",0x014C},
    {"SEQ_MB_CONTROL_IN_ADDR",0x0150},
    {"SEQUENCER_CONFIG",0x0154},
    {"SLICE_CONFIG",0x0158},
    {"SLICE_QP_CONFIG",0x015C},
    {"SEQUENCER_KICK",0x0160},
    {"H264COMP_REJECT_THRESHOLD",0x0184},
    {"H264COMP_CUSTOM_QUANT_SP",0x01A0},
    {"H264COMP_CUSTOM_QUANT_Q",0x01A4},
    {"H264COMP_CONTROL",0x01A8},
    {"H264COMP_INTRA_PRED_MODES",0x01AC},
    {"H264COMP_MAX_CYCLE_COUNT",0x01B0},
    {"H264COMP_MAX_CYCLE_MB",0x01B4},
    {"H264COMP_MAX_CYCLE_RESET",0x01B8},
    {"H264COMP4X4_PRED_CRC",0x01BC},
    {"H264COMP4X4_COEFFS_CRC",0x01C0},
    {"H264COMP4X4_RECON_CRC",0x01C4},
    {"H264COMP8X8_PRED_CRC",0x01C8},
    {"H264COMP8X8_COEFFS_CRC",0x01CC},
    {"H264COMP8X8_RECON_CRC",0x01D0},
    {"H264COMP16X16_PRED_CRC",0x01D4},
    {"H264COMP16X16_COEFFS_CRC",0x01D8},
    {"H264COMP16X16_RECON_CRC",0x01DC},
    {"H264COMP_ROUND_0",0x01E0},
    {"H264COMP_ROUND_1",0x01E4},
    {"H264COMP_ROUND_2",0x01E8},
    {"H264COMP_ROUND_INIT",0x01EC},
    {"H264COMP_VIDEO_CONF_CONTROL_0",0x01F0},
    {"H264COMP_VIDEO_CONF_CONTROL_1",0x01F4},
    {"H264COMP_VIDEO_CONF_STATUS_0",0x01F8},
    {"H264COMP_VIDEO_CONF_STATUS_1",0x01FC},
};

static struct RegisterInfomation mtx_regs[] = {
    {"MTX_ENABLE",0x0000},
    {"MTX_STATUS",0x0008},
    {"MTX_KICK",0x0080},
    {"MTX_KICKI",0x0088},
    {"MTX_FAULT0",0x0090},
    {"MTX_REGISTER_READ_WRITE_DATA",0x00F8},
    {"MTX_REGISTER_READ_WRITE_REQUEST",0x00FC},
    {"MTX_RAM_ACCESS_DATA_EXCHANGE",0x0100},
    {"MTX_RAM_ACCESS_DATA_TRANSFER",0x0104},
    {"MTX_RAM_ACCESS_CONTROL",0x0108},
    {"MTX_RAM_ACCESS_STATUS",0x010C},
    {"MTX_SOFT_RESET",0x0200},
    {"MTX_SYSC_CDMAC",0x0340},
    {"MTX_SYSC_CDMAA",0x0344},
    {"MTX_SYSC_CDMAS0",0x0348},
    {"MTX_SYSC_CDMAS1",0x034C},
    {"MTX_SYSC_CDMAT",0x0350}
};


static struct RegisterInfomation dmac_regs[] = {
    {"DMA_Setup",0x0000},
    {"DMA_Count",0x0004},
    {"DMA_Peripheral_param",0x0008},
    {"DMA_IRQ_Stat",0x000c},
    {"DMA_2D_Mode",0x0010},
    {"DMA_Peripheral_addr",0x0014},
    {"DMA_Per_hold",0x0018},
    {"DMA_SoftReset",0x0020},
};


int topazhp_dump_command(unsigned int *comm_dword)
{
    int cmdid;

    if (comm_dword == NULL)
        return 1;
    
    cmdid = (comm_dword[0] & MASK_MTX_MSG_CMD_ID) & ~ MASK_MTX_MSG_PRIORITY;

    (void)multicore_regs;
    (void)core_regs;
    (void)mtx_regs;
    (void)dmac_regs;
    
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\tSend command to MTX\n");
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tCMDWORD_ID=%s(High priority:%s)\n", cmd2str(cmdid),
           (comm_dword[0] & MASK_MTX_MSG_PRIORITY)?"Yes":"No");
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tCMDWORD_CORE=%d\n", (comm_dword[0] & MASK_MTX_MSG_CORE) >> SHIFT_MTX_MSG_CORE);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tCMDWORD_COUNT=%d\n", (comm_dword[0] & MASK_MTX_MSG_COUNT) >> SHIFT_MTX_MSG_COUNT);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tCMDWORD_DATA=0x%08x\n", comm_dword[1]);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tCMDWORD_ADDR=0x%08x\n", comm_dword[2]);
    drv_debug_msg(VIDEO_ENCODE_PDUMP,"\t\tCMDWORD_WBVALUE=0x%08x\n", comm_dword[3]);
    command_parameter_dump(cmdid, comm_dword[1], comm_dword[2]);

    return 0;
}
