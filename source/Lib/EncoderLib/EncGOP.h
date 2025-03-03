/* -----------------------------------------------------------------------------
The copyright in this software is being made available under the Clear BSD
License, included below. No patent rights, trademark rights and/or 
other Intellectual Property Rights other than the copyrights concerning 
the Software are granted under this license.

The Clear BSD License

Copyright (c) 2019-2022, Fraunhofer-Gesellschaft zur Förderung der angewandten Forschung e.V. & The VVenC Authors.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted (subject to the limitations in the disclaimer below) provided that
the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

     * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

     * Neither the name of the copyright holder nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.


------------------------------------------------------------------------------------------- */
/** \file     EncGOP.h
    \brief    GOP encoder class (header)
*/

#pragma once

#include "EncSampleAdaptiveOffset.h"
#include "VLCWriter.h"
#include "SEIwrite.h"
#include "SEIEncoder.h"
#include "Analyze.h"
#include "EncPicture.h"
#include "EncReshape.h"
#include "CommonLib/Picture.h"
#include "CommonLib/CommonDef.h"
#include "CommonLib/Nal.h"
#include "EncHRD.h"
#include "EncStage.h"

#include <vector>
#include <list>
#include <stdlib.h>
#include <atomic>


#include "Utilities/NoMallocThreadPool.h"
#include <mutex>
#include <condition_variable>

//! \ingroup EncoderLib
//! \{

namespace vvenc {

// ====================================================================================================================

class InputByteStream;
class DecLib;
class EncHRD;
class MsgLog;

struct FFwdDecoder
{
  bool bDecode1stPart;
  bool bHitFastForwardPOC;
  bool loopFiltered;
  int  iPOCLastDisplay;
  std::ifstream* bitstreamFile;
  InputByteStream* bytestream;
  DecLib *pcDecLib;

  FFwdDecoder()
    : bDecode1stPart      ( true )
      , bHitFastForwardPOC( false )
      , loopFiltered      ( false )
      , iPOCLastDisplay   ( -MAX_INT )
      , bitstreamFile     ( nullptr )
      , bytestream        ( nullptr )
      , pcDecLib          ( nullptr )
  {}
};

// ====================================================================================================================

class EncGOP;
struct FinishTaskParam {
  EncGOP*     gopEncoder;
  EncPicture* picEncoder;
  Picture*    pic;
  FinishTaskParam()                                          : gopEncoder( nullptr ), picEncoder( nullptr ), pic( nullptr ) {}
  FinishTaskParam( EncGOP* _g, EncPicture* _e, Picture* _p ) : gopEncoder( _g ),      picEncoder( _e ),      pic( _p )      {}
};

// ====================================================================================================================

class EncGOP : public EncStage
{
private:
  MsgLog&                   msg;

  Analyze                   m_AnalyzeAll;
  Analyze                   m_AnalyzeI;
  Analyze                   m_AnalyzeP;
  Analyze                   m_AnalyzeB;

  std::function<void( void*, vvencYUVBuffer* )> m_recYuvBufFunc;
  void*                                         m_recYuvBufCtx;

  NoMallocThreadPool*       m_threadPool;
  std::mutex                m_gopEncMutex;
  std::condition_variable   m_gopEncCond;
  XUCache                   m_shrdUnitCache;
  std::mutex                m_unitCacheMutex;

  const VVEncCfg*           m_pcEncCfg;
  RateCtrl*                 m_pcRateCtrl;
  HLSWriter                 m_HLSWriter;
  SEIWriter                 m_seiWriter;
  SEIEncoder                m_seiEncoder;
  EncReshape                m_Reshaper;
  BlkStat                   m_BlkStat;
  FFwdDecoder               m_ffwdDecoder;

  ParameterSetMap<APS>      m_gopApsMap;
  ParameterSetMap<SPS>      m_spsMap;
  ParameterSetMap<PPS>      m_ppsMap;
  EncHRD                    m_EncHRD;
  VPS                       m_VPS;
  DCI                       m_DCI;

