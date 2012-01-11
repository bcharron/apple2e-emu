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
#define DISK_NB_PHASES 4

class Disk
{
public:
	Disk(void);
	bool init(void);
	void reset(void);
	bool openFile(std::string filename);
	void closeFile(void);
	unsigned char readByte(void);
	void motorOn(void);
	void motorOff(void);
	void writeByte(unsigned char byte);
	void phaseOn(unsigned char phaseNumber);
	void phaseOff(unsigned char phaseNumber);

private:
	bool updateShaftPosition(void);
	bool updateHeadTrack(void);
	void changePhase(uint8_t phaseNumber, bool value);

private:
	std::string diskImageFilename;
	bool phases[DISK_NB_PHASES];         // Status (on/off) of stepper motor phases (magnets)
	int currentTrack;                    // Track under the head [0..79]
	uint8_t currentSector;               // Sector under the head [0..15]
	uint16_t currentSectorPosition;      // Byte under the head in the current sector [0..?]
	unsigned char shaftPosition;         // Position of the shaft within the magnets
	unsigned char previousShaftPosition; // Position of the shaft within the magnets
};

#endif
