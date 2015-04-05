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
 * huffman.c - wrapper for IPP primitive which decodes Huffman codewords
 **************************************************************************************/

#include "coder.h"	

/**************************************************************************************
 * Function:    DecodeHuffman
 *
 * Description: decode one granule, one channel worth of Huffman codes
 *
 * Inputs:      MP3DecInfo structure filled by UnpackFrameHeader(), UnpackSideInfo(),
 *                and UnpackScaleFactors() (for this granule)
 *              buffer pointing to start of Huffman data in MP3 frame
 *              pointer to bit offset (0-7) indicating starting bit in buf[0]
 *              number of bits in the Huffman data section of the frame
 *                (could include padding bits)
 *              index of current granule and channel
 *
 * Outputs:     decoded coefficients in hi->huffDecBuf[ch] (hi pointer in mp3DecInfo)
 *              updated bitOffset
 *
 * Return:      length (in bytes) of frame header (for caller to calculate offset to
 *                first byte following frame header)
 *              bitOffset also returned in parameter (0 = MSB, 7 = LSB of 
 *                byte located at buf + offset)
 *              -1 if null input pointers
 **************************************************************************************/

#ifdef HELIX_FEATURE_USE_IPP4
/* MPEG-2 modified long block SFB table */
IppMP3ScaleFactorBandTableLong Modified_Sfb_Table_Long =
{
	/* MPEG-2
	 * 11.025 kHz
	 */
	0,	 6,	  12,  18,	24,	 30,  36,  44,	54,	 66,  80,	96,
	116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576,

	/* MPEG-2
	 * 12 kHz
	 */
    0,   6,	  12,  18,	24,  30,  36,  44,  54,  66,  80,   96,		
    116, 140, 168, 200,	238, 284, 336, 396,	464, 522, 576,

	/* MPEG-2
	 * 8 kHz
	 */
    0,	 12,  24,  36,  48,	 60,  72,  88,  108, 132, 160, 192,
    232, 280, 336, 400, 476, 566, 568, 570, 572, 574, 576,

	/* MPEG-1
	 * 44.1 kHz
	 */
    0,   4,	  8,   12,	16,  20,  24,  30,	36,  44,  52,  62,		
	74,  90,  110, 134,	162, 196, 238, 288,	342, 418, 576,

	/* MPEG-1
	 * 48 kHz
	 */
    0,	 4,   8,   12,  16,	 20,  24,  30,  36,	 42,  50,  60,
	72,	 88,  106, 128, 156, 190, 230, 276, 330, 384, 576,

	/* MPEG-1
	 * 32 kHz
	 */
    0,   4,	  8,   12,  16,  20,  24,  30,  36,  44,  54,  66,		
	82,  102, 126, 156,	194, 240, 296, 364,	448, 550, 576 
};

/* MPEG-2 modified short block SFB table */
IppMP3ScaleFactorBandTableShort Modified_Sfb_Table_Short = 
{
	/* MPEG-2
	 * 11.025 kHz
	 */
    0,	 4,   8,   12,  18,	 26,  36,		
    48,  62,  80,  104,	134, 174, 192,

	/* MPEG-2
	 * 12 kHz
	 */
    0,	 4,   8,   12,  18,  26,  36,		
    48,	 62,  80,  104,	134, 174, 192,

	/* MPEG-2
	 * 8 kHz
	 */
 	0,	 8,   16,  24,  36,	 52,  72,		
    96,  124, 160, 162,	164, 166, 192,

	/* MPEG-1
	 * 44.1 kHz
	 */
    0,	 4,   8,   12,  16,  22,  30,  
	40,  52,  66,  84,	106, 136, 192,

	/* MPEG-1
	 * 48 kHz
	 */
    0,	 4,   8,   12,  16,	 22,  28,		
	38,  50,  64,  80,	100, 126, 192,

	/* MPEG-1
	 * 32 kHz
	 */
    0,	 4,   8,   12,  16,	 22,  30,		
	42,  58,  78,  104, 138, 180, 192
};

