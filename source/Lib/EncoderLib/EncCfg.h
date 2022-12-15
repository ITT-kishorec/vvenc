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
/** \file     EncCfg.h
    \brief    encoder internal configuration class (header)
*/

#pragma once

#include "vvenc/vvencCfg.h"

namespace vvenc {

struct VVEncCfg : public vvenc_config
{

  VVEncCfg();

  VVEncCfg& operator= ( const vvenc_config& extern_cfg );

  bool m_stageParallelProc;
  #ifdef VVENC_FEATURE_FGS
  public:

  int  xGetFGCSEIModelValue(int compNumber, int intensityIntervalNumber, int modelNumber) const
  {
    const std::vector<int>* cmv;

    int rowSize;
    int rowNumber = intensityIntervalNumber;
    int columnNumber = modelNumber;
    
    if(compNumber == 0)
    {
      cmv = &(m_compModelValuesComp0);
      rowSize = cmv->size()/(m_numIntensityIntervalsMinus1Comp0+1);
    } 
    if(compNumber == 1)
    {
      cmv = &(m_compModelValuesComp1);
      rowSize = cmv->size()/(m_numIntensityIntervalsMinus1Comp1+1);
    }
    if(compNumber == 2)
    {
      cmv = &(m_compModelValuesComp2);
      rowSize = cmv->size()/(m_numIntensityIntervalsMinus1Comp2+1);
    }
    
    return (*cmv)[rowNumber * rowSize + columnNumber];
  }
#endif


private:
  void xInitCfgMembers();
  
};


}


