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
****************************************************************************/
#include "lib_preprocess/PreprocessQP.h"
#include "lib_common_enc/EncBuffers.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "lib_rtos/lib_rtos.h"
#undef max

/****************************************************************************/
static int FromHex2(char a, char b)
{
  int A = ((a >= 'a') && (a <= 'f')) ? (a - 'a') + 10 :
          ((a >= 'A') && (a <= 'F')) ? (a - 'A') + 10 :
          ((a >= '0') && (a <= '9')) ? (a - '0') : 0;

  int B = ((b >= 'a') && (b <= 'f')) ? (b - 'a') + 10 :
          ((b >= 'A') && (b <= 'F')) ? (b - 'A') + 10 :
          ((b >= '0') && (b <= '9')) ? (b - '0') : 0;

  return (A << 4) + B;
}

/****************************************************************************/
static int FromHex4(char a, char b, char c, char d)
{
  return (FromHex2(a, b) << 8) + FromHex2(c, d);
}

/****************************************************************************/
void Generate_RampQP_VP9(uint8_t* pSegs, uint8_t* pQPs, int iNumLCUs, int iMinQP, int iMaxQP)
{
  static int16_t s_iQP = 0;
  static uint8_t s_iCurSeg = 0;
  int iStepQP;
  int iSeg, iLCU;
  int16_t* pSeg;

  if(s_iQP < iMinQP)
    s_iQP = iMinQP;

  iStepQP = (iMaxQP - iMinQP) >> 3;
  pSeg = (int16_t*)pSegs;

  for(iSeg = 0; iSeg < 8; iSeg++)
  {
    pSeg[iSeg] = (s_iQP += iStepQP) & 0xFF;

    if(s_iQP > iMaxQP)
      s_iQP = iMinQP;
  }

  s_iCurSeg = 0;

  for(iLCU = 0; iLCU < iNumLCUs; ++iLCU)
  {
    pQPs[iLCU] = (s_iCurSeg % 8);
    s_iCurSeg++;
  }
}

/****************************************************************************/
void Generate_RampQP(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int iMinQP, int iMaxQP)
{
  static int8_t s_iQP = 0;
  int iLCU;

  if(s_iQP < iMinQP)
    s_iQP = iMinQP;

  for(iLCU = 0; iLCU < iNumLCUs; iLCU++)
  {
    int iFirst = iNumBytesPerLCU * iLCU;
    int iQP;

    for(iQP = 0; iQP < iNumQPPerLCU; ++iQP)
      pQPs[iFirst + iQP] = s_iQP & MASK_QP;

    if(++s_iQP > iMaxQP)
      s_iQP = iMinQP;
  }
}

/****************************************************************************/
void Generate_RandomQP_VP9(uint8_t* pSegs, uint8_t* pQPs, int iNumLCUs, int iMinQP, int iMaxQP, int16_t iSliceQP)
{
  static int iRandQP = 0;
  int iConvSliceQP = iSliceQP % 52;
  uint32_t iSeed = iNumLCUs * iConvSliceQP - (0xEFFACE << (iConvSliceQP >> 1)) + iRandQP++;
  uint32_t iRand = iSeed;
  int iRange = iMaxQP - iMinQP + 1;

  int16_t* pSeg = (int16_t*)pSegs;
  int iSeg;
  int iLCU;

  for(iSeg = 0; iSeg < 8; iSeg++)
  {
    iRand = (1103515245 * iRand + 12345); // Unix
    pSeg[iSeg] = (iMinQP + (iRand % iRange));
  }

  for(iLCU = 0; iLCU < iNumLCUs; iLCU++)
  {
    iRand = (1103515245 * iRand + 12345); // Unix
    pQPs[iLCU] = (iRand % 8);
  }
}

