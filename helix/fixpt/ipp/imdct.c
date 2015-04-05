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
 * imdct.c - wrapper for IPP primitive which calculates inverse MDCT
 **************************************************************************************/

#include "coder.h"	

/**************************************************************************************
 * Function:    IMDCT
 *
 * Description: do alias reduction, inverse MDCT, overlap-add, and frequency inversion
 *
 * Inputs:      MP3DecInfo structure filled by UnpackFrameHeader(), UnpackSideInfo(),
 *                UnpackScaleFactors(), and DecodeHuffman() (for this granule, channel)
 *                includes PCM samples in overBuf (from last call to IMDCT) for OLA
 *              index of current granule and channel
 *
 * Outputs:     PCM samples in outBuf, for input to subband transform
 *              PCM samples in overBuf, for OLA next time
 *              updated hi->nonZeroBound index for this channel
 *
 * Return:      0 on success,  -1 if null input pointers
 **************************************************************************************/
int IMDCT(MP3DecInfo *mp3DecInfo, int gr, int ch)
{
	SideInfo *si;
	IppMP3SideInfo *siIPP;
	HuffmanInfo *hi;
	IMDCTInfo *mi;
	IppStatus ippStatus = ippStsNoErr;

	/* validate pointers */
	if (!mp3DecInfo || !mp3DecInfo->SideInfoPS || !mp3DecInfo->HuffmanInfoPS || !mp3DecInfo->IMDCTInfoPS)
		return -1;

	/* si is an array of up to 4 structs, stored as gr0ch0, gr0ch1, gr1ch0, gr1ch1 */
	si = (SideInfo *)(mp3DecInfo->SideInfoPS);
	siIPP = &(si->siIPP[0]);
	siIPP += (gr*mp3DecInfo->nChans + ch);

	hi = (HuffmanInfo*)(mp3DecInfo->HuffmanInfoPS);
	mi = (IMDCTInfo *)(mp3DecInfo->IMDCTInfoPS);
	
	/* IPP primitive #6 - alias reduction, inverse MDCT, overlap-add, frequency inversion */
	ippStatus = ippsMDCTInv_MP3_32s(hi->huffDecBuf[ch], mi->outBuf[ch], mi->overBuf[ch], hi->nonZeroBound[ch], 
		&(mi->numPrevIMDCT[ch]), siIPP->blockType, siIPP->mixedBlock);

	if (ippStatus != ippStsNoErr)
		return -1;
	else
		return 0;
}

