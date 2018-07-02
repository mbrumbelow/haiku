
#ifndef _SDHCI_PCI_H
#define _SDHCI_PCI_H 

#include <device_manager.h>
#include <KernelExport.h>

#define SDHCI_PCI_SLOT_INFO 							0x40
#define SDHCI_PCI_SLOTS(x) 								((( x >> 4) & 7))
#define SDHCI_PCI_SLOT_INFO_FIRST_BASE_INDEX(x)			(( x ) & 7)

#define SDHCI_BLOCK_COUNT 								0x06

#define SDHCI_CLOCK_CONTROL_REGISTER					0x2C

#define SDHCI_NORMAL_INTERRUPT_STATUS_REGISTER 			0x30

#define SDHCI_HOST_CONTROLLER_VERSION					0xFC

#define SDHCI_NORMAL_INTERRUPT_ENABLE_STATUS_REGISTER			0x34
#define SDHCI_NORMAL_INTERRUPT_ENABLE_SIGNAL_REGISTER			0x38

#define SDHCI_HOST_CONTROL_REGISTER_1					0x28

#define SDHCI_DEVICE_TYPE_ITEM 							"sdhci/type"
#define SDHCI_BUS_TYPE_NAME 							"bus/sdhci/v1"

#define SDHCI_PRESENT_STATE_REGISTER 					0x24

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

static void sdhci_register_dump(uint8_t, struct registers*);
static void sdhci_reset(volatile uint32_t*, volatile uint16_t*, volatile uint8_t*, volatile uint8_t*);
static void sdhci_set_clock(volatile uint32_t*, volatile uint16_t*);
static void sdhci_set_power(volatile uint32_t*, volatile uint32_t*, volatile uint8_t*);
static void sdhci_stop_clock(volatile uint16_t*);

typedef struct {
	driver_module_info info;


//		static void sdhci_register_dump(struct registers* regs);

} sdhci_mmc_bus_interface;


#endif
