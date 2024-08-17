/*
 *	Copyright 2024, Haiku Inc. All Rights Reserved.
 *	Distributed under the terms of the MIT License.
 */

#ifndef PCI_H
#define PCI_H

#include <bus_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	B_PCI_MODULE_NAME   "bus_managers/pci/v1"
/*
*	pci device info
*/

typedef struct pci_info {
    uint16 vendor_id;
    uint16 device_id;
    uint8 bus;
    uint8 device;
    uint8 function;
    uint8 revision;
    uint8 class_api;
    uint8 class_sub;
    uint8 class_base;
    uint8 line_size;
    uint8 latency;
    uint8 header_type;
    uint8 bist;
    uint8 reserved;
    union {
        struct {
  uint32 cardbus_cis;
  uint16 subsystem_id;
  uint16 subsystem_vendor_id;
  uint32 rom_base;
  uint32 rom_base_pci;
  uint32 rom_size;
  uint32 base_registers[6];
  uint32 base_registers_pci[6];
  uint32 base_register_sizes[6];
  uint8 base_register_flags[6];
  uint8 interrupt_line;
  uint8 interrupt_pin;
  uint8 min_grant;
  uint8 max_latency;
        };
        struct {
  uint32 base_registers[2];
 uint32 base_registers_pci[2];
 uint32 base_register_sizes[2];
   uint8 base_register_flags[2];
   uint8 primary_bus;
   uint8 secondary_bus;
   uint8 subordinate_bus;
   uint8 secondary_latency;
uint8 io_base;
   uint8 io_limit;
   uint16 secondary_status;
   uint16 memory_base;
   uint16 memory_limit;
   uint16 prefetchable_memory_base;
   uint16 prefetchable_memory_limit;
    uint32 prefetchable_memory_base_upper32;
   uint32 prefetchable_memory_limit_upper32;
   uint16 io_base_upper16;
   uint16 io_limit_upper16;
   uint32 rom_base;
   uint32 rom_base_pci;
   uint8 interrupt_line;
   uint8 interrupt_pin;
   uint16 bridge_control;
   uint16 subsystem_id;
   uint16 subsystem_vendor_id;
        };
        struct {
   uint16 subsystem_id;
   uint16 subsystem_vendor_id;
#ifdef __HAIKU_PCI_BUS_MANAGER_TESTING 
            // for testing only, not final (do not use!): 
   uint8 primary_bus;
   uint8 secondary_bus;
   uint8 subordinate_bus;
   uint8 secondary_latency;
   uint16 reserved;
   uint32 memory_base;
   uint32 memory_limit;
   uint32 memory_base_upper32;
   uint32 memory_limit_upper32;
   uint32 io_base;
   uint32 io_limit;
   uint32 io_base_upper32;
   uint32 io_limit_upper32;
   uint16 secondary_status;
   uint16 bridge_control;
#endif /* __HAIKU_PCI_BUS_MANAGER_TESTING */
        };
    };
} pci_info;