/****************************************************************************/
void Generate_RandomQP(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int iMinQP, int iMaxQP, int16_t iSliceQP)
{
  static int iRandQP = 0;

  uint32_t iSeed = iNumLCUs * iSliceQP - (0xEFFACE << (iSliceQP >> 1)) + iRandQP++;
  uint32_t iRand = iSeed;

  int iRange = iMaxQP - iMinQP + 1;
  int iLCU;

  for(iLCU = 0; iLCU < iNumLCUs; ++iLCU)
  {
    int iFirst = iLCU * iNumBytesPerLCU;
    int iQP;

    for(iQP = 0; iQP < iNumQPPerLCU; ++iQP)
    {
      iRand = (1103515245 * iRand + 12345); // Unix
      pQPs[iFirst + iQP] = (iMinQP + (iRand % iRange)) & MASK_QP;
    }
  }
}

/****************************************************************************/
void Generate_BorderQP(uint8_t* pQPs, int iNumLCUs, int iLCUWidth, int iLCUHeight, int iNumQPPerLCU, int iNumBytesPerLCU, int iMaxQP, int16_t iSliceQP, bool bRelative)
{
  int iQP0 = bRelative ? 0 : iSliceQP;
  int iQP2 = iQP0 + 2;
  int iQP1;

  const int iFirstX2 = 0;
  const int iFirstX1 = 1;
  const int iLastX2 = iLCUWidth - 1;
  const int iLastX1 = iLCUWidth - 2;

  const int iFirstY2 = 0;
  const int iFirstY1 = 1;
  const int iLastY2 = iLCUHeight - 1;
  const int iLastY1 = iLCUHeight - 2;

  int iFirstLCU = 0;
  int iLastLCU = iNumLCUs - 1;
  int iLCU;

  if(iQP2 > iMaxQP)
    iQP2 = iMaxQP;
  iQP1 = iQP0 + 1;

  if(iQP1 > iMaxQP)
    iQP1 = iMaxQP;

  for(iLCU = iFirstLCU; iLCU <= iLastLCU; iLCU++)
  {
    int X = iLCU % iLCUWidth;
    int Y = iLCU / iLCUWidth;
    int iQP;

    int iFirst = iNumBytesPerLCU * iLCU;

    if(X == iFirstX2 || Y == iFirstY2 || X == iLastX2 || Y >= iLastY2)
      pQPs[iFirst] = iQP2;
    else if(X == iFirstX1 || Y == iFirstY1 || X == iLastX1 || Y == iLastY1)
      pQPs[iFirst] = iQP1;
    else
      pQPs[iFirst] = iQP0;

    for(iQP = 1; iQP < iNumQPPerLCU; ++iQP)
      pQPs[iFirst + iQP] = pQPs[iFirst];
  }
}

/****************************************************************************/
void Generate_BorderQP_VP9(uint8_t* pSegs, uint8_t* pQPs, int iNumLCUs, int iLCUWidth, int iLCUHeight, int iMaxQP, int16_t iSliceQP, bool bRelative)
{
  int iQP0 = bRelative ? 0 : iSliceQP;
  int iQP2 = iQP0 + 10;
  int iQP1;

  int iFirstLCU = 0;
  int iLastLCU = iNumLCUs - 1;

  const int iFirstX2 = 0;
  const int iFirstX1 = 1;
  const int iLastX2 = iLCUWidth - 1;
  const int iLastX1 = iLCUWidth - 2;

  const int iFirstY2 = 0;
  const int iFirstY1 = 1;
  const int iLastY2 = iLCUHeight - 1;
  const int iLastY1 = iLCUHeight - 2;
  int16_t* pSeg = (int16_t*)pSegs;
  int iLCU;

  if(iQP2 > iMaxQP)
    iQP2 = iMaxQP;
  iQP1 = iQP0 + 5;

  if(iQP1 > iMaxQP)
    iQP1 = iMaxQP;

  Rtos_Memset(pSeg, 0, 8 * sizeof(int16_t));
  pSeg[0] = iQP0;
  pSeg[1] = iQP1;
  pSeg[2] = iQP2;

  // write Map
  for(iLCU = iFirstLCU; iLCU <= iLastLCU; iLCU++)
  {
    int X = iLCU % iLCUWidth;
    int Y = iLCU / iLCUWidth;

    if(X == iFirstX2 || Y == iFirstY2 || X == iLastX2 || Y >= iLastY2)
      pQPs[iLCU] = 2;
    else if(X == iFirstX1 || Y == iFirstY1 || X == iLastX1 || Y == iLastY1)
      pQPs[iLCU] = 1;
    else
      pQPs[iLCU] = 0;
  }
}

