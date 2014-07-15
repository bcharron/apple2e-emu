/*
 * Disk.cc - Disk controller for the Apple ][e emulator
 * Copyright (C) 2011 Benjamin Charron <bcharron@pobox.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Disk.cc - Benjamin Charron <bcharron@pobox.com>
 * Created  : Fri Dec 23 09:24:59 2011
 * Revision : $Id$
 */

#include <assert.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "Disk.h"

using namespace std;

uint8_t XLAT62[64] = {                // Translation table for 6-and-2 encoding
	0x96, 0x97, 0x9A, 0x9B, 0x9D, 0x9E, 0x9F, 0xA6, 
	0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB2, 0xB3,
	0xB4, 0xB5, 0xB6, 0xB7, 0xB9, 0xBA, 0xBB, 0xBC,
	0xBD, 0xBE, 0xBF, 0xCB,	0xCD, 0xCE, 0xCF, 0xD3,
	0xD6, 0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE,
	0xDF, 0xE5, 0xE6, 0xE7, 0xE9, 0xEA, 0xEB, 0xEC,
	0xED, 0xEE, 0xEF, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6,
	0xF7, 0xF9, 0xFA, 0xFB,	0xFC, 0xFD, 0xFE, 0xFF };

Disk::Disk(void)
	: diskImageFilename(""),
	  diskImageData(NULL),
	  diskImageOpened(false),
	  currentTrack(0),
	  currentSector(0),
	  currentSectorPosition(0),
	  shaftPosition(0)
{
	for (int x = 0; x < DISK_NB_PHASES; x++)
		phases[x] = false;
}

bool
Disk::init(void)
{
	return(true);
}

/* Reset the disk controller */
void
Disk::reset(void)
{
	currentTrack = 0;
	currentSector = 0;
	currentSectorPosition = 0;
}

bool
Disk::openFile(std::string filename)
{
	bool success = false;

	// Clear previous file data before reading another
	if (diskImageData) {
		delete[] diskImageData;
		diskImageData = NULL;
	}

	diskImageData = new uint8_t[DISK_IMAGE_LEN];
	diskImageFilename = filename;

	ifstream file(diskImageFilename.c_str(), ios::in | ios::binary);

	if (file.is_open()) {
		// XXX: Check to make sure the disk is the appropriate size
		file.read((char *) diskImageData, DISK_IMAGE_LEN);
		// XXX: Make sure the read succeeded.
		file.close();
		success = true;
		diskImageOpened = true;
		cout << "Loaded disk " << diskImageFilename << " OK." << endl;

		printf("Building track %d, sector %d\n", currentTrack, currentSector);
		buildSector(currentSector, sectorRawData);
	} else {
		cerr << "Unable to open " << filename << endl;
		delete[] diskImageData;
		diskImageData = NULL;
	}

	return(success);
}

void
Disk::closeFile(void)
{
	diskImageFilename = "";
}

void
Disk::motorOn(void)
{
}

void
Disk::motorOff(void)
{
}

void
Disk::writeByte(unsigned char byte)
{
}

/*
 *           (active)
 *             [1]
 *         \
 *           \
 *   [0]       \         [2]
 *  (active)
 *
 *
 *             [3]
 *        (doesn't matter)
 *
 * Returns true if the shaft change position
 */
