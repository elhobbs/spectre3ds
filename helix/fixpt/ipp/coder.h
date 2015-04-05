/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

/**************************************************************************************
 * Fixed-point MP3 decoder using Intel IPP libraries
 * June 2003
 *
 * coder.h - definitions of data structures for IPP wrapper functions
 **************************************************************************************/

#ifndef _CODER_H
#define _CODER_H

#include "mp3common.h"

/* IPP definitions - may need to tweak names/paths for future versions of IPP */
#if defined(_WIN32) && defined(_M_IX86) && defined (HELIX_FEATURE_USE_IPP3) 
#include "ippMP.h"		/* for version 3.0 */
#else
#include "ippAC.h"
#endif


#if defined(HELIX_FEATURE_USE_IPP4)
extern const Ipp16s _ipp_pMP3SfbTableLong[14*6];
extern const Ipp16s _ipp_pMP3SfbTableShort[23*6];
extern IppMP3ScaleFactorBandTableLong Modified_Sfb_Table_Long;
extern IppMP3ScaleFactorBandTableShort Modified_Sfb_Table_Short;
extern IppMP3MixedBlockPartitionTable Modified_Mbp_Table;
#endif


typedef struct _FrameHeader {
	IppMP3FrameHeader fhIPP;
} FrameHeader;

typedef struct _SideInfo {
	IppMP3SideInfo siIPP[MAX_NCHAN*MAX_NGRAN];
	int privateBits;
	int scfsi[MAX_NCHAN][MAX_SCFBD];				/* 4 scalefactor bands per channel */
} SideInfo;

typedef struct _ScaleFactorInfo {
	/* scalefactors (1 or 2 granules, 1 or 2 channels) */
	signed char scaleFactor[MAX_NCHAN][IPP_MP3_SF_BUF_LEN];
} ScaleFactorInfo;

typedef struct _HuffmanInfo {
	int huffDecBuf[MAX_NCHAN][IPP_MP3_GRANULE_LEN];		/* used both for decoded Huffman values and dequantized coefficients */
	int nonZeroBound[MAX_NCHAN];						/* number of coeffs in huffDecBuf[ch] which can be > 0*/
} HuffmanInfo;

typedef struct _DequantInfo {
	int workBuf[IPP_MP3_GRANULE_LEN];
} DequantInfo;

typedef struct _IMDCTInfo {
	int outBuf[MAX_NCHAN][IPP_MP3_GRANULE_LEN];			/* output of IMDCT */	
	int overBuf[MAX_NCHAN][IPP_MP3_GRANULE_LEN];		/* overlap-add buffer (save for next time) */
	int numPrevIMDCT[MAX_NCHAN];						/* how many IMDCT's calculated in this channel on prev. granule */
} IMDCTInfo;

typedef struct _SubbandInfo {
	int vbuf[MAX_NCHAN][IPP_MP3_V_BUF_LEN];	/* vbuf for fast DCT-based synthesis PQMF */
	int vindex[MAX_NCHAN];						/* internal indices for tracking position in vbuf */
} SubbandInfo;

#endif	/* _CODER_H */