typedef struct pci_module_info {
    bus_manager_info binfo;

    uint8 (*read_io_8)(
        int mapped_io_addr
    );
    void (*write_io_8)(
        int mapped_io_addr,
        uint8 value
    );
    uint16 (*read_io_16)(
        int mapped_io_addr
    );
    void (*write_io_16)(
        int mapped_io_addr,
        uint16 value
    );
    uint32 (*read_io_32)(
        int mapped_io_addr
    );
    void (*write_io_32)(
        int mapped_io_addr,
        uint32 value
    );

    long (*get_nth_pci_info)(
        long index,
        pci_info *info
    );
    uint32 (*read_pci_config)(
        uint8 bus,
        uint8 device,
        uint8 function,
        uint16 offset,
        uint8 size
    );
    void (*write_pci_config)(
        uint8 bus,
        uint8 device,
        uint8 function,
        uint16 offset,
        uint8 size,
        uint32 value
    );

    phys_addr_t (*ram_address)(
        phys_addr_t physical_address_in_system_memory
    );

    status_t (*find_pci_capability)(
        uint8 bus,
        uint8 device,
        uint8 function,
        uint8 cap_id,
        uint8 *offset
    );
    status_t (*reserve_device)(
        uint8 bus,
        uint8 device,
        uint8 function,
        const char *driver_name,
        void *cookie
    );
    status_t (*unreserve_device)(
        uint8 bus,
        uint8 device,
        uint8 function,
        const char *driver_name,
        void *cookie
    );
    status_t (*update_interrupt_line)(
        uint8 bus,
        uint8 device,
        uint8 function,
        uint8 newInterruptLineValue
    );
    status_t (*find_pci_extended_capability)(
        uint8 bus,
        uint8 device,
        uint8 function,
        uint16 cap_id,
        uint16 *offset
    );
    status_t (*get_powerstate)(
        uint8 bus,
        uint8 device,
        uint8 function,
        uint8* state
    );
    status_t (*set_powerstate)(
        uint8 bus,
        uint8 device,
        uint8 function,
        uint8 newState
    );
    uint32 (*get_msi_count)(
        uint8 bus,
        uint8 device,
        uint8 function
    );
    status_t (*configure_msi)(
        uint8 bus,
        uint8 device,
        uint8 function,
        uint32 count,
        uint32 *startVector
    );
    status_t (*unconfigure_msi)(
        uint8 bus,
        uint8 device,
        uint8 function
    );
    status_t (*enable_msi)(
        uint8 bus,
        uint8 device,
        uint8 function
    );
    status_t (*disable_msi)(
        uint8 bus,
        uint8 device,
        uint8 function
    );
    uint32 (*get_msix_count)(
        uint8 bus,
        uint8 device,
        uint8 function
    );
    status_t (*configure_msix)(
        uint8 bus,
        uint8 device,
        uint8 function,
        uint32 count,
        uint32 *startVector
    );
    status_t (*enable_msix)(
        uint8 bus,
        uint8 device,
        uint8 function
    );
} pci_module_info;


/*
*	offsets in PCI configuration space to the elements of the predefined
*	header common to all header types
*/

#define PCI_vendor_id			0x00
#define PCI_device_id			0x02
#define PCI_command				0x04
#define PCI_status				0x06
#define PCI_revision			0x08
#define PCI_class_api			0x09
#define PCI_class_sub			0x0a
#define PCI_class_base			0x0b
#define PCI_line_size			0x0c
#define PCI_latency				0x0d
#define PCI_header_type			0x0e
#define PCI_bist				0x0f
#define PCI_extended_capability 0x100

/*
*	offsets in PCI configuration space to the elements of the predefined
*	header common to header types 0x00 and 0x01
*/
#define PCI_base_registers		0x10
#define PCI_interrupt_line		0x3c
#define PCI_interrupt_pin		0x3d



/*
*	offsets in PCI configuration space to the elements of header type 0x00
*/

#define PCI_cardbus_cis			0x28
#define PCI_subsystem_vendor_id	0x2c
#define PCI_subsystem_id		0x2e
#define PCI_rom_base			0x30
#define PCI_capabilities_ptr    0x34
#define PCI_min_grant			0x3e
#define PCI_max_latency			0x3f


/*
*	offsets in PCI configuration space to the elements of header type 0x01 (PCI-to-PCI bridge)
*/

#define PCI_primary_bus								0x18
#define PCI_secondary_bus							0x19
#define PCI_subordinate_bus							0x1A
#define PCI_secondary_latency						0x1B
#define PCI_io_base									0x1C
#define PCI_io_limit								0x1D
#define PCI_secondary_status						0x1E
#define PCI_memory_base								0x20
#define PCI_memory_limit							0x22
#define PCI_prefetchable_memory_base				0x24
#define PCI_prefetchable_memory_limit				0x26
#define PCI_prefetchable_memory_base_upper32		0x28
#define PCI_prefetchable_memory_limit_upper32		0x2C
#define PCI_io_base_upper16							0x30
#define PCI_io_limit_upper16						0x32
#define PCI_sub_vendor_id_1                         0x34
#define PCI_sub_device_id_1                         0x36
#define PCI_bridge_rom_base							0x38
#define PCI_bridge_control							0x3E

#define PCI_capabilities_ptr_2                      0x14
#define PCI_secondary_status_2                      0x16
#define PCI_primary_bus_2							0x18
#define PCI_secondary_bus_2							0x19
#define PCI_subordinate_bus_2						0x1A
#define PCI_secondary_latency_2                     0x1B
#define PCI_memory_base0_2                          0x1C
#define PCI_memory_limit0_2                         0x20
#define PCI_memory_base1_2                          0x24
#define PCI_memory_limit1_2                         0x28
#define PCI_io_base0_2                              0x2c
#define PCI_io_limit0_2                             0x30
#define PCI_io_base1_2                              0x34
#define PCI_io_limit1_2                             0x38
#define PCI_bridge_control_2                        0x3E