bool
Disk::updateShaftPosition(void)
{
	// XXX: This should be called more often than every X cycles
	// because the shaft rotation would be instantaenous. For the
	// purpose at hand, however, it probably doesn't matter
	// though: I don't see why existing Apple2 software would try
	// to carry fake operations..
	int8_t leftMagnet;
	int8_t rightMagnet;

	previousShaftPosition = shaftPosition;

	/*
	 * Shaft positions:
	 *
	 * Even numbers are positions aligned with a magnet:
	 * 0: Aligned with magnet 0
	 * 2: Aligned with magnet 1
	 * 4: Aligned with magnet 2
	 * 6: Aligned with magnet 3
	 *
	 * Odd numbers are positions aligned between magnets:
	 * 1: Between magnets 0 and 1
	 * 3: Between magnets 1 and 2
	 * 5: Between magnets 2 and 3
	 * 7: Between magnets 3 and 0
	 */

	/* Shaft currently aligned on a magnet */
	if (shaftPosition % 2 == 0) {
		leftMagnet = (shaftPosition - 2) / 2;
		if (leftMagnet < 0)
			leftMagnet = 3;

		rightMagnet = (shaftPosition + 2) / 2;
		rightMagnet %= 4;

		/* Currently aligned magnet is ON */
		if (phases[shaftPosition / 2]) {
			if (phases[leftMagnet] && phases[rightMagnet]) {
				// no change in position
			} else if (phases[leftMagnet] && !phases[rightMagnet])
				shaftPosition = (leftMagnet * 2) + 1;
			else if (!phases[leftMagnet] && phases[rightMagnet])
				shaftPosition = (rightMagnet * 2) - 1;
		} else {
			/* Currently aligned magnet is OFF */
			if (phases[leftMagnet] && phases[rightMagnet])
				shaftPosition = (leftMagnet * 2) + 1;
			else if (phases[leftMagnet] && !phases[rightMagnet])
				shaftPosition = leftMagnet * 2;
			else if (!phases[leftMagnet] && phases[rightMagnet])
				shaftPosition = rightMagnet * 2;
		}
	} else {
		/* Shaft in between two magnets */
		leftMagnet = (shaftPosition - 1) / 2;
		rightMagnet = (shaftPosition + 1) / 2;
		rightMagnet %= 4;

		if (phases[leftMagnet] && phases[rightMagnet])
			shaftPosition = (leftMagnet * 2) + 1;
		else if (phases[leftMagnet] && !phases[rightMagnet])
			shaftPosition = leftMagnet * 2;
		else if (!phases[leftMagnet] && phases[rightMagnet])
			shaftPosition = rightMagnet * 2;
	}

	shaftPosition %= 8;
       
	bool ret = false;

	if (shaftPosition != previousShaftPosition) {
		printf("Shaft position: %d -> %d\n", previousShaftPosition, shaftPosition);
		ret = true;
	}

	return(ret);
}

/* 
 * updateHeadTrack(): Change the head's track depending on the shaft rotation
 *
 * Depending on the shaft position, the read head may have changed tracks
 */
bool
Disk::updateHeadTrack(void)
{
	int previousTrack = currentTrack;

	if (shaftPosition < previousShaftPosition) {
		if (shaftPosition == 0 && previousShaftPosition >= 6) {
			// Shaft turned clockwise and wrapped around
			currentTrack++;
		}
		else if (shaftPosition % 2 == 0) {
			// Shaft turned counter-clockwise one quarter-turn
			if (currentTrack > 0)
				currentTrack--;
		}
	} else if (shaftPosition > previousShaftPosition) {
		if (shaftPosition >= 6 && previousShaftPosition == 0) {
			// Shaft turned counter-clockwise and wrapped around
			if (currentTrack > 0)
				currentTrack--;
		} else if (shaftPosition % 2 == 0) {
			// Shaft turned clockwise one quarter-turn
			currentTrack++;
		}
	}

	// Since sector is unsigned, -1 == 255 which is greater than
	// DISK_MAX_TRACK
	if (currentTrack > DISK_MAX_TRACK)
		currentTrack = DISK_MAX_TRACK;

	bool ret = false;

	if (previousTrack != currentTrack) {
		printf("Changed track %d -> %d\n", previousTrack, currentTrack);
		ret = true;
	}

	return(ret);
}

/*
 * Extract from rwts.d1.format.txt:
 * - The stepper motor can be envisioned as containing a central  *
 *   magnet on a rotatable shaft and a circle of four fixed       *
 *   magnets (magnets 0 to 3) surrounding the shaft.  Each time a *
 *   peripheral magnet is enegized, the central shaft is rotated  *
 *   until its magnet is in line with the energized peripheral    *
 *   magnet.  By turning the fixed peripheral magnets on and off  *
 *   in sequence, we can spin the shaft of the stepper motor.     *
 *   Movement of this shaft causes the read/write head to "step"  *
 *   across the disk.  Each time the next magnet in sequence is   *
 *   turned on, the shaft is rotated one quarter turn.  One       *
 *   quarter turn of the shaft moves the read/write head half a   *
 *   track width.
 */
void
Disk::phaseOn(uint8_t phaseNumber)
{
	changePhase(phaseNumber, true);
}

void
Disk::phaseOff(uint8_t phaseNumber)
{
	changePhase(phaseNumber, false);
}

/* Turns phase magnet on or off depending on 'value' */
void
Disk::changePhase(uint8_t phaseNumber, bool value)
{
	assert(phaseNumber < DISK_NB_PHASES);

	printf("phase %d: %d -> %d\n", phaseNumber, phases[phaseNumber], value);
	phases[phaseNumber] = value;

	if (updateShaftPosition())
		updateHeadTrack();
}

uint8_t addressPrologue[] = { 0xD5, 0xAA, 0x96 };
uint8_t dataPrologue[] = { 0xD5, 0xAA, 0xAD };
uint8_t epilogue[] = { 0xDE, 0xAA, 0xEB };

/*
 * Returns the next byte from the disk
 */
uint8_t
Disk::readNextByte(void)
{
	// Depends on the track and track position. Returns MSB=1 when valid byte is ready to be read.
	int8_t byte;

	printf("READ[trk:%d sec:%d  pos:%d]: ", currentTrack, currentSector, currentSectorPosition);

	if (diskImageOpened) {
		byte = sectorRawData[currentSectorPosition];
		currentSectorPosition++;
	} else {
		printf("Disk image not opened. Returning garbage.\n");
		byte = 0xff;
	}

	printf("0x%02X\n", (unsigned char) byte);

	if (currentSectorPosition >= DISK_RAW_SECTOR_LEN) {
		currentSectorPosition = 0;
		currentSector++;
		currentSector %= DISK_SECTORS_PER_TRACK;
		
		printf("Building track %d, sector %d\n", currentTrack, currentSector);
		buildSector(currentSector, sectorRawData);
	}

	return(byte);
}

#define BUFF_ODD 0
#define BUFF_EVEN 1

void oddEvenEncode(uint8_t byte, uint8_t *buf)
{
	uint8_t odd, even;

	odd = ((byte & 0xAA) >> 1) | 0xAA;
	even = (byte & 0x55) | 0xAA;

	buf[BUFF_ODD] = odd;
	buf[BUFF_EVEN] = even;
}

/* Encode 256 bytes from 'in' into 342 bytes in 'out' */
void encode5And3(uint8_t *in, uint8_t *out)
{
	int srcIdx = 0;
	uint8_t smallBuf[3];
	uint8_t bufA[51];
	uint8_t bufB[51];
	uint8_t bufC[51];
	uint8_t bufD[51];
	//uint8_t bufE[51];

	for (int x = 51; x > 0; x++) {
		uint8_t byte = in[srcIdx++];
		smallBuf[0] = byte;
		bufA[x] = byte >> 3;
		
		byte = in[srcIdx++];
		smallBuf[1] = byte;
		bufB[x] = byte >> 3;

		byte = in[srcIdx++];
		smallBuf[2] = byte;
		bufC[x] = byte >> 3;

		byte = in[srcIdx++];
		smallBuf[2] = (smallBuf[2] << 1) | (byte & 0x01);
		byte >>= 1;
		smallBuf[1] = (smallBuf[1] << 1) | (byte & 0x01);
		byte >>= 1;
		smallBuf[0] = (smallBuf[0] << 1) | (byte & 0x01);
		byte >>= 1;
		bufD[x] = byte;

		byte = in[srcIdx++];
		smallBuf[2] = (smallBuf[2] << 1) | (byte & 0x01);
		byte >>= 1;
		smallBuf[1] = (smallBuf[1] << 1) | (byte & 0x01);
		byte >>= 1;
		smallBuf[0] = (smallBuf[0] << 1) | (byte & 0x01);
		byte >>= 1;
		bufD[x] = byte;

		smallBuf[0] = smallBuf[0] & 0x1F;
		smallBuf[1] = smallBuf[1] & 0x1F;
		smallBuf[2] = smallBuf[2] & 0x1F;

		// XXX: Incomplete
	}
}

