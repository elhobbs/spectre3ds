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
 * bitstream.c - wrapper functions for IPP primitives which unpack frame header, 
 *                 side info, and scale factors
 **************************************************************************************/

#include "coder.h"	

/**************************************************************************************
 * Function:    UnpackFrameHeader
 *
 * Description: parse the fields of the MP3 frame header
 *
 * Inputs:      buffer pointing to a complete MP3 frame header (4 bytes, plus 2 if CRC)
 *
 * Outputs:     filled frame header info in the MP3DecInfo structure
 *              updated platform-specific FrameHeader struct
 *
 * Return:      length (in bytes) of frame header (for caller to calculate offset to
 *                first byte following frame header)
 *              -1 if null frameHeader or invalid syncword
 *
 * TODO:        check for valid fields, depending on capabilities of decoder
 *              test CRC on actual stream (verify no endian problems)
 **************************************************************************************/
int UnpackFrameHeader(MP3DecInfo *mp3DecInfo, unsigned char *buf)
{
	int verTabIdx, mpeg25Flag;
	Ipp8u *pBitStream;
	FrameHeader *fh;
	IppMP3FrameHeader *fhIPP;

	IppStatus ippRes = ippStsNoErr;
	
	/* validate pointers and sync word */
	if (!mp3DecInfo || !mp3DecInfo->FrameHeaderPS || (buf[0] & SYNCWORDH) != SYNCWORDH || (buf[1] & SYNCWORDL) != SYNCWORDL)
		return -1;

	/* IPP primitive #1 - unpack frame header */
	pBitStream = (Ipp8u *)buf;
	fh = ((FrameHeader *)(mp3DecInfo->FrameHeaderPS));
	fhIPP = &(fh->fhIPP);
	ippRes = ippsUnpackFrameHeader_MP3(&pBitStream, fhIPP);
	if (ippRes != ippStsNoErr)
		return -1;

	/* init user-accessible data */
	mpeg25Flag = 1 - ((buf[1] >> 4) & 0x01);	/* bit 20: 0 = MPEG25, 1 = MPEG1 or MPEG2 (dep. on bit 19) */
	if (fhIPP->id == 0x01 && !mpeg25Flag)
		verTabIdx = 0;		/* MPEG 1 */
	else if (fhIPP->id == 0x00 && !mpeg25Flag)
		verTabIdx = 1;		/* MPEG 2 */
	else
		verTabIdx = 2;		/* MPEG 2.5 */

#if defined(HELIX_FEATURE_USE_IPP3)
	/* currently MPEG2.5 is not supported with IPP
	 * should be possible using "*Sfb" versions of Huffman decoder and dequantizer primitives
	 *   (pass in user-defined scalefactor band tables for MPEG2.5 critical bands)
	 * possible problem is that Huffman decoder only lets user override long-block table
	 * see IPP docs for more info
	 */
	if (verTabIdx == 2)
		return -1;
#endif /* HELIX_FEATURE_USE_IPP3 */

	/*
	 * IPP4.0 has supported MPEG2.5
	 */
	if ( (fhIPP->bitRate<0) || (fhIPP->bitRate>=15) || (fhIPP->samplingFreq<0) || 
		     (fhIPP->samplingFreq>2) ||	(fhIPP->layer != 1 ))
		return -1;	
	
	/* fhIPP->layer: 3 for layer1, 2 for layer2, 1 for layer3 */
	mp3DecInfo->bitrate = ((int)bitrateTab[verTabIdx][4 - fhIPP->layer - 1][fhIPP->bitRate]) * 1000;
	mp3DecInfo->nChans = (fhIPP->mode == 0x03 ? 1 : 2);
	mp3DecInfo->samprate = samplerateTab[verTabIdx][fhIPP->samplingFreq];
	mp3DecInfo->nGrans = (fhIPP->id == 1 ? NGRANS_MPEG1 : NGRANS_MPEG2);
	mp3DecInfo->nGranSamps = ((int)samplesPerFrameTab[verTabIdx][4 - fhIPP->layer - 1]) / mp3DecInfo->nGrans;
	mp3DecInfo->layer = (4 - fhIPP->layer);
	mp3DecInfo->version = (MPEGVersion)verTabIdx;

	/* nSlots = total frame bytes (from table) - sideInfo bytes - header - CRC (if present) + pad (if present) */
	mp3DecInfo->nSlots = (int)slotTab[verTabIdx][fhIPP->samplingFreq][fhIPP->bitRate] - 
		(int)sideBytesTab[verTabIdx][(fhIPP->mode == 0x03 ? 0 : 1)] - 
		4 - (fhIPP->protectionBit ? 0 : 2) + (fhIPP->paddingBit ? 1 : 0);

	/* IPP primitive updated pBitStream to point to the byte immediately following the 
	 *   frame header, so we use the difference to determine length of frame header
	 */
	return (pBitStream - buf);
}

