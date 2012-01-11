/*
 * Disk.cc - <description>
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

#include "Disk.h"

Disk::Disk(void)
	: diskImageFilename(NULL),
	  currentTrack(0),
	  currentSector(0),
	  currentSectorPosition(0)
{
	for (int x = 0; x < DISK_NB_PHASES; x++)
		phases[x] = 0;
}

bool
Disk::init(void)
{
	bool ret = false;

	ret = true;

	return(ret);
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
	bool ret = false;

	diskImageFilename = filename;

	return(ret);
}

void
Disk::closeFile(void)
{
	diskImageFilename = "";
}

unsigned char
Disk::readByte(void)
{
	unsigned char byte;

	return(byte);
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

	if (shaftPosition != previousShaftPosition)
		ret = true;

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
		if (shaftPosition == 0 && previousShaftPosition == DISK_NB_PHASES) {
			// Shaft turned clockwise and wrapped around
			currentTrack++;
		}
		else if (shaftPosition % 2 == 0) {
			// Shaft turned counter-clockwise one quarter-turn
			currentTrack--;
		}
	} else if (shaftPosition > previousShaftPosition) {
		if (shaftPosition == DISK_NB_PHASES && previousShaftPosition == 0) {
			// Shaft turned counter-clockwise and wrapped around
			currentTrack--;
		} else if (shaftPosition % 2 == 0) {
			// Shaft turned clockwise one quarter-turn
			currentTrack++;
		}
	}

	if (currentTrack < 0)
		currentTrack = 0;
	else if (currentTrack > DISK_MAX_TRACK)
		currentTrack = DISK_MAX_TRACK;

	bool ret = false;

	if (previousTrack != currentTrack)
		ret = true;

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

	phases[phaseNumber] = value;

	if (updateShaftPosition())
		updateHeadTrack();
}