/* Example mixed block partition table

	For mixed blocks only, this partition table informs 
	the Huffman decoder of how many SFBs to count for region0.
*/

IppMP3MixedBlockPartitionTable Modified_Mbp_Table = 
{
	/* MPEG-2
	 * 22.050 kHz
	 * Long block SFBs		Short block SFBs
	 */
		6,					2,

	/* MPEG-2
	 * 24 kHz
	 * Long block SFBs		Short block SFBs
	 */
		6,					2,		

	/* MPEG-2
	 * 16 kHz
	 * Long block SFBs		Short block SFBs
	 */
		6,					2,		

	/* MPEG-1
	 * 44.1 kHz
	 * Long block SFBs		Short block SFBs
	 */
		8,					0,

	/* MPEG-1
	 * 48 kHz
	 * Long block SFBs		Short block SFBs
	 */
		8,					0,

	/* MPEG-1
	 * 32 kHz
	 * Long block SFBs		Short block SFBs
	 */
		8,					0
};

#endif /* #if defined(HELIX_FEATURE_USE_IPP4) */


int DecodeHuffman(MP3DecInfo *mp3DecInfo, unsigned char *buf, int *bitOffset, int huffBlockBits, int gr, int ch)
{
	Ipp8u *pBitStream;
	FrameHeader *fh;
	IppMP3FrameHeader *fhIPP;
	SideInfo *si;
	IppMP3SideInfo *siIPP;
	HuffmanInfo *hi;
	IppStatus ippStatus = ippStsNoErr;

	/* validate pointers */
	if (!mp3DecInfo || !mp3DecInfo->FrameHeaderPS || !mp3DecInfo->SideInfoPS || !mp3DecInfo->HuffmanInfoPS)
		return -1;

	pBitStream = (Ipp8u *)buf;
	fh = (FrameHeader *)(mp3DecInfo->FrameHeaderPS);
	fhIPP = &(fh->fhIPP);

	/* si is an array of up to 4 structs, stored as gr0ch0, gr0ch1, gr1ch0, gr1ch1 */
	si = (SideInfo *)(mp3DecInfo->SideInfoPS);
	siIPP = &(si->siIPP[0]);
	siIPP += (gr*mp3DecInfo->nChans + ch);

	hi = (HuffmanInfo*)(mp3DecInfo->HuffmanInfoPS);
#if defined(HELIX_FEATURE_USE_IPP3)
	/* IPP3.0 primitive #4 - Huffman decoding */
	ippStatus = ippsHuffmanDecode_MP3_1u32s(&pBitStream, bitOffset, hi->huffDecBuf[ch], &(hi->nonZeroBound[ch]), 
		siIPP, fhIPP, huffBlockBits);
#else 
	/*
	 * IPP4.0 primitive #4 - Huffman decoding
	 * Bug fix prototype for
	 *	1. MPEG2.5 short block region0count
	 *	2. MPEG2.5 mixed block region0count
	 *	3. MPEG-2 mixed block region0count
	 */	
	if (mp3DecInfo->version == MPEG25)
		ippStatus = ippsHuffmanDecodeSfbMbp_MP3_1u32s( &pBitStream,	bitOffset, hi->huffDecBuf[ch],
				&(hi->nonZeroBound[ch]), siIPP,	fhIPP,	huffBlockBits, Modified_Sfb_Table_Long,
				Modified_Sfb_Table_Short, Modified_Mbp_Table);
	else
		ippStatus = ippsHuffmanDecodeSfbMbp_MP3_1u32s( &pBitStream, bitOffset, hi->huffDecBuf[ch],
				&(hi->nonZeroBound[ch]), siIPP,	fhIPP, huffBlockBits, _ipp_pMP3SfbTableLong,
				_ipp_pMP3SfbTableShort,	Modified_Mbp_Table);
#endif /* HELIX_FEATURE_USE_IPP3 */
	/* IPP primitive updated pBitStream, return the offset to start of next block in bitstream */

	if(ippStatus != ippStsNoErr)
		return -1;
	else
		return (pBitStream - buf);
}

