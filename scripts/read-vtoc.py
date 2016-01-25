#!/usr/bin/python

import sys
import struct

class VTOC:
	pass

class CatalogSector:
	pass

class CatalogFileEntry:
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
		pos = (self.sectors_per_track * self.bytes_per_sector) * track + (sector * self.bytes_per_sector)

		return(pos)

	def _read_sector(self, track, sector):
		pos = self._get_offset(track, sector)
		self.fd.seek(pos)
		buf = self.fd.read(256)

		return(buf)

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

	def read_catalog(self):
		buf = self._read_sector(self.vtoc.first_catalog_entry_track, self.vtoc.first_catalog_entry_sector)
		print "Reading catalog"
		print "Track: %02d   sector: %02d" % ( self.vtoc.first_catalog_entry_track, self.vtoc.first_catalog_entry_sector )

		catalog_sector = CatalogSector()
		catalog_sector.next_track = struct.unpack("B", buf[0x01])[0]
		catalog_sector.next_sector = struct.unpack("B", buf[0x02])[0]

		print "Catalog next track: 0x%02X" % catalog_sector.next_track
		print "Catalog next sector: 0x%02X" % catalog_sector.next_sector
		
		for entry_nr in range(7):
			ENTRY_SIZE = 0x23
			FIRST_ENTRY_OFFSET = 0x0B

			pos = entry_nr * ENTRY_SIZE + FIRST_ENTRY_OFFSET
			entry = self.parse_catalog_entry(buf[pos:pos + ENTRY_SIZE + 1])

			print "Entry %02d (type %02X, track %02d, sector %02d. Size: %02d sectors): %s" % ( entry_nr, entry.file_type, entry.first_track, entry.first_sector, entry.size, entry.filename)


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
dsk.read_catalog()
