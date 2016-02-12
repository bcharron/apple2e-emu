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

void dumpHex(uint8_t *data, uint16_t len);

// Translation table for 6-and-2 encoding
uint8_t XLAT62[64] = {
	0x96, 0x97, 0x9A, 0x9B, 0x9D, 0x9E, 0x9F, 0xA6, 
	0xA7, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB2, 0xB3,
	0xB4, 0xB5, 0xB6, 0xB7, 0xB9, 0xBA, 0xBB, 0xBC,
	0xBD, 0xBE, 0xBF, 0xCB,	0xCD, 0xCE, 0xCF, 0xD3,
	0xD6, 0xD7, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE,
	0xDF, 0xE5, 0xE6, 0xE7, 0xE9, 0xEA, 0xEB, 0xEC,
	0xED, 0xEE, 0xEF, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6,
	0xF7, 0xF9, 0xFA, 0xFB,	0xFC, 0xFD, 0xFE, 0xFF };

// .DSK are generally in DOS 3.3 order, meaning the sectors are translated as
// follows.
unsigned int DOS33_SECTOR_XLAT[16] = {
	0,	// 0
	7,	// 1
	14,	// 2
	6,	// 3
	13,	// 4
	5,	// 5
	12,	// 6
	4,	// 7
	11,	// 8
	3,	// 9
	10,	// 10
	2,	// 11
	9,	// 12
	1,	// 13
	8,	// 14
	15	// 15
};

Disk::Disk(void)
	: diskImageFilename(""),
	  diskImageData(NULL),
	  diskImageOpened(false),
	  motorEnabled(false),
	  currentVolume(0xFE),
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

		printf("Building track %d, sector %d\n", currentTrack / 2, currentSector);
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

bool
Disk::isMotorEnabled(void)
{
	return(motorEnabled);
}

void
Disk::motorOn(void)
{
	printf("Disk motor ON\n");
	motorEnabled = true;
}

void
Disk::motorOff(void)
{
	printf("Disk motor OFF\n");
	motorEnabled = false;
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
		// printf("Shaft position: %d -> %d\n", previousShaftPosition, shaftPosition);
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
		printf("Changed track %d -> %d\n", previousTrack / 2, currentTrack / 2);
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
	// printf("phaseOn(%u)\n", phaseNumber);
	changePhase(phaseNumber, true);
}

void
Disk::phaseOff(uint8_t phaseNumber)
{
	// printf("phaseOff(%u)\n", phaseNumber);
	changePhase(phaseNumber, false);
}