/****************************************************************************/
static void ReadQPs(FILE* qpFile, uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU)
{
  char sLine[256];

  int iNumQPPerLine = (iNumQPPerLCU == 5) ? 5 : 1;
  int iNumDigit = iNumQPPerLine * 2;

  int iIdx = 0;
  int iLCU;

  for(iLCU = 0; iLCU < iNumLCUs; ++iLCU)
  {
    int iFirst = iNumBytesPerLCU * iLCU;
    int iQP;

    for(iQP = 0; iQP < iNumQPPerLCU; ++iQP)
    {
      if(iIdx == 0)
        fgets(sLine, 256, qpFile);

      pQPs[iFirst + iQP] = FromHex2(sLine[iNumDigit - 2 * iIdx - 2], sLine[iNumDigit - 2 * iIdx - 1]);

      iIdx = (iIdx + 1) % iNumQPPerLine;
    }
  }
}

#ifdef _MSC_VER
#include <stdarg.h>
static AL_INLINE
int LIBSYS_SNPRINTF(char* str, size_t size, const char* format, ...)
{
  int retval;
  va_list ap;
  va_start(ap, format);
  retval = _vsnprintf(str, size, format, ap);
  va_end(ap);
  return retval;
}

#define snprintf LIBSYS_SNPRINTF
#endif

char* createQPFileNameWithID(int iFrameID)
{
  char* str = NULL;
  char* format_str = DEBUG_PATH "/QP_%d.hex";
  int numberCharNeeded = snprintf(str, 0, format_str, iFrameID) + 1;
  str = (char*)malloc(numberCharNeeded * sizeof(char));
  numberCharNeeded = snprintf(str, numberCharNeeded, format_str, iFrameID);

  if(numberCharNeeded < 0)
  {
    free(str);
    return NULL;
  }
  return str;
}

char* createQPFileName()
{
  char* str = strdup(DEBUG_PATH "/QPs.hex");
  return str;
}

void freeQPFileName(char* filename)
{
  free(filename);
}

/****************************************************************************/
bool Load_QPTable_FromFile_Vp9(uint8_t* pSegs, uint8_t* pQPs, int iNumLCUs, int iFrameID, bool bRelative)
{
  char* qpFileName = createQPFileNameWithID(iFrameID);
  FILE* qpFile = fopen(qpFileName, "r");
  char sLine[256];
  int16_t* pSeg = (int16_t*)pSegs;
  int iSeg;

  if(!qpFile)
    qpFileName = createQPFileName();
  qpFile = fopen(qpFileName, "r");

  if(!qpFile)
  {
    freeQPFileName(qpFileName);
    return false;
  }

  for(iSeg = 0; iSeg < 8; ++iSeg)
  {
    int idx = (iSeg & 0x01) << 2;

    if(idx == 0)
      fgets(sLine, 256, qpFile);
    pSeg[iSeg] = FromHex4(sLine[4 - idx], sLine[5 - idx], sLine[6 - idx], sLine[7 - idx]);

    if(!bRelative && (pSeg[iSeg] < 0 || pSeg[iSeg] > 255))
      pSeg[iSeg] = 255;
  }

  // read QPs
  ReadQPs(qpFile, pQPs, iNumLCUs, 1, 1);

  freeQPFileName(qpFileName);

  return true;
}