#define PCI_sub_vendor_id_2                         0x40
#define PCI_sub_device_id_2                         0x42

#define PCI_card_interface_2                        0x44

/*
*	values for the class_base field in the common header
*/
#define PCI_early 0x00
#define PCI_mass_storage 0x01
#define PCI_network 0x02
#define PCI_display 0x03
#define PCI_multimedia 0x04
#define PCI_memory 0x05
#define PCI_bridge 0x06
#define PCI_simple_communications 0x07
#define PCI_base_peripheral 0x08
#define PCI_input 0x09
#define PCI_docking_station 0x0a
#define PCI_processor 0x0b
#define PCI_serial_bus 0x0c
#define PCI_wireless 0x0d
#define PCI_intelligent_io 0x0e
#define PCI_satellite_communications 0x0f
#define PCI_encryption_decryption 0x10
#define PCI_data_acquisition 0x11
#define PCI_processing_accelerator 0x12
#define PCI_nonessential_function 0x13
#define PCI_undefined 0xFF
/*
*	values for the class_sub field for class_base = 0x00 (built before
*	class codes were defined)
*/
#define PCI_early_not_vga 0x00
#define PCI_early_vga 0x01
/*
*	values for the class_sub field for class_base = 0x01 (mass storage)
*/
#define PCI_scsi 0x00
#define PCI_ide 0x01
#define PCI_floppy 0x02
#define PCI_ipi 0x03
#define PCI_raid 0x04
#define PCI_ata 0x05
#define PCI_sata 0x06
#define PCI_sas 0x07
#define PCI_nvm 0x08
#define PCI_ufs 0x09
#define PCI_mass_storage_other 0x80
/*
*	bit mask of the class_api field for
*		class_base = 0x01 (mass storage)
*		class_sub = 0x01 (IDE controller)
*/
#define PCI_ide_primary_native 0x01
#define PCI_ide_primary_fixed 0x02
#define PCI_ide_secondary_native 0x04
#define PCI_ide_secondary_fixed 0x08
#define PCI_ide_master 0x80
/*
*	values of the class_api field for
*		class_base = 0x01 (mass storage)
*		class_sub = 0x06 (Serial ATA controller)
*/
#define PCI_sata_other 0x00
#define PCI_sata_ahci 0x01
/*
*	values of the class_api field for
*		class_base = 0x01 (mass storage)
*		class_sub = 0x08 (NVM Express controller)
*/
#define PCI_nvm_other 0x00
#define PCI_nvm_hci 0x01
#define PCI_nvm_hci_enterprise 0x02
/*
*	values of the class_api field for
*		class_base = 0x01 (mass storage)
*		class_sub = 0x09 (Universal Flash Storage controller)
*/
#define PCI_ufs_other 0x00
#define PCI_ufs_hci 0x01
/*
*	values for the class_sub field for class_base = 0x02 (network)
*/
#define PCI_ethernet 0x00
#define PCI_token_ring 0x01
#define PCI_fddi 0x02
#define PCI_atm 0x03
#define PCI_isdn 0x04
#define PCI_worldfip 0x05
#define PCI_picmg 0x06
#define PCI_network_infiniband 0x07
#define PCI_hfc 0x08
#define PCI_network_other 0x80
/*
*	values for the class_sub field for class_base = 0x03 (display)
*/
#define PCI_vga 0x00
#define PCI_xga 0x01
#define PCI_3d 0x02
#define PCI_display_other 0x80
/*
*	values for the class_sub field for class_base = 0x04 (multimedia device)
*/
#define PCI_video 0x00
#define PCI_audio 0x01
#define PCI_telephony 0x02
#define PCI_hd_audio 0x03
#define PCI_multimedia_other 0x80
/*
*	values of the class_api field for
*		class_base = 0x04 (multimedia device)
*		class_sub = 0x03 (HD audio)
*/
#define PCI_hd_audio_vendor 0x80
/*
*	values for the class_sub field for class_base = 0x05 (memory)
*/
#define PCI_ram 0x00
#define PCI_flash 0x01
#define PCI_memory_other 0x80
/*
*	values for the class_sub field for class_base = 0x06 (bridge)
*/
#define PCI_host 0x00
#define PCI_isa 0x01
#define PCI_eisa 0x02
#define PCI_microchannel 0x03
#define PCI_pci 0x04
#define PCI_pcmcia 0x05
#define PCI_nubus 0x06
#define PCI_cardbus 0x07
#define PCI_raceway 0x08
#define PCI_bridge_transparent 0x09
#define PCI_bridge_infiniband 0x0a
#define PCI_bridge_as_pci 0x0b
#define PCI_bridge_other 0x80
/*
*	values of the class_api field for
*		class_base = 0x06 (bridge), and
*		class_sub = 0x0b (Advanced Switching to PCI host bridge)
*/
#define PCI_bridge_as_pci_asi_sig 0x01
/*
    values for the class_sub field for class_base = 0x07 (simple
    communications controllers)
*/
#define PCI_serial 0x00
#define PCI_parallel 0x01
#define PCI_multiport_serial 0x02
#define PCI_modem 0x03
#define PCI_gpib 0x04
#define PCI_smart_card 0x05
#define PCI_simple_communications_other 0x80
/*
*	values of the class_api field for
*		class_base = 0x07 (simple communications), and
*		class_sub = 0x00 (serial port controller)
*/
#define PCI_serial_xt 0x00
#define PCI_serial_16450 0x01
#define PCI_serial_16550 0x02
/*
*	values of the class_api field for
*		class_base = 0x07 (simple communications), and
*		class_sub = 0x01 (parallel port)
*/
#define PCI_parallel_simple 0x00
#define PCI_parallel_bidirectional 0x01
#define PCI_parallel_ecp 0x02
/*
*	values for the class_sub field for class_base = 0x08 (generic
*	system peripherals)
*/
#define PCI_pic 0x00
#define PCI_dma 0x01
#define PCI_timer 0x02
#define PCI_rtc 0x03
#define PCI_generic_hot_plug 0x04
#define PCI_sd_host 0x05
#define PCI_iommu 0x06
#define PCI_rcec 0x07
#define PCI_system_peripheral_other 0x80
/*
*	values of the class_api field for
*		class_base = 0x08 (generic system peripherals)
*		class_sub = 0x00 (peripheral interrupt controller)
*/
#define PCI_pic_8259 0x00
#define PCI_pic_isa 0x01
#define PCI_pic_eisa 0x02
/*
*	values of the class_api field for
*		class_base = 0x08 (generic system peripherals)
*		class_sub = 0x01 (dma controller)
*/
#define PCI_dma_8237 0x00
#define PCI_dma_isa 0x01
#define PCI_dma_eisa 0x02
/*
*	values of the class_api field for
*		class_base = 0x08 (generic system peripherals)
*		class_sub = 0x02 (timer)
*/
#define PCI_timer_8254 0x00
#define PCI_timer_isa 0x01
#define PCI_timer_eisa 0x02
/*
*	values of the class_api field for
*		class_base = 0x08 (generic system peripherals)
*		class_sub = 0x03 (real time clock
*/
#define PCI_rtc_generic 0x00
#define PCI_rtc_isa 0x01
/*
*	values for the class_sub field for class_base = 0x09 (input devices)
*/
#define PCI_keyboard 0x00
#define PCI_pen 0x01
#define PCI_mouse 0x02
#define PCI_scanner 0x03
#define PCI_gameport 0x04
#define PCI_input_other 0x80
/*
*	values for the class_sub field for class_base = 0x0a (docking stations)
*/
#define PCI_docking_generic 0x00
#define PCI_docking_other 0x80
/*
*	values for the class_sub field for class_base = 0x0b (processor)
*/
#define PCI_386 0x00
#define PCI_486 0x01
#define PCI_pentium 0x02
#define PCI_alpha 0x10
#define PCI_PowerPC 0x20
#define PCI_mips 0x30
#define PCI_coprocessor 0x40
/*
*	values for the class_sub field for class_base = 0x0c (serial bus
*	controller)
*/
#define PCI_firewire 0x00
#define PCI_access 0x01
#define PCI_ssa 0x02
#define PCI_usb 0x03
#define PCI_fibre_channel 0x04
#define PCI_smbus 0x05
#define PCI_infiniband 0x06
#define PCI_ipmi 0x07
#define PCI_sercos 0x08
#define PCI_canbus 0x09
#define PCI_mipi_i3c 0x0a
/*
*	values of the class_api field for
*		class_base = 0x0c ( serial bus controller )
*		class_sub = 0x03 ( Universal Serial Bus  )
*/
#define PCI_usb_uhci 0x00
#define PCI_usb_ohci 0x10
#define PCI_usb_ehci 0x20
#define PCI_usb_xhci 0x30
#define PCI_usb_usb4 0x40
/*
*	values for the class_sub field for class_base = 0x0d (wireless controller)
*/
#define PCI_wireless_irda 0x00
#define PCI_wireless_consumer_ir 0x01
#define PCI_wireless_rf 0x10
#define PCI_wireless_bluetooth 0x11
#define PCI_wireless_broadband 0x12
#define PCI_wireless_80211A 0x20
#define PCI_wireless_80211B 0x21
#define PCI_wireless_cellular 0x40
#define PCI_wireless_cellular_ethernet 0x41
#define PCI_wireless_other 0x80
/*
*	values for the class_sub field for class_base = 0x10 (encryption decryption)
*/
#define PCI_encryption_decryption_network_computing 0x00
#define PCI_encryption_decryption_entertainment 0x10
#define PCI_encryption_decryption_other 0x80
/*
*	values for the class_sub field for class_base = 0x11 (data acquisition)
*/
#define PCI_data_acquisition_dpio 0x00
#define PCI_data_acquisition_performance_counters 0x01
#define PCI_data_acquisition_communication_synchroniser 0x10
#define PCI_data_acquisition_management 0x20
#define PCI_data_acquisition_other 0x80
/*
*	masks for command register bits
*/
#define PCI_command_io 0x001
#define PCI_command_memory 0x002
#define PCI_command_master 0x004
#define PCI_command_special 0x008
#define PCI_command_mwi 0x010
#define PCI_command_vga_snoop 0x020
#define PCI_command_parity 0x040
#define PCI_command_address_step 0x080
#define PCI_command_serr 0x100
#define PCI_command_fastback 0x200
#define PCI_command_int_disable 0x400
/*
*	masks for status register bits
*/
#define PCI_status_capabilities 0x0010
#define PCI_status_66_MHz_capable 0x0020
#define PCI_status_udf_supported 0x0040
#define PCI_status_fastback 0x0080
#define PCI_status_parity_signalled 0x0100
#define PCI_status_devsel 0x0600
#define PCI_status_target_abort_signalled 0x0800
#define PCI_status_target_abort_received 0x1000
#define PCI_status_master_abort_received 0x2000
#define PCI_status_serr_signalled 0x4000
#define PCI_status_parity_error_detected 0x8000
/*
*	masks for devsel field in status register
*/
#define PCI_status_devsel_fast 0x0000
#define PCI_status_devsel_medium 0x0200
#define PCI_status_devsel_slow 0x0400
/*
*	masks for header type register
*/
#define PCI_header_type_mask 0x7F
#define PCI_multifunction 0x80
// types of PCI header
#define PCI_header_type_generic 0x00
#define PCI_header_type_PCI_to_PCI_bridge 0x01
#define PCI_header_type_cardbus 0x02
/*
*	masks for built in self test (bist) register bits
*/
#define PCI_bist_code 0x0F
#define PCI_bist_start 0x40
#define PCI_bist_capable 0x80
// masks for flags in the various base address registers
#define PCI_address_space 0x01
#define PCI_register_start 0x10
#define PCI_register_end 0x24
#define PCI_register_ppb_end 0x18
#define PCI_register_pcb_end 0x14
// masks for flags in memory space base address registers
#define PCI_address_type_32 0x00
#define PCI_address_type_32_low 0x02
#define PCI_address_type_64 0x04
#define PCI_address_type 0x06
#define PCI_address_prefetchable 0x08
#define PCI_address_memory_32_mask 0xFFFFFFF0
/*
*	masks for flags in i/o space base address registers
*/
#define PCI_address_io_mask 0xFFFFFFFC
#define PCI_range_memory_mask 0xFFFFFFF0
/*
*	masks for flags in expansion rom base address registers
*/
#define PCI_rom_enable 0x00000001
#define PCI_rom_shadow 0x00000010
#define PCI_rom_copy 0x00000100
#define PCI_rom_bios 0x00001000
#define PCI_rom_address_mask 0xFFFFF800
// PCI interrupt pin values
#define PCI_pin_mask 0x07
#define PCI_pin_none 0x00
#define PCI_pin_a 0x01
#define PCI_pin_b 0x02
#define PCI_pin_c 0x03
#define PCI_pin_d 0x04
#define PCI_pin_max 0x04
// PCI bridge control register bits
#define PCI_bridge_parity_error_response 0x0001
#define PCI_bridge_serr 0x0002
#define PCI_bridge_isa 0x0004
#define PCI_bridge_vga 0x0008
#define PCI_bridge_master_abort 0x0020
#define PCI_bridge_secondary_bus_reset 0x0040
#define PCI_bridge_secondary_bus_fastback 0x0080
#define PCI_bridge_primary_discard_timeout 0x0100
#define PCI_bridge_secondary_discard_timeout 0x0200
#define PCI_bridge_discard_timer_status 0x0400
#define PCI_bridge_discard_timer_serr 0x0800
// PCI Capability Codes
#define PCI_cap_id_reserved 0x00
#define PCI_cap_id_pm 0x01
#define PCI_cap_id_agp 0x02
#define PCI_cap_id_vpd 0x03
#define PCI_cap_id_slotid 0x04
#define PCI_cap_id_msi 0x05
#define PCI_cap_id_chswp 0x06
#define PCI_cap_id_pcix 0x07
#define PCI_cap_id_ht 0x08
#define PCI_cap_id_vendspec 0x09
#define PCI_cap_id_debugport 0x0a
#define PCI_cap_id_cpci_rsrcctl 0x0b
#define PCI_cap_id_hotplug 0x0c
#define PCI_cap_id_subvendor 0x0d
#define PCI_cap_id_agp8x 0x0e
#define PCI_cap_id_secure_dev 0x0f
#define PCI_cap_id_pcie 0x10
#define PCI_cap_id_msix 0x11
#define PCI_cap_id_sata 0x12
#define PCI_cap_id_pciaf 0x13
#define PCI_cap_id_ea 0x14
#define PCI_cap_id_fpb 0x15
// PCI Extended Capabilities
#define PCI_extcap_id(x) (x & 0x0000ffff)
#define PCI_extcap_version(x) ((x & 0x000f0000) >> 16)
#define PCI_extcap_next_ptr(x) ((x & 0xfff00000) >> 20)
#define PCI_extcap_id_aer 0x0001
#define PCI_extcap_id_vc 0x0002
#define PCI_extcap_id_serial 0x0003
#define PCI_extcap_id_power_budget 0x0004
#define PCI_extcap_id_rcl_decl 0x0005
#define PCI_extcap_id_rcil_ctl 0x0006
#define PCI_extcap_id_rcec_assoc 0x0007
#define PCI_extcap_id_mfvc 0x0008
#define PCI_extcap_id_vc2 0x0009
#define PCI_extcap_id_rcrb_header 0x000a
#define PCI_extcap_id_vendor 0x000b
#define PCI_extcap_id_acs 0x000d
#define PCI_extcap_id_ari 0x000e
#define PCI_extcap_id_ats 0x000f
#define PCI_extcap_id_srio_virtual 0x0010
#define PCI_extcap_id_mrio_virtual 0x0011
#define PCI_extcap_id_multicast 0x0012
#define PCI_extcap_id_page_request 0x0013
#define PCI_extcap_id_amd 0x0014
#define PCI_extcap_id_resizable_bar 0x0015
#define PCI_extcap_id_dyn_power_alloc 0x0016
#define PCI_extcap_id_tph_requester 0x0017
#define PCI_extcap_id_latency_tolerance 0x0018
#define PCI_extcap_id_2ndpcie 0x0019
#define PCI_extcap_id_pmux 0x001a
#define PCI_extcap_id_pasid 0x001b
#define PCI_extcap_id_ln_requester 0x001c
#define PCI_extcap_id_dpc 0x001d
#define PCI_extcap_id_l1pm 0x001e
#define PCI_extcap_id_ptm 0x001f
#define PCI_extcap_id_m_pcie 0x0020
#define PCI_extcap_id_frs 0x0021
#define PCI_extcap_id_rtr 0x0022
#define PCI_extcap_id_dvsec 0x0023
#define PCI_extcap_id_vf_resizable_bar 0x0024
#define PCI_extcap_id_datalink 0x0025
#define PCI_extcap_id_16gt 0x0026
#define PCI_extcap_id_lmr 0x0027
#define PCI_extcap_id_hierarchy_id 0x0028
#define PCI_extcap_id_npem 0x0029
#define PCI_extcap_id_pl32 0x002a
#define PCI_extcap_id_ap 0x002b
#define PCI_extcap_id_sfi 0x002c
#define PCI_extcap_id_sf 0x002d
#define PCI_extcap_id_doe 0x002e
// Power Management Control Status Register settings
#define PCI_pm_mask 0x03
#define PCI_pm_ctrl 0x02
#define PCI_pm_d1supp 0x0200
#define PCI_pm_d2supp 0x0400
#define PCI_pm_status 0x04
#define PCI_pm_state_d0 0x00
#define PCI_pm_state_d1 0x01
#define PCI_pm_state_d2 0x02
#define PCI_pm_state_d3 0x03
// MSI registers
#define PCI_msi_control 0x02
#define PCI_msi_address 0x04
#define PCI_msi_address_high 0x08
#define PCI_msi_data 0x08
#define PCI_msi_data_64bit 0x0c
#define PCI_msi_mask 0x10
#define PCI_msi_pending 0x14
// MSI control register values
#define PCI_msi_control_enable 0x0001
#define PCI_msi_control_vector 0x0100
#define PCI_msi_control_64bit 0x0080
#define PCI_msi_control_mme_mask 0x0070
#define PCI_msi_control_mme_1 0x0000
#define PCI_msi_control_mme_2 0x0010
#define PCI_msi_control_mme_4 0x0020
#define PCI_msi_control_mme_8 0x0030
#define PCI_msi_control_mme_16 0x0040
#define PCI_msi_control_mme_32 0x0050
#define PCI_msi_control_mmc_mask 0x000e
#define PCI_msi_control_mmc_1 0x0000
#define PCI_msi_control_mmc_2 0x0002
#define PCI_msi_control_mmc_4 0x0004
#define PCI_msi_control_mmc_8 0x0006
#define PCI_msi_control_mmc_16 0x0008
#define PCI_msi_control_mmc_32 0x000a
// MSI-X registers
#define PCI_msix_control 0x02
#define PCI_msix_table 0x04
#define PCI_msix_pba 0x08
#define PCI_msix_control_table_size 0x07ff
#define PCI_msix_control_function_mask 0x4000
#define PCI_msix_control_enable 0x8000
#define PCI_msix_bir_mask 0x0007
#define PCI_msix_bir_0 0x10
#define PCI_msix_bir_1 0x14
#define PCI_msix_bir_2 0x18
#define PCI_msix_bir_3 0x1c
#define PCI_msix_bir_4 0x20
#define PCI_msix_bir_5 0x24
#define PCI_msix_offset_mask 0xfff8
#define PCI_msix_vctrl_mask 0x0001
// HyperTransport registers
#define PCI_ht_command 0x02
#define PCI_ht_msi_address_low 0x04
#define PCI_ht_msi_address_high 0x08
#define PCI_ht_command_cap_mask_3_bits 0xe000
#define PCI_ht_command_cap_mask_5_bits 0xf800
#define PCI_ht_command_cap_slave 0x0000
#define PCI_ht_command_cap_host 0x2000
#define PCI_ht_command_cap_switch 0x4000
#define PCI_ht_command_cap_interrupt 0x8000
#define PCI_ht_command_cap_revision_id 0x8800
#define PCI_ht_command_cap_unit_id_clumping 0x9000
#define PCI_ht_command_cap_ext_config_space 0x9800
#define PCI_ht_command_cap_address_mapping 0xa000
#define PCI_ht_command_cap_msi_mapping 0xa800
#define PCI_ht_command_cap_direct_route 0xb000
#define PCI_ht_command_cap_vcset 0xb800
#define PCI_ht_command_cap_retry_mode 0xc000
#define PCI_ht_command_cap_x86_encoding 0xc800
#define PCI_ht_command_cap_gen3 0xd000
#define PCI_ht_command_cap_fle 0xd800
#define PCI_ht_command_cap_pm 0xe000
#define PCI_ht_command_cap_high_node_count 0xe800
#define PCI_ht_command_msi_enable 0x0001
#define PCI_ht_command_msi_fixed 0x0002

#ifdef __cplusplus
}

#endif

#endif 	/* PCI_H */