/* Turns phase magnet on or off depending on 'value' */
void
Disk::changePhase(uint8_t phaseNumber, bool value)
{
	assert(phaseNumber < DISK_NB_PHASES);

	// printf("phase %d: %d -> %d\n", phaseNumber, phases[phaseNumber], value);
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

	// printf("READ[trk:%d sec:%d  pos:%d]: ", currentTrack, currentSector, currentSectorPosition);

	if (diskImageOpened) {
		byte = sectorRawData[currentSectorPosition];
		currentSectorPosition++;
		// printf("0x%02X\n", (unsigned char) byte);
	} else {
		printf("Disk image not opened. Returning garbage.\n");
		byte = 0xff;
	}

	if (currentSectorPosition >= DISK_RAW_SECTOR_LEN) {
		currentSectorPosition = 0;
		currentSector++;
		currentSector %= DISK_SECTORS_PER_TRACK;
		
		printf("Building track %d, sector %d\n", currentTrack / 2, currentSector);
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

/* Create the low bytes for use in 6-and-2 encoding */
uint8_t low2(uint8_t in) {
	uint8_t byte = in & 0x03;
	uint8_t bit0 = byte & 0x01;
	uint8_t bit1 = (byte & 0x02) >> 1;

	return(bit0 << 1 | bit1);
}

/*
 Take a 256-bytes block and encode it 6-and-2 into 342 bytes. Returns the
 "checksum". (The last byte, really.)
*/
uint8_t encode6And2(uint8_t *in, uint8_t *out)
{
	memset(out, 0x00, DISK_USERDATA_LEN);

	// Create the low (2 bits) bytes at buf[0x00..85]
	for (int x = 0; x < 0x56; x++) {
		// The last two bytes are special for b3 and the last byte is
		// special for b1, because 0x56 can't be divided by 3
		uint8_t b3 = x < 0x54 ? low2(in[0x56 * 2 + x]) : 0;
		uint8_t b2 = low2(in[0x56 + x]);
		uint8_t b1 = low2(in[x]);

		out[x] = b3 << 4 | b2 << 2 | b1;

		assert(out[x] < 64);
	}

	// Create the high (6 bits) bytes at buf[85..341]
	for (int src = 0, dst = 0x56; src < 256; src++, dst++) {
		out[dst] = in[src] >> 2;
	}

	// Chain-XOR every byte and create the checksum
	uint8_t tmp = 0;
	uint8_t last = 0;

	for (int x = 0; x < DISK_USERDATA_LEN; x++) {
		assert(tmp < 64);

		tmp = out[x] ^ last;

		last = out[x];
		out[x] = XLAT62[tmp];

		assert(out[x] >= 0x80);
	}

	// The last byte of the buffer is the checksum. It is XOR'd against
	// itself during reading/decoding, 0 indicating success.

	return(last);
}

void
dumpHex(uint8_t *data, uint16_t len)
{
        int x = 0;
	int offset = 0;

        while(len) {
                printf("%04X  ", offset);

                for (x = 0; x < 16 && len > 0; x++) {
                        printf("%02X ", data[offset + x]);
                        len--;
                }

                printf("   ");

                for (x = 0; x < 16; x++) {
                        uint8_t c = data[offset + x];
                        if (! isprint(c))
                                c = '.';

                        printf("%c", c);

                }

                printf("\n");

                offset += 16;
        }
}

/*
Prepare a buffer representing a disk sector
All disk reads will be served from this buffer until it reaches the end of the buffer.
*/
bool
Disk::buildSector(uint8_t sectorNumber, uint8_t *out)
{
	uint8_t *data = out;
	uint16_t idx = 0;
	uint8_t encodeBuffer[2];

	// currentTrack is 0..79, so divide by 2 to get the current track in DOS-parlance
	uint8_t thisTrack = currentTrack / 2;

	// Gap 1
	// XXX: Shouldn't these sync bytes be 10 bits (0xFF + '0' '0' ?)  Not clear what the hardware hides
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
	oddEvenEncode(thisTrack, encodeBuffer);
	data[idx++] = encodeBuffer[BUFF_ODD];
	data[idx++] = encodeBuffer[BUFF_EVEN];

	// Sector
	oddEvenEncode(currentSector, encodeBuffer);
	data[idx++] = encodeBuffer[BUFF_ODD];
	data[idx++] = encodeBuffer[BUFF_EVEN];

	// Checksum
	uint8_t checksum = currentVolume ^ thisTrack ^ currentSector;
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
	unsigned int translatedSector = DOS33_SECTOR_XLAT[currentSector];
	unsigned int diskIdx = (thisTrack * DISK_SECTORS_PER_TRACK + translatedSector) * DISK_BYTES_PER_SECTOR;
	// printf("(currentTrack(%d) * DISK_SECTORS_PER_TRACK(%d) + currentSector(%d)) * DISK_BYTES_PER_SECTOR(%d) == %u\n", thisTrack, DISK_SECTORS_PER_TRACK, currentSector, DISK_BYTES_PER_SECTOR, diskIdx);
	uint8_t *currentSectorData = &diskImageData[diskIdx];
	
	// printf("Decoded data:\n");
	// dumpHex(currentSectorData, 256);	

	checksum = encode6And2(currentSectorData, &data[idx]);

	// printf("Encoded data:\n");
	// dumpHex(&data[idx], DISK_USERDATA_LEN);
	idx += DISK_USERDATA_LEN;

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