/*
 * Prenibble data ($B800-$B829).
 * Convert 256 memory bytes into 342
 * six-bit nibbles as shown below:
 *              RWTSBUF1
 * BB00: 0 0 00-7 00-6 00-5 00-5 00-3 00-2
 * BB01: 0 0 01-7 01-6 01-5 01-4 01-3 01-2
 * BB02: 0 0 02-7 02-6 02-5 02-4 02-3 02-2
 *  .    . .  .    .    .    .    .    .
 *  .    . .  .    .    .    .    .    .
 *  .    . .  .    .    .    .    .    .
 * BBFF: 0 0 FF-7 FF-6 FF-5 FF-4 FF-3 FF-2
 *
 *              RWTSBUF2
 * BC00: 0 0 01-0 01-1 AB-0 AB-1 55-0 55-1
 * BB01: 0 0 00-0 00-1 AA-0 AA-1 54-0 54-1
 * BB02: 0 0 FF-0 FF-1 A9-0 A9-1 53-0 53-1
 *  .    . .  .    .    .    .    .    .
 *  .    . .  .    .    .    .    .    .
 *  .    . .  .    .    .    .    .    .
 * BC54: 0 0 AD-0 AD-1 57-0 57-1 01-0 01-1
 * BC55: 0 0 AC-0 AC-1 56-0 56-1 00-0 00-1
 *
 * Where AC-0 = bit 0 of memory byte which
 *              is offset #$AC bytes into
 *              the data sector.
 * The following bits are duplicated in
 * $BC00 - $BC01 & $BC54 - $BC55 but are
 * ignored in $BC00 - $BC01:
 *        01-0, 01-1, 00-0, 00-1.

 Take a 256 block and encode it 6-and-2 into 342 bytes
 */
void encode6And2(uint8_t *in, uint8_t *out)
{
	int idx = 0;

	for (int x = 0; x < DISK_USERDATA_LEN; x++)
		out[x] = 0x00;

	for (int x = 0; x <= 0x55; x++) {
		uint8_t byte = in[idx];
		out[idx] = byte >> 2;

		// Swap bits 0 and 1
		uint8_t bit1 = (byte & 0x02) >> 1;
		uint8_t bit0 = byte & 0x01;
		uint8_t swap = (bit0 << 1) | bit1;
		
		out[DISK_USERDATA_LEN - x] = swap;
		idx++;
	}

	for (int x = 0; x <= 0x55; x++) {
		uint8_t byte = in[idx];
		out[idx] = byte >> 2;

		// Swap bits 0 and 1
		uint8_t bit1 = (byte & 0x02) >> 1;
		uint8_t bit0 = byte & 0x01;
		uint8_t swap = (bit0 << 1) | bit1;

		uint8_t byte2 = out[DISK_USERDATA_LEN - x];
		byte2 = (swap << 2) | byte2;
		out[DISK_USERDATA_LEN - x] = byte2;

		idx++;
	}

	for (int x = 0; x <= 0x53; x++) {
		uint8_t byte = in[idx];
		out[idx] = byte >> 2;

		// Swap bits 0 and 1
		uint8_t bit1 = (byte & 0x02) >> 1;
		uint8_t bit0 = byte & 0x01;
		uint8_t swap = (bit0 << 1) | bit1;

		uint8_t byte2 = out[DISK_USERDATA_LEN - x];
		byte2 = (swap << 4) | byte2;
		out[DISK_USERDATA_LEN - x] = byte2;

		idx++;
	}

	while (idx < 0xff) {
		uint8_t byte = in[idx];
		out[idx] = byte >> 2;
		idx++;
	}
	
	// Last two bit-pairs at 0xFF + 1 and 0xFF + 2
	{
		uint8_t byte = in[1];

		// Swap bits 0 and 1
		uint8_t bit1 = (byte & 0x02) >> 1;
		uint8_t bit0 = byte & 0x01;
		uint8_t swap = (bit0 << 1) | bit1;

		uint8_t byte2 = out[256];
		out[256] = (swap << 4) | byte2;


		byte = in[0];

		// Swap bits 0 and 1
		bit1 = (byte & 0x02) >> 1;
		bit0 = byte & 0x01;
		swap = (bit0 << 1) | bit1;

		byte2 = out[257];
		out[257] = (swap << 4) | byte2;
	}
}

