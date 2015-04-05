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
 * subband.c - wrapper for IPP primitive which runs synthesis filter
 **************************************************************************************/

#include "coder.h"	

#define BLOCK_SIZE	18
#define	NBANDS		32

/**************************************************************************************
 * Function:    Subband
 *
 * Description: do subband transform on all the blocks in one granule, all channels
 *
 * Inputs:      filled MP3DecInfo structure, after calling IMDCT for all channels
 *              vbuf[ch] and vindex[ch] must be preserved between calls
 *
 * Outputs:     decoded PCM data, interleaved LRLRLR... if stereo
 *
 * Return:      0 on success,  -1 if null input pointers
 **************************************************************************************/
int Subband(MP3DecInfo *mp3DecInfo, short *pcmBuf)
{
	int b, ch, iMode;
	short *pcmPtr;
	IMDCTInfo *mi;
	SubbandInfo *sbi;
	IppStatus ippStatus = ippStsNoErr;

	/* validate pointers */
	if (!mp3DecInfo || !mp3DecInfo->IMDCTInfoPS || !mp3DecInfo->SubbandInfoPS)
		return -1;

	mi = (IMDCTInfo *)(mp3DecInfo->IMDCTInfoPS);
	sbi = (SubbandInfo*)(mp3DecInfo->SubbandInfoPS);

	iMode = (mp3DecInfo->nChans == 1 ? 1 : 2);
	for (ch = 0; ch < mp3DecInfo->nChans; ch++) {
		/* IPP primitive #7 - subband transform 
		 *   mode 2 means output samples are skipped by 2 (fills pcmBuf[0, 2, 4...])
		 *    - for stereo, we start at &pcmBuf[0] first time (L) and &pcmBuf[1] second time (R)
		 *    - for mono, we still use mode 1 (no interleaving)
		 */
		pcmPtr = pcmBuf + ch;		/* offset for stereo interleaving */
		for(b = 0; b < BLOCK_SIZE; b++) {
			ippStatus = ippsSynthPQMF_MP3_32s16s(mi->outBuf[ch] + b*NBANDS,	pcmPtr, sbi->vbuf[ch], &(sbi->vindex[ch]), iMode);
			pcmPtr += NBANDS * mp3DecInfo->nChans;
			if (ippStatus != ippStsNoErr)
				return -1;
		}
	}

	return 0;
}
