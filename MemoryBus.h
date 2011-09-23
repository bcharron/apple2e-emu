#include <vector>
#include <stdint.h>
#include "MemoryRegion.h"

typedef struct soft_switches_struct {
	bool altCharset;
	bool text;
	bool mixed;
	bool page2;
	bool hires;
	bool text80Col;
	bool text80Store;
	bool vbl;
	bool altzp;        // Pages 0 and 1 go go AUX mem
	bool ramrd;        // Reads go to AUX mem
	bool ramwrt;       // Writes go to AUX mem
	bool bankRead;
	bool bankWrite;
	bool useBank2;     // 0: Read from bank 1.  1: Read from bank 2.
} soft_switches_t;

#define NB_REGIONS 8
enum memory_regions {
	REGION_MAIN_RAM,
	REGION_MAIN_ROM,
	REGION_INTERNAL_ROM,
	REGION_AUX_RAM,
	REGION_AUX_BANK2,
	REGION_MAIN_BANK1,
	REGION_SOFT_SWITCHES,
	REGION_SLOT_ROMS,
};

/*
 * The memory bus is a linked list of all regions in the system: RAM, ROMs, screen memory, etc.
 */
class MemoryBus
{
public:
	MemoryBus(unsigned int size);
	void init(void);
	void addRegion(MemoryRegion *region);
	//void setRegion(MemoryRegion *region, uint16_t start, uint16_t end);
	void setRegionData(enum memory_regions regionNumber, uint16_t size, uint8_t *data);
	uint8_t read(uint16_t offset);
	void write(uint16_t offset, uint8_t byte);
	unsigned int getSize(void);
	uint8_t readSoftSwitch(uint16_t offset);
	void writeSoftSwitch(uint16_t offset, uint8_t val);
	MemoryRegion* getRegion(enum memory_regions regionNumber);

protected:
	MemoryRegion* findRegion(uint16_t offset);
	unsigned int memorySize;
	soft_switches_t softSwitches;
	MemoryRegion *pageTable[256];

/*
	 std::vector<MemoryRegion*> regions;
	MemoryRegion *ram;
	MemoryRegion *auxiliary;
	MemoryRegion *bankSwitched1;
	MemoryRegion *bankSwitched2;
	MemoryRegion *internalROM;
	MemoryRegion *mainROM;
	MemoryRegion *ioROM;
*/
	MemoryRegion *regions[NB_REGIONS];
};