  bool                      m_isPreAnalysis;
  bool                      m_bFirstInit;
  bool                      m_bFirstWrite;
  bool                      m_bRefreshPending;
  bool                      m_disableLMCSIP;
  int                       m_numPicsCoded;
  int                       m_pocEncode;
  int                       m_pocRecOut;
  int                       m_gopSizeLog2;
  int                       m_ticksPerFrameMul4;
  int                       m_codingOrderIdx;
  int                       m_lastIDR;
  int                       m_lastRasPoc;
  int                       m_pocCRA;
  int                       m_appliedSwitchDQQ;
  int                       m_associatedIRAPPOC;
  vvencNalUnitType          m_associatedIRAPType;

  std::list<EncPicture*>    m_freePicEncoderList;
  std::list<Picture*>       m_gopEncListInput;
  std::list<Picture*>       m_gopEncListOutput;
  std::list<Picture*>       m_procList;
  std::list<Picture*>       m_rcUpdateList;

  std::vector<int>          m_pocToGopId;
  std::vector<int>          m_nextPocOffset;
  std::vector<int>          m_globalCtuQpVector;

  bool                      m_trySkipOrDecodePicture;
#if LMCS_CONTROL_PARAM_EVALUATE
  std::vector<picStat>      m_gopTemporalActivity;
  int                       m_numGOPStatsProcessed;
#endif

public:
  EncGOP( MsgLog& msglog );
  virtual ~EncGOP();

  void setRecYUVBufferCallback( void* ctx, std::function<void( void*, vvencYUVBuffer* )> func );

  const EncReshape& getReshaper() const { return m_Reshaper; }

  void init               ( const VVEncCfg& encCfg, RateCtrl& rateCtrl, NoMallocThreadPool* threadPool, bool isPreAnalysis );
  void picInitRateControl ( Picture& pic, Slice* slice, EncPicture *picEncoder );
  void printOutSummary    ( const bool printMSEBasedSNR, const bool printSequenceMSE, const bool printHexPsnr );
  void getParameterSets   ( AccessUnitList& accessUnit );
  bool nextPicReadyForOutput    () { return !m_gopEncListOutput.empty() && m_gopEncListOutput.front()->isReconstructed; }

protected:
  virtual void initPicture    ( Picture* pic );
  virtual void processPictures( const PicList& picList, bool flush, AccessUnitList& auList, PicList& doneList, PicList& freeList );
  virtual void waitForFreeEncoders();

private:
  int  xGetGopIdFromPoc               ( int poc ) const { return m_pocToGopId[ poc % m_pcEncCfg->m_GOPSize ]; }

  int  xGetNextPocICO                 ( int poc, int max, bool altGOP ) const;
  Picture* xFindPicture               ( const PicList& picList, int poc ) const;
  void xCreateCodingOrder             ( const PicList& picList, bool flush, std::vector<Picture*>& encList ) const;
  void xUpdateRasInit                 ( Slice* slice );
  void xEncodePictures                ( bool flush, AccessUnitList& auList, PicList& doneList );
  void xOutputRecYuv                  ( const PicList& picList );
  void xReleasePictures               ( const PicList& picList, PicList& freeList, bool allDone );

  void xInitVPS                       ( VPS &vps ) const;
  void xInitDCI                       ( DCI &dci, const SPS &sps, const int dciId ) const;
  void xInitConstraintInfo            ( ConstraintInfo &ci ) const;
  void xInitSPS                       ( SPS &sps ) const;
  void xInitPPS                       ( PPS &pps, const SPS &sps ) const;
  void xInitPPSforTiles               ( PPS &pps, const SPS &sps ) const;
  void xInitRPL                       ( SPS &sps ) const;
  void xInitHrdParameters             ( SPS &sps );

  vvencNalUnitType xGetNalUnitType    ( int pocCurr, int lastIdr ) const;
  int  xGetSliceDepth                 ( int poc ) const;
  bool xIsSliceTemporalSwitchingPoint ( const Slice* slice, const PicList& picList, int gopId ) const;

  void xInitPicsInCodingOrder         ( const std::vector<Picture*>& encList, const PicList& picList, bool isEncodeLtRef );
  void xGetProcessingLists            ( std::list<Picture*>& procList, std::list<Picture*>& rcUpdateList );
  void xInitFirstSlice                ( Picture& pic, const PicList& picList, bool isEncodeLtRef );
  void xInitSliceTMVPFlag             ( PicHeader* picHeader, const Slice* slice, int gopId );
  void xUpdateRPRtmvp                 ( PicHeader* picHeader, Slice* slice );
  void xInitSliceMvdL1Zero            ( PicHeader* picHeader, const Slice* slice );
  void xInitLMCS                      ( Picture& pic );
  void xSelectReferencePictureList    ( Slice* slice, int curPoc, int gopId, int ltPoc );
  void xSyncAlfAps                    ( Picture& pic, ParameterSetMap<APS>& dst, const ParameterSetMap<APS>& src );

  void xWritePicture                  ( Picture& pic, AccessUnitList& au, bool isEncodeLtRef );
  int  xWriteParameterSets            ( Picture& pic, AccessUnitList& accessUnit, HLSWriter& hlsWriter );
  int  xWritePictureSlices            ( Picture& pic, AccessUnitList& accessUnit, HLSWriter& hlsWriter );
  void xWriteLeadingSEIs              ( const Picture& pic, AccessUnitList& accessUnit );
  void xWriteTrailingSEIs             ( const Picture& pic, AccessUnitList& accessUnit, std::string& digestStr );
  int  xWriteVPS                      ( AccessUnitList &accessUnit, const VPS *vps, HLSWriter& hlsWriter );
  int  xWriteDCI                      ( AccessUnitList &accessUnit, const DCI *dci, HLSWriter& hlsWriter );
  int  xWriteSPS                      ( AccessUnitList &accessUnit, const SPS *sps, HLSWriter& hlsWriter );
  int  xWritePPS                      ( AccessUnitList &accessUnit, const PPS *pps, const SPS *sps, HLSWriter& hlsWriter );
  int  xWriteAPS                      ( AccessUnitList &accessUnit, const APS *aps, HLSWriter& hlsWriter, vvencNalUnitType eNalUnitType );
  void xWriteAccessUnitDelimiter      ( AccessUnitList &accessUnit, Slice* slice, bool IrapOrGdr, HLSWriter& hlsWriter );
  void xWriteSEI                      ( vvencNalUnitType naluType, SEIMessages& seiMessages, AccessUnitList &accessUnit, AccessUnitList::iterator &auPos, int temporalId, const SPS *sps );
  void xWriteSEISeparately            ( vvencNalUnitType naluType, SEIMessages& seiMessages, AccessUnitList &accessUnit, AccessUnitList::iterator &auPos, int temporalId, const SPS *sps );
  void xAttachSliceDataToNalUnit      ( OutputNALUnit& rNalu, const OutputBitstream* pcBitstreamRedirect );
  void xCabacZeroWordPadding          ( const Picture& pic, const Slice* slice, uint32_t binCountsInNalUnits, uint32_t numBytesInVclNalUnits, std::ostringstream &nalUnitData );

  void xCalculateAddPSNR              ( const Picture* pic, CPelUnitBuf cPicD, AccessUnitList&, bool printFrameMSE, double* PSNR_Y, bool isEncodeLtRef );
  uint64_t xFindDistortionPlane       ( const CPelBuf& pic0, const CPelBuf& pic1, uint32_t rshift ) const;
  void xPrintPictureInfo              ( const Picture& pic, AccessUnitList& accessUnit, const std::string& digestStr, bool printFrameMSE, bool isEncodeLtRef );
};// END CLASS DEFINITION EncGOP

} // namespace vvenc

//! \}

