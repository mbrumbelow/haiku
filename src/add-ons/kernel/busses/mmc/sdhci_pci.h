
#ifndef _SDHCI_PCI_H
#define _SDHCI_PCI_H


#include <device_manager.h>
#include <KernelExport.h>


#define SDHCI_PCI_SLOT_INFO 							0x40
#define SDHCI_PCI_SLOTS(x) 								((( x >> 4) & 7))
#define SDHCI_PCI_SLOT_INFO_FIRST_BASE_INDEX(x)			(( x ) & 7)

#define SDHCI_DEVICE_TYPE_ITEM 							"sdhci/type"
#define SDHCI_BUS_TYPE_NAME 							"bus/sdhci/v1"

#define SDHCI_CARD_DETECT								1 << 16

#define SDHCI_SOFTWARE_RESET_ALL						1 << 0

#define SDHCI_BASE_CLOCK_FREQ(x)						((x >> 8) & 63)
#define SDHCI_BASE_CLOCK_DIV_1							0 << 8
#define SDHCI_BASE_CLOCK_DIV_2							1 << 8
#define SDHCI_BASE_CLOCK_DIV_4							2 << 8
#define SDHCI_BASE_CLOCK_DIV_8							4 << 8
#define SDHCI_BASE_CLOCK_DIV_16							8 << 8
#define SDHCI_BASE_CLOCK_DIV_32							10 << 8
#define SDHCI_BASE_CLOCK_DIV_64							20 << 8
#define SDHCI_BASE_CLOCK_DIV_128						40 << 8
#define SDHCI_BASE_CLOCK_DIV_256						80 << 8
#define SDHCI_INTERNAL_CLOCK_ENABLE						1 << 0
#define SDHCI_INTERNAL_CLOCK_STABLE						1 << 1
#define SDHCI_SD_CLOCK_ENABLE							1 << 2
#define SDHCI_SD_CLOCK_DISABLE							~(1 << 2)



struct registers {
	volatile uint32_t system_address;
	volatile uint16_t block_size;
	volatile uint16_t block_count;
	volatile uint32_t argument;
	volatile uint16_t transfer_mode;
	volatile uint16_t command;
	volatile uint32_t response0;
	volatile uint32_t response2;
	volatile uint32_t response4;
	volatile uint32_t response6;
	volatile uint32_t buffer_data_port;
	volatile uint32_t present_state;
	volatile uint8_t host_control;
	volatile uint8_t power_control;
	volatile uint8_t block_gap_control;
	volatile uint8_t wakeup_control;
	volatile uint16_t clock_control;
	volatile uint8_t timeout_control;
	volatile uint8_t software_reset;
	volatile uint16_t normal_interrupt_status;
	volatile uint16_t error_interrupt_status;
	volatile uint16_t normal_interrupt_status_enable;
	volatile uint16_t error_interrupt_status_enable;
	volatile uint16_t normal_interrupt_signal_enable;
	volatile uint16_t error_interrupt_signal_enable;
	volatile uint16_t auto_cmd12_error_status;
	volatile uint32_t : 32;
	volatile uint32_t capabilities;
	volatile uint32_t capabilities_rsvd;
	volatile uint32_t max_current_capabilities;
	volatile uint32_t max_current_capabilities_rsvd;
	volatile uint32_t : 32;
	volatile uint32_t : 32;
	volatile uint32_t : 32;
	volatile uint16_t slot_interrupt_status;
	volatile uint16_t host_control_version;
} __attribute__((packed));

typedef void* sdhci_mmc_bus;

#define SDHCI_BUS_CONTROLLER_MODULE_NAME "bus_managers/mmc_bus/driver_v1"

#define	MMC_BUS_MODULE_NAME "bus_managers/mmc_bus/device/v1"

static void sdhci_register_dump(uint8_t, struct registers*);
static void sdhci_reset(struct registers*);
static void sdhci_set_clock(struct registers*);
static void sdhci_set_power(struct registers*);
static void sdhci_stop_clock(struct registers*);

typedef struct {
	driver_module_info info;
	
	void (*hey)();

//		static void sdhci_register_dump(struct registers* regs);

} sdhci_mmc_bus_interface;


#endif