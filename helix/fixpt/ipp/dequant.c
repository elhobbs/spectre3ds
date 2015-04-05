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
 * dequant.c - wrapper for IPP primitive which reconstructs the dequantized spectrum
 *               from scale factors and decoded Huffman codewords
 **************************************************************************************/

#include "coder.h"	

/**************************************************************************************
 * Function:    Dequantize
 *
 * Description: dequantize coefficients, decode stereo, reorder short blocks
 *                (one granule-worth)
 *
 * Inputs:      MP3DecInfo structure filled by UnpackFrameHeader(), UnpackSideInfo(),
 *                UnpackScaleFactors(), and DecodeHuffman() (for this granule)
 *              index of current granule
 *
 * Outputs:     dequantized and reordered coefficients in hi->huffDecBuf[0] 
 *                (one granule-worth, all channels)
 *              operates in-place on huffDecBuf but also needs di->workBuf
 *              updated hi->nonZeroBound index for both channels
 *
 * Return:      0 on success,  -1 if null input pointers
 **************************************************************************************/

int Dequantize(MP3DecInfo *mp3DecInfo, int gr)
{
	FrameHeader *fh;
	IppMP3FrameHeader *fhIPP;
	SideInfo *si;
	IppMP3SideInfo *siIPP;
	ScaleFactorInfo *sfi;
	HuffmanInfo *hi;
	DequantInfo *di;
	IppStatus ippStatus = ippStsNoErr;

	/* validate pointers */
	if (!mp3DecInfo || !mp3DecInfo->FrameHeaderPS || !mp3DecInfo->SideInfoPS || !mp3DecInfo->ScaleFactorInfoPS || 
		!mp3DecInfo->HuffmanInfoPS || !mp3DecInfo->DequantInfoPS)
		return -1;

	fh = (FrameHeader *)(mp3DecInfo->FrameHeaderPS);
	fhIPP = &(fh->fhIPP);

	/* si is an array of up to 4 structs, stored as gr0ch0, gr0ch1, gr1ch0, gr1ch1 */
	si = (SideInfo *)(mp3DecInfo->SideInfoPS);
	siIPP = &(si->siIPP[0]);
	siIPP += (gr*mp3DecInfo->nChans);		/* does both channels at a time, so just offset to ch[0] of correct granule */

	sfi = (ScaleFactorInfo *)mp3DecInfo->ScaleFactorInfoPS;
	hi = (HuffmanInfo *)mp3DecInfo->HuffmanInfoPS;
	di = (DequantInfo *)mp3DecInfo->DequantInfoPS;
#if defined(HELIX_FEATURE_USE_IPP3)
	/* IPP primitive #5 - dequantize, stereo decode, short block reorder */
	ippStatus = ippsReQuantize_MP3_32s_I(hi->huffDecBuf[0], hi->nonZeroBound, sfi->scaleFactor[0], siIPP, fhIPP, di->workBuf);
#else
	if(mp3DecInfo->version == MPEG25)
		ippStatus = ippsReQuantizeSfb_MP3_32s_I( hi->huffDecBuf[0],	hi->nonZeroBound, sfi->scaleFactor[0],
				siIPP, fhIPP, di->workBuf, Modified_Sfb_Table_Long,	Modified_Sfb_Table_Short);
	else
		ippStatus = ippsReQuantize_MP3_32s_I( hi->huffDecBuf[0], hi->nonZeroBound, sfi->scaleFactor[0], 
				siIPP, fhIPP, di->workBuf);
#endif /* HELIX_FEATURE_USE_IPP4 */

	if(ippStatus != ippStsNoErr)
		return -1;
	else
		return 0;
}