/****************************************************************************/
bool Load_QPTable_FromFile(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int iFrameID)
{
  char* qpFileName = createQPFileNameWithID(iFrameID);
  FILE* qpFile = fopen(qpFileName, "r");

  if(!qpFile)
    qpFileName = createQPFileName();
  qpFile = fopen(qpFileName, "r");

  if(!qpFile)
  {
    freeQPFileName(qpFileName);
    return false;
  }
  // Warning : the LOAD_QP is not backward compatible
  ReadQPs(qpFile, pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU);

  freeQPFileName(qpFileName);

  return true;
}

/****************************************************************************/
void Generate_FullSkip(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU)
{
  int iLCU;

  for(iLCU = 0; iLCU < iNumLCUs; iLCU++)
  {
    int iFirst = iLCU * iNumBytesPerLCU;
    int iQP;

    for(iQP = 0; iQP < iNumQPPerLCU; ++iQP)
    {
      pQPs[iFirst + iQP] &= ~MASK_FORCE;
      pQPs[iFirst + iQP] |= MASK_FORCE_MV0;
    }
  }
}

/****************************************************************************/
void Generate_BorderSkip(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int iLCUWidth, int iLCUHeight)
{
  int H = iLCUHeight * 2 / 6;
  int W = iLCUWidth * 2 / 6;
  int iLCU;

  H *= H;
  W *= W;

  for(iLCU = 0; iLCU < iNumLCUs; iLCU++)
  {
    int X = (iLCU % iLCUWidth) - (iLCUWidth >> 1);
    int Y = (iLCU / iLCUWidth) - (iLCUHeight >> 1);

    int iFirst = iNumBytesPerLCU * iLCU;
    int iQP;

    if(100 * X * X / W + 100 * Y * Y / H > 100)
    {
      pQPs[iFirst] &= ~MASK_FORCE;
      pQPs[iFirst] |= MASK_FORCE_MV0;
    }

    for(iQP = 1; iQP < iNumQPPerLCU; ++iQP)
      pQPs[iFirst + iQP] = pQPs[iFirst];
  }
}

/****************************************************************************/
void Generate_Random_WithFlag(uint8_t* pQPs, int iNumLCUs, int iNumQPPerLCU, int iNumBytesPerLCU, int16_t iSliceQP, int iRandFlag, int iPercent, uint8_t uFORCE)
{
  int iLimitQP = iSliceQP % 52;
  int iSeed = iNumLCUs * iLimitQP - (0xEFFACE << (iLimitQP >> 1)) + iRandFlag;
  int iRand = iSeed;
  int iLCU;

  for(iLCU = 0; iLCU < iNumLCUs; iLCU++)
  {
    int iFirst = iNumBytesPerLCU * iLCU;

    if(!(pQPs[iFirst] & MASK_FORCE))
    {
      int iQP;

      for(iQP = 0; iQP < iNumQPPerLCU; ++iQP)
      {
        if((pQPs[iFirst + iQP] & MASK_FORCE) != (pQPs[iFirst] & MASK_FORCE)) // remove existing flag if different from depth 0
          pQPs[iFirst + iQP] &= ~MASK_FORCE;

        iRand = (1103515245 * iRand + 12345); // Random formula from Unix

        if(abs(iRand) % 100 <= iPercent)
          pQPs[iFirst + iQP] |= uFORCE;
      }
    }
  }
}

