/******************************************************************************
*
* Copyright (C) 2017 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"

#include "lib_common/SliceConsts.h"
#include "lib_common/BufCommon.h"
#include "lib_common/BufferAPI.h"

#define SIZE_PIXEL sizeof(uint16_t)

#define AL_MAX_HEIGHT 8192
#define SIZE_LCU_INFO 16   /*!< LCU compressed size + LCU offset                  */
#define SCD_SIZE 128 /*!< size of start code detector output                */

#define MAX_NAL_UNIT 4096/*!< Maximum nal unit within an access unit supported  */
#define NON_VCL_NAL_SIZE 2048/*!< Init size of the Deanti-emulated buffer used by the software to parse the high level syntax data  */
#define WP_SLICE_SIZE 256

static const int HEVC_LCU_CMP_SIZE[4] =
{
  832, 1088, 1344, 1856
}; /*!< HEVC LCU compressed max size*/
static const int AVC_LCU_CMP_SIZE[4] =
{
  800, 1120, 1408, 2016
}; /*!< AVC  LCU compressed max size*/

static const int POCOL_LIST_OFFSET = 128;
static const int MVCOL_LIST_OFFSET = 192;
static const int FBC_LIST_OFFSET = 256;
static const int SCLST_SIZE_DEC = 12288;
static const int LCU_SIZE_OFF = 160;
static const int WP_OFFSET = 168;
static const int REF_LIST_SIZE = 96 * sizeof(size_t);

/*************************************************************************//*!
   \brief Buffer with Poc list content
*****************************************************************************/
typedef TBuffer TBufferPOC;

static const int POCBUFF_PL_SIZE = 96;  // POC List size
static const int POCBUFF_LONG_TERM_OFFSET = 64;  // Long term flag List

/****************************************************************************/

#define MAX_SECTION 32 // greater or equal to 2 * (MAX_ENC_SLICE + 2)

/*************************************************************************//*!
   \brief List of references frame buffer
*****************************************************************************/
typedef struct t_BufferRef
{
  AL_TBuffer RefBuf; // adress of the corresponding frame buffer
  uint8_t uNodeID;
}TBufferRef, TBufferListRef[2][MAX_REF + 1];

/****************************************************************************/
uint32_t RndUpPow2(uint32_t uVal);

/****************************************************************************/
uint32_t RndPitch(uint32_t uWidth, uint8_t uBitDepth);

/****************************************************************************/
uint32_t RndHeight(uint32_t uHeight);

/*************************************************************************//*!
   \brief Retrieves the number of LCU in the frame
   \param[in] uWidth      Frame width in pixel unit
   \param[in] uHeight     Frame height in pixel unit
   \param[in] uLCUSize    Max Size of a Coding Unit
   \return number of LCU in the frame
*****************************************************************************/
uint32_t AL_GetNumLCU(uint16_t uWidth, uint16_t uHeight, uint8_t uLCUSize);

/*************************************************************************//*!
   \brief Retrieves the size of a HEVC compressed buffer(LCU header + MVDs + Residuals)
   \param[in] uWidth      Frame width in pixels
   \param[in] uHeight     Frame height in pixels
   \param[in] eChromaMode Chroma sub-sampling mode
   \return maximum size (in bytes) needed for the compressed buffer
*****************************************************************************/
uint32_t AL_GetAllocSize_HevcCompData(uint16_t uWidth, uint16_t uHeight, AL_EChromaMode eChromaMode);

/*************************************************************************//*!
   \brief Retrieves the size of a AVC compressed buffer(LCU header + MVDs + Residuals)
   \param[in] uWidth      Frame width in pixels
   \param[in] uHeight     Frame height in pixels
   \param[in] eChromaMode Chroma sub-sampling mode
   \return maximum size (in bytes) needed for the compressed buffer
*****************************************************************************/
uint32_t AL_GetAllocSize_AvcCompData(uint16_t uWidth, uint16_t uHeight, AL_EChromaMode eChromaMode);

/*************************************************************************//*!
   \brief Retrieves the size of the compressed map buffer (LCU Offset & size)
   \param[in] uWidth  Frame width in pixels
   \param[in] uHeight Frame height in pixels
   \return maximum size (in bytes) needed for the compressed map buffer
*****************************************************************************/
uint32_t AL_GetAllocSize_CompMap(uint16_t uWidth, uint16_t uHeight);

/*************************************************************************//*!
   \brief Retrieves the size of a motion vector buffer
   \param[in] uWidth   Frame Width in pixel
   \param[in] uHeight  Frame Height in pixel
   \param[in] bIsAvc   Specifies if the standard used is AVC or HEVC
   \return the size (in bytes) needed for the colocated frame buffer
*****************************************************************************/
uint32_t AL_GetAllocSize_MV(uint16_t uWidth, uint16_t uHeight, bool bIsAvc);

/*************************************************************************//*!
   \brief Retrieves the size of the output frame buffer
   \param[in] tDimension  Frame dimension (width, height) in pixel
   \param[in] eChromaMode Chroma mode
   \param[in] uBitDepth   Bit Depth
   \param[in] bFrameBufferCompression Specifies if compression is needed
   \param[in] eFrameBufferStorageMode Storage Mode of the frame buffer
   \return the size (in bytes) needed for the output frame buffer
*****************************************************************************/
int AL_GetAllocSize_Frame(AL_TDimension tDimension, AL_EChromaMode eChromaMode, uint8_t uBitDepth, bool bFrameBufferCompression, AL_EFbStorageMode eFrameBufferStorageMode);

/*************************************************************************//*!
   \brief Retrieves the size of a reference frame buffer
   \param[in] uWidth      frame width in pixels units
   \param[in] uHeight     frame height in pixels units
   \param[in] eChromaMode Chroma subsampling
   \param[in] uBitDepth   Size in bits of picture samples
   \return the size (in bytes) needed for reference frame buffer
*****************************************************************************/
uint32_t AL_GetAllocSize_Reference(uint16_t uWidth, uint16_t uHeight, AL_EChromaMode eChromaMode, uint8_t uBitDepth);

/*************************************************************************//*!
   \brief Retrieves the size of the luma map
   \param[in] uWidth      frame width in pixels units
   \param[in] uHeight     frame height in pixels units
   \param[in] eFrameBufferStorageMode Storage Mode of the frame buffer
   \return the size (in bytes) needed for the luma map
*****************************************************************************/
uint32_t AL_GetAllocSize_LumaMap(uint16_t uWidth, uint16_t uHeight, AL_EFbStorageMode eFrameBufferStorageMode);

/*@}*/

