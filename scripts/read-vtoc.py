#!/usr/bin/python

import sys
import struct

class VTOC:
	pass

class CatalogSector:
	pass

class CatalogFileEntry:
	pass

# A "Track/Sector List" entry
class TSListEntry:
	pass

class DSKFile:
	filename = None
	fd = None
	sectors_per_track = 16
	bytes_per_sector = 256
	vtoc_track_number = 17
	vtoc = VTOC()

	def __init__(self, filename):
		self.filename = filename

	def open(self):
		# Leave stack trace if open fails
		self.fd = open(filename, "r")
		print "Opened %s" % filename
			
	def read_vtoc(self):
		vtoc_position = self._get_offset(self.vtoc_track_number, 0)
		self.fd.seek(vtoc_position)
		self.vtoc = VTOC()
		self.vtoc.buf = self.fd.read(256)

		self.vtoc.first_catalog_entry_track = struct.unpack("B", self.vtoc.buf[1])[0]
		self.vtoc.first_catalog_entry_sector = struct.unpack("B", self.vtoc.buf[2])[0]
		self.vtoc.dos_release = struct.unpack("B", self.vtoc.buf[3])[0]
		self.vtoc.disk_vol = struct.unpack("B", self.vtoc.buf[6])[0]
		self.vtoc.max_tracks = struct.unpack("B", self.vtoc.buf[0x27])[0]
		self.vtoc.last_track = struct.unpack("B", self.vtoc.buf[0x30])[0]
		self.vtoc.nb_tracks_per_disk = struct.unpack("B", self.vtoc.buf[0x34])[0]
		self.vtoc.nb_sectors_per_track = struct.unpack("B", self.vtoc.buf[0x35])[0]
		self.vtoc.nb_bytes_per_sector = struct.unpack("<H", self.vtoc.buf[0x36:0x38])[0]

	def _get_offset(self, track, sector):
		real_sector = self._dos33_sector_skew(sector)
		pos = (self.sectors_per_track * self.bytes_per_sector) * track + (real_sector * self.bytes_per_sector)

		return(pos)

	# DOS 3.3 leaves the physical sectors in 0..15 order, but "skews" them
	# in software so that, for example, physical sector 1 corresponds
	# becomes "soft" sector 7. This is to achieve better performance.
	# But apparently it doesn't always apply.. I am mystified by this.
	def _dos33_sector_skew(self, sector):
		#TABLE = [ 0, 7, 14, 6, 13, 5, 12, 4, 11, 3, 10, 2, 9, 1, 8, 15 ]
		#return(TABLE[sector])

		return(sector)

	def _read_sector(self, track, sector):
		pos = self._get_offset(track, sector)
		self.fd.seek(pos)
		buf = self.fd.read(256)

		return(buf)

	# Read a "Track/Sector List Sector", which is a linked list of all
	# sectors of a file. (ie, block pointers in an inode)
	def _read_ts_list_sector(self, track, sector):
		buf = self._read_sector(track, sector)
		pos = 0

		entry = TSListEntry()

		entry.cur_track = track
		entry.cur_sector = sector
		
		# Byte 0x00 is unused
		pos += 1

		# The next track, if this file has more than 121 sectors
		entry.next_track = struct.unpack("B", buf[pos])[0]
		entry.next_sector = struct.unpack("B", buf[pos + 1])[0]
		pos += 2

		# Byte 0x03, 0x04 are unused
		pos += 2

		# Where to map this sector in the file's buffer.
		# ie, if it's 0, then this sector will be at position 0 of the
		# file, and the next sector will be at 0x100, the one after at
		# 0x200, etc.
		entry.file_offset = struct.unpack("<H", buf[pos:pos + 2])[0]
		pos += 2

		# Bytes 0x07 - 0x0B are unused
		pos += 5

		entry.sector_positions = []

		if pos != 0x0C:
			print "ERROR: pos is 0x%02X but should be 0x0C" % pos
			sys.exit(1)
		#pos = 0x0C

		while (pos < 0xFE):
			data_track = struct.unpack("B", buf[pos])[0]
			data_sector = struct.unpack("B", buf[pos + 1])[0]
			data_tuple = (data_track, data_sector)

			entry.sector_positions.append(data_tuple)
			pos += 2

		return(entry)

	def parse_catalog_entry(self, buf):
		entry = CatalogFileEntry()
		entry.first_track = struct.unpack("B", buf[0x00])[0]
		entry.first_sector = struct.unpack("B", buf[0x01])[0]
		entry.file_type = struct.unpack("B", buf[0x02])[0]
		entry.filename = list(struct.unpack("30B", buf[0x03:0x21]))
		for x in range(len(entry.filename)):
			entry.filename[x] = chr(entry.filename[x] & 0x7F)

		entry.filename = "".join(entry.filename)

		entry.size = struct.unpack("<H", buf[0x21:0x23])[0]

		return(entry)

	def read_catalog(self, track, sector):
		#buf = self._read_sector(self.vtoc.first_catalog_entry_track, self.vtoc.first_catalog_entry_sector)
		buf = self._read_sector(track, sector)
		print "Reading catalog at track %02d sector %02d" % ( track, sector )

		catalog_sector = CatalogSector()
		catalog_sector.next_track = struct.unpack("B", buf[0x01])[0]
		catalog_sector.next_sector = struct.unpack("B", buf[0x02])[0]

		print "Catalog next track: 0x%02X" % catalog_sector.next_track
		print "Catalog next sector: 0x%02X" % catalog_sector.next_sector
		
		for entry_nr in range(7):
			ENTRY_SIZE = 0x23
			FIRST_ENTRY_OFFSET = 0x0B
			DELETED_FILE_TRACK = 0xFF
			UNUSED_ENTRY_TRACK = 0x00

			pos = entry_nr * ENTRY_SIZE + FIRST_ENTRY_OFFSET
			entry = self.parse_catalog_entry(buf[pos:pos + ENTRY_SIZE + 1])

			#print "pos: %02X" % pos

			print "Entry %02d (type %02X, track %02d, sector %02d. Size: %02d sectors): %s" % ( entry_nr, entry.file_type, entry.first_track, entry.first_sector, entry.size, entry.filename)

			if entry.first_track == DELETED_FILE_TRACK:
				print "\tFile is DELETED."
			elif entry.first_track != UNUSED_ENTRY_TRACK:
				ts_entry_nr = 0
				ts_entry = self._read_ts_list_sector(entry.first_track, entry.first_sector)

				self.print_ts_entry(ts_entry_nr, ts_entry)

				while ts_entry.next_track != 0x00:
					ts_entry_nr += 1
					ts_entry = self._read_ts_list_sector(ts_entry.next_track, ts_entry.next_sector)
					self.print_ts_entry(ts_entry_nr, ts_entry)

		# The last catalog sector has a 0 pointer
		if catalog_sector.next_track != 0 and catalog_sector.next_sector != 0:
			self.read_catalog(catalog_sector.next_track, catalog_sector.next_sector)

	def print_ts_entry(self, ts_entry_nr, ts_entry):
		print "\tT/S Entry #%02d   File offset: 0x%04X    Track 0x%02X (%d)  Sector 0x%02X (%d)" % ( ts_entry_nr, ts_entry.file_offset, ts_entry.cur_track, ts_entry.cur_track, ts_entry.cur_sector, ts_entry.cur_sector )
		print "\tNext T/S track: 0x%02X    Next T/S sector: 0x%02X" % ( ts_entry.next_track, ts_entry.next_sector )

		x = 0
		for pos in ts_entry.sector_positions:
			# The list stops when track is zero
			if pos[0] == 0x00:
				break

			print "\t\tOffset 0x%04X  track 0x%02X (%d) sector 0x%02X (%d)" % ( ts_entry.file_offset + (x * 256), pos[0], pos[0], pos[1], pos[1] )
			x += 1

	def print_vtoc(self):
		
		print "First catalog sector track: 0x%02X (%02d)" % ( self.vtoc.first_catalog_entry_track, self.vtoc.first_catalog_entry_track )
		print "First catalog sector sector: 0x%02X (%02d)" % ( self.vtoc.first_catalog_entry_sector, self.vtoc.first_catalog_entry_sector )
		print "DOS Release: 0x%02X (%02d)" % ( self.vtoc.dos_release, self.vtoc.dos_release )
		print "Diskette volume number: 0x%02X (%02d)" % ( self.vtoc.disk_vol, self.vtoc.disk_vol )
		print "Max track/sectors (122 = 256 byte sectors): 0x%02X (%02d)" % ( self.vtoc.max_tracks, self.vtoc.max_tracks )
		print "Last allocated track: 0x%02X (%02d)" % ( self.vtoc.last_track, self.vtoc.last_track )
		print ""
		print "Number of tracks per disk: 0x%02X (%02d)" % ( self.vtoc.nb_tracks_per_disk, self.vtoc.nb_tracks_per_disk )
		print "Number of sectors per track: 0x%02X (%02d)" % ( self.vtoc.nb_sectors_per_track, self.vtoc.nb_sectors_per_track )
		print "Number of bytes per sector: 0x%02X (%02d)" % ( self.vtoc.nb_bytes_per_sector, self.vtoc.nb_bytes_per_sector )
		print ""
		print "Free sectors:"

		for track in range(0, 35):
			pos = 0x38 + 4 * track
			buf = self.vtoc.buf[pos:pos + 4]

			bitmaps = struct.unpack("BBBB", buf)
			sectors = []

			#print bitmaps

			for i in range(0, 8):
				bit = 1 << i

				if bitmaps[1] & bit:
					sectors.append(i)
					#print "\tTrack %02d sector %02d" % ( track, i )

			for i in range(0, 8):
				bit = 1 << i

				if bitmaps[0] & bit:
					sectors.append(i + 8)
					#print "\tTrack %02d sector %02d" % ( track, i + 8 )

			l = " ".join([str(s) for s in sectors])
			print "Track %02d: %s" % ( track, l )

		print ""


filename = sys.argv[1]
dsk = DSKFile(filename)

dsk.open()
dsk.read_vtoc()
dsk.print_vtoc()
dsk.read_catalog(dsk.vtoc.first_catalog_entry_track, dsk.vtoc.first_catalog_entry_sector)