/**************************************************************************************
 * Function:    UnpackSideInfo
 *
 * Description: parse the fields of the MP3 side info header
 *
 * Inputs:      MP3DecInfo structure filled by UnpackFrameHeader()
 *              buffer pointing to the MP3 side info data
 *
 * Outputs:     updated mainDataBegin in MP3DecInfo struct
 *              updated private (platform-specific) SideInfo struct
 *
 * Return:      length (in bytes) of side info data
 *              -1 if null input pointers
 **************************************************************************************/
int UnpackSideInfo(MP3DecInfo *mp3DecInfo, unsigned char *buf)
{
	Ipp8u *pBitStream;
	FrameHeader *fh;
	IppMP3FrameHeader *fhIPP;
	SideInfo *si;
	IppMP3SideInfo *siIPP;
	IppStatus ippRes = ippStsNoErr;

	/* validate pointers */
	if (!mp3DecInfo || !mp3DecInfo->FrameHeaderPS || !mp3DecInfo->SideInfoPS)
		return -1;

	pBitStream = (Ipp8u *)buf;
	fh = (FrameHeader *)(mp3DecInfo->FrameHeaderPS);
	fhIPP = &(fh->fhIPP);
	si = (SideInfo *)(mp3DecInfo->SideInfoPS);
	siIPP = &(si->siIPP[0]);

	/* IPP primitive #2 - unpack side info */
	ippRes = ippsUnpackSideInfo_MP3(&pBitStream, siIPP, &(mp3DecInfo->mainDataBegin), &(si->privateBits), 
		&(si->scfsi[0][0]), fhIPP);
	if (ippRes != ippStsNoErr)
		return -1;
	
	/* IPP primitive updated pBitStream, return the offset to start of next block in bitstream */
	return (pBitStream - buf);
}

/**************************************************************************************
 * Function:    UnpackScaleFactors
 *
 * Description: parse the fields of the MP3 scale factor data section
 *
 * Inputs:      MP3DecInfo structure filled by UnpackFrameHeader() and UnpackSideInfo()
 *              buffer pointing to the MP3 scale factor data
 *              bit offset (0-7)
 *              number of bits available in buf
 *              index of current granule and channel
 *
 * Outputs:     updated platform-specific ScaleFactorInfo struct
 *
 * Return:      length (in bytes) of scale factor data
 *              bitOffset also returned in parameter (0 = MSB, 7 = LSB of 
 *                byte located at buf + offset)
 *              -1 if null input pointers
 **************************************************************************************/
int UnpackScaleFactors(MP3DecInfo *mp3DecInfo, unsigned char *buf, int *bitOffset, int bitsAvail, int gr, int ch)
{
	Ipp8u *pBitStream;
	FrameHeader *fh;
	IppMP3FrameHeader *fhIPP;
	SideInfo *si;
	IppMP3SideInfo *siIPP;
	ScaleFactorInfo *sfi;
	IppStatus ippRes = ippStsNoErr;

	/* validate pointers */
	if (!mp3DecInfo || !mp3DecInfo->FrameHeaderPS || !mp3DecInfo->SideInfoPS || !mp3DecInfo->ScaleFactorInfoPS)
		return -1;

	pBitStream = (Ipp8u *)buf;
	fh = (FrameHeader *)(mp3DecInfo->FrameHeaderPS);
	fhIPP = &(fh->fhIPP);

	/* si is an array of up to 4 structs, stored as gr0ch0, gr0ch1, gr1ch0, gr1ch1 */
	si = (SideInfo *)(mp3DecInfo->SideInfoPS);
	siIPP = &(si->siIPP[0]);
	siIPP += (gr*mp3DecInfo->nChans + ch);

	sfi = (ScaleFactorInfo *)mp3DecInfo->ScaleFactorInfoPS;

	/* IPP primitive #3 - unpack scale factors */
	ippRes = ippsUnpackScaleFactors_MP3_1u8s(&pBitStream, bitOffset, sfi->scaleFactor[ch],
		siIPP, si->scfsi[ch], fhIPP, gr, ch);
	if (ippRes != ippStsNoErr)
		return -1;
	mp3DecInfo->part23Length[gr][ch] = siIPP->part23Len;

	/* IPP primitive updated pBitStream, return the offset to start of next block in bitstream */
	return (pBitStream - buf);
}

/**************************************************************************************
 * Function:    CheckPadBit
 *
 * Description: indicate whether or not padding bit is set in frame header
 *
 * Inputs:      MP3DecInfo structure filled by UnpackFrameHeader()
 *
 * Outputs:     none
 *
 * Return:      1 if pad bit is set, 0 if not, -1 if null input pointer
 **************************************************************************************/
int CheckPadBit(MP3DecInfo *mp3DecInfo)
{
	FrameHeader *fh;
	IppMP3FrameHeader *fhIPP;

	/* validate pointers */
	if (!mp3DecInfo || !mp3DecInfo->FrameHeaderPS)
		return -1;

	fh = ((FrameHeader *)(mp3DecInfo->FrameHeaderPS));
	fhIPP = &(fh->fhIPP);

	return (fhIPP->paddingBit ? 1 : 0);
}

