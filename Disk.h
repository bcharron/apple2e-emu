/*
 * Disk.h - <description>
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
 * Disk.h - Benjamin Charron <bcharron@pobox.com>
 * Created  : Fri Dec 23 09:26:05 2011
 * Revision : $Id$
 */

#include <string>

#ifndef _DISK_H
#define _DISK_H

#define DISK_MAX_TRACK 80
#define DISK_TRACKS_PER_DISK 35
#define DISK_SECTORS_PER_TRACK 16
#define DISK_BYTES_PER_SECTOR 256
#define DISK_NB_PHASES 4
#define DISK_SYNC_BYTE 0xff

// Length of a disk image
#define DISK_IMAGE_LEN DISK_TRACKS_PER_DISK * DISK_SECTORS_PER_TRACK * DISK_BYTES_PER_SECTOR

#define DISK_GAP1_LEN 10
#define DISK_GAP2_LEN 5
#define DISK_GAP3_LEN 20
#define DISK_USERDATA_LEN 342
#define DISK_DATAFIELD_CKSUM_LEN 1
#define DISK_PROLOGUE_LEN 3
#define DISK_EPILOGUE_LEN 3
#define DISK_ADDR_VOL_LEN 2
#define DISK_ADDR_TRK_LEN 2
#define DISK_ADDR_SEC_LEN 2
#define DISK_ADDR_CKSUM_LEN 2

#define DISK_ADDRFIELD_LEN DISK_PROLOGUE_LEN + DISK_EPILOGUE_LEN + DISK_ADDR_VOL_LEN + DISK_ADDR_TRK_LEN + DISK_ADDR_SEC_LEN + DISK_ADDR_CKSUM_LEN
#define DISK_DATAFIELD_LEN DISK_PROLOGUE_LEN + DISK_USERDATA_LEN + DISK_DATAFIELD_CKSUM_LEN + DISK_EPILOGUE_LEN
#define DISK_RAW_SECTOR_LEN DISK_GAP1_LEN + DISK_ADDRFIELD_LEN + DISK_GAP2_LEN + DISK_DATAFIELD_LEN + DISK_GAP3_LEN

class Disk
{
public:
	Disk(void);
	bool init(void);
	void reset(void);
	bool openFile(std::string filename);
	void closeFile(void);
	uint8_t readNextByte(void);
	void motorOn(void);
	void motorOff(void);
	void writeByte(unsigned char byte);
	void phaseOn(unsigned char phaseNumber);
	void phaseOff(unsigned char phaseNumber);


private:
	bool updateShaftPosition(void);
	bool updateHeadTrack(void);
	void changePhase(uint8_t phaseNumber, bool value);
	bool buildSector(uint8_t sectorNumber, uint8_t *out);

private:
	std::string diskImageFilename;
	uint8_t *diskImageData;
	bool diskImageOpened;
	bool phases[DISK_NB_PHASES];         // Status (on/off) of stepper motor phases (magnets)
	uint8_t currentVolume;
	uint8_t currentTrack;                    // Track under the head [0..79]
	uint8_t currentSector;               // Sector under the head [0..15]
	uint16_t currentSectorPosition;      // Byte under the head in the current sector [0..?] (gap1 + address field + gap2 + data field(342) + gap3)
	unsigned char shaftPosition;         // Position of the shaft within the magnets
	unsigned char previousShaftPosition; // Position of the shaft within the magnets
	uint8_t sectorRawData[DISK_RAW_SECTOR_LEN]; // 6-and-2, nibblized sector data for currentSector including gaps, address field and data field
};

#endif