/****************************************************************************/
bool PreprocessQP(AL_EQpCtrlMode eMode, int16_t iSliceQP, int16_t iMinQP, int16_t iMaxQP, int iLCUWidth, int iLCUHeight, AL_EProfile eProf, int iFrameID, uint8_t* pQPs, uint8_t* pSegs)
{
  (void)eProf;
  bool bIsVp9 = false;

  int iNumQPPerLCU = 1;
  int iNumBytesPerLCU = 1;

  bool bRelative = (eMode & RELATIVE_QP) ? true : false;
  const int iMaxLCUs = iLCUWidth * iLCUHeight;
  int iSize = ((((iMaxLCUs * iNumBytesPerLCU) + 127) >> 7) << 7);
  const int iNumLCUs = iMaxLCUs;
  int iQPMode = eMode & 0x0F; // exclusive mode
  bool bRet = false;
  static int iRandFlag = 0;

  assert(pQPs);
  Rtos_Memset(pQPs, 0, iSize);

  if(bIsVp9)
    Rtos_Memset(pSegs, 0, 8 * sizeof(int16_t));

  if(bRelative)
  {
    int iMinus = bIsVp9 ? 128 : 32;
    int iPlus = bIsVp9 ? 127 : 31;

    if(iQPMode == RANDOM_QP)
    {
      iMinQP = -iMinus;
      iMaxQP = iPlus;
    }
    else
    {
      iMinQP = (iSliceQP - iMinus < iMinQP) ? iMinQP - iSliceQP : -iMinus;
      iMaxQP = (iSliceQP + iPlus > iMaxQP) ? iMaxQP - iSliceQP : iPlus;
    }
  }
  /////////////////////////////////  QPs  /////////////////////////////////////
  switch(iQPMode)
  {
  case RAMP_QP:
  {
    bIsVp9 ? Generate_RampQP_VP9(pSegs, pQPs, iNumLCUs, iMinQP, iMaxQP) :
    Generate_RampQP(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iMinQP, iMaxQP);
    bRet = true;
  } break;
  // ------------------------------------------------------------------------
  case RANDOM_QP:
  {
    bIsVp9 ? Generate_RandomQP_VP9(pSegs, pQPs, iNumLCUs, iMinQP, iMaxQP, iSliceQP) :
    Generate_RandomQP(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iMinQP, iMaxQP, iSliceQP);
    bRet = true;
  } break;
  // ------------------------------------------------------------------------
  case BORDER_QP:
  {
    bIsVp9 ? Generate_BorderQP_VP9(pSegs, pQPs, iNumLCUs, iLCUWidth, iLCUHeight, iMaxQP, iSliceQP, bRelative) :
    Generate_BorderQP(pQPs, iNumLCUs, iLCUWidth, iLCUHeight, iNumQPPerLCU, iNumBytesPerLCU, iMaxQP, iSliceQP, bRelative);
    bRet = true;
  } break;
  // ------------------------------------------------------------------------
  case LOAD_QP:
  {
    bRet = bIsVp9 ? Load_QPTable_FromFile_Vp9(pSegs, pQPs, iNumLCUs, iFrameID, bRelative) :
           Load_QPTable_FromFile(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iFrameID);
  } break;
    // ------------------------------------------------------------------------
  }

  // ------------------------------------------------------------------------
  if((eMode != UNIFORM_QP) && (iQPMode == UNIFORM_QP) && !bRelative)
  {
    int s;
    int iLCU;

    if(bIsVp9)
      for(s = 0; s < 8; ++s)
        pSegs[2 * s] = iSliceQP;

    else
      for(iLCU = 0; iLCU < iNumLCUs; iLCU++)
      {
        int iFirst = iLCU * iNumBytesPerLCU;
        int iQP;

        for(iQP = 0; iQP < iNumQPPerLCU; ++iQP)
          pQPs[iFirst + iQP] = iSliceQP;
      }
  }
  // ------------------------------------------------------------------------

  if(eMode & RANDOM_I_ONLY)
  {
    Generate_Random_WithFlag(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iSliceQP, iRandFlag++, 20, MASK_FORCE_INTRA); // 20 percent
    bRet = true;
  }

  // ------------------------------------------------------------------------
  if(eMode & RANDOM_SKIP)
  {
    Generate_Random_WithFlag(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iSliceQP, iRandFlag++, 30, MASK_FORCE_MV0); // 30 percent
    bRet = true;
  }

  // ------------------------------------------------------------------------
  if(eMode & FULL_SKIP)
  {
    Generate_FullSkip(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU);
    bRet = true;
  }
  else if(eMode & BORDER_SKIP)
  {
    Generate_BorderSkip(pQPs, iNumLCUs, iNumQPPerLCU, iNumBytesPerLCU, iLCUWidth, iLCUHeight);
    bRet = true;
  }

  return bRet;
}

/****************************************************************************/