bool
Disk::buildSector(uint8_t sectorNumber, uint8_t *out)
{
	uint8_t *data = out;
	uint16_t idx = 0;
	uint8_t encodeBuffer[2];
	uint8_t nibblizedSectorData[342];

	// Gap 1
	for (int x = 0; x < DISK_GAP1_LEN; x++)
		data[idx++] = DISK_SYNC_BYTE;

	/***** ADDRESS FIELD *****/
	// Prologue
	for (int x = 0; x < DISK_PROLOGUE_LEN; x++)
		data[idx++] = addressPrologue[x];

	// Volume number
	oddEvenEncode(currentVolume, encodeBuffer);
	data[idx++] = encodeBuffer[BUFF_ODD];
	data[idx++] = encodeBuffer[BUFF_EVEN];

	// Track
	oddEvenEncode(currentTrack, encodeBuffer);
	data[idx++] = encodeBuffer[BUFF_ODD];
	data[idx++] = encodeBuffer[BUFF_EVEN];

	// Sector
	oddEvenEncode(currentSector, encodeBuffer);
	data[idx++] = encodeBuffer[BUFF_ODD];
	data[idx++] = encodeBuffer[BUFF_EVEN];

	// Checksum
	uint8_t checksum = currentVolume ^ currentTrack ^ currentSector;
	oddEvenEncode(checksum, encodeBuffer);
	data[idx++] = encodeBuffer[BUFF_ODD];
	data[idx++] = encodeBuffer[BUFF_EVEN];

	// Epilogue
	for (int x = 0; x < DISK_EPILOGUE_LEN; x++)
		data[idx++] = epilogue[x];

	// Gap 2
	for (int x = 0; x < DISK_GAP2_LEN; x++)
		data[idx++] = DISK_SYNC_BYTE;

	/***** DATA FIELD *****/
	// Prologue
	for (int x = 0; x < DISK_PROLOGUE_LEN; x++)
		data[idx++] = dataPrologue[x];

	// Encode 6-and-2
	// Since currentTrack is [0..79], divide by 2 to get the .DSK equivalent of [0..34]
	unsigned int diskIdx = ((currentTrack/2) * DISK_SECTORS_PER_TRACK + currentSector) * DISK_BYTES_PER_SECTOR;
	printf("(currentTrack(%d) * DISK_SECTORS_PER_TRACK(%d) + currentSector(%d)) * DISK_BYTES_PER_SECTOR(%d) == %u\n", currentTrack/2, DISK_SECTORS_PER_TRACK, currentSector, DISK_BYTES_PER_SECTOR, diskIdx);
	uint8_t *currentSectorData = &diskImageData[diskIdx];
	encode6And2(currentSectorData, nibblizedSectorData);

	checksum = 0;
	
	int srcIdx = DISK_USERDATA_LEN - 1;

	// Writing the sector userdata is done in two steps: the last
	// 0x56 bytes, then the first 0xFF

	// First, BC55 -> BC00
	for (int x = 0; x < 86; x++) {
		uint8_t byte62 = nibblizedSectorData[srcIdx--];

		checksum = checksum ^ byte62;

		assert(byte62 < 64 && checksum < 64);

		// Translate on-the-fly
		data[idx++] = XLAT62[checksum];
	}

	// Then, BB00->BBFF
	for (int x = 0; x < 256; x++) {
		uint8_t byte62 = nibblizedSectorData[x];

		checksum = checksum ^ byte62;

		assert(byte62 < 64 && checksum < 64);

		// Translate on-the-fly
		data[idx++] = XLAT62[checksum];
	}

	// Data Field Checksum
	data[idx++] = XLAT62[checksum];

	// Data Field Epilogue
	for (int x = 0; x < DISK_EPILOGUE_LEN; x++)
		data[idx++] = epilogue[x];

	// Gap 3
	for (int x = 0; x < DISK_GAP3_LEN; x++)
		data[idx++] = DISK_SYNC_BYTE;

	assert(idx == DISK_RAW_SECTOR_LEN);

	return(true);
}
