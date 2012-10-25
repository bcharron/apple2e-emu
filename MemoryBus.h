#include <vector>
#include <stdint.h>

#include "MemoryRegion.h"
#include "Registers.h"

#define NB_REGIONS 9
enum memory_regions {
	REGION_MAIN_RAM = 0,
	REGION_MAIN_BANK2,
	REGION_MAIN_ROM,
	REGION_INTERNAL_ROM,
	REGION_AUX_RAM,
	REGION_AUX_BANK2,
	REGION_SOFT_SWITCHES,
	REGION_SLOT_ROMS,
	REGION_SLOT_IO,
};

/*
 * The memory bus is a linked list of all regions in the system: RAM, ROMs, screen memory, etc.
 */
class MemoryBus
{
public:
	MemoryBus(unsigned int size, registers_t *registers);
	void init(void);
	void addRegion(MemoryRegion *region);
	void setRegionData(enum memory_regions regionNumber, uint16_t size, uint8_t *data);
	uint8_t read(uint16_t offset);
	void write(uint16_t offset, uint8_t byte);
	unsigned int getSize(void);
	uint8_t readSoftSwitch(uint16_t offset);
	void writeSoftSwitch(uint16_t offset, uint8_t val);
	MemoryRegion* getRegion(enum memory_regions regionNumber);
	uint8_t access(uint16_t offset, bool write, uint8_t byte);

protected:
	unsigned int memorySize;
	MemoryRegion *regions[NB_REGIONS];
	registers_t *registers;
};
