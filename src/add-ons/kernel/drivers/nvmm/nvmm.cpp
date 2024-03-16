/* Code of vmx_ident(), vmx_init_ctls(), vmx_check_ctls() and vmx_check_cr() is taken
   from DragonFlyBSD (/sys/dev/virtual/nvmm/x86/nvmm_x86_vmx.c)
   The macros and #defines are still from the NetBSD version (but they should be the same)

   Pretend this file was copied as it was from DragonFlyBSD. The only changes done are at
   the beginning of vmx_ident(), in the #ifdef __HAIKU__ and #if defined(__NetBSD) ||
   defined(__DragonFly__) code blocks. All other Haiku-specific code is put into nvmm_os.h
   and nvmm_haiku.cpp
 */
#include "nvmm_os.h"

#define NBBY 8 // Number of bits in a byte
// __BIT and __BITS taken from sys/cdefs.h (NetBSD)
#define __BIT(__n)      \
    (((uintmax_t)(__n) >= NBBY * sizeof(uintmax_t)) ? 0 : \
    ((uintmax_t)1 << (uintmax_t)((__n) & (NBBY * sizeof(uintmax_t) - 1))))

#define __BITS(__m, __n)        \
        ((__BIT(max_c((__m), (__n)) + 1) - 1) ^ (__BIT(min_c((__m), (__n))) - 1))

#define __LOWEST_SET_BIT(__mask) ((((__mask) - 1) & (__mask)) ^ (__mask))
#define __SHIFTOUT(__x, __mask) (((__x) & (__mask)) / __LOWEST_SET_BIT(__mask))

#define MSR_IA32_FEATURE_CONTROL	0x003A
#define		IA32_FEATURE_CONTROL_LOCK	__BIT(0)
#define		IA32_FEATURE_CONTROL_IN_SMX	__BIT(1)
#define		IA32_FEATURE_CONTROL_OUT_SMX	__BIT(2)

#define MSR_IA32_VMX_BASIC		0x0480
#define		IA32_VMX_BASIC_IDENT		__BITS(30,0)
#define		IA32_VMX_BASIC_DATA_SIZE	__BITS(44,32)
#define		IA32_VMX_BASIC_MEM_WIDTH	__BIT(48)
#define		IA32_VMX_BASIC_DUAL		__BIT(49)
#define		IA32_VMX_BASIC_MEM_TYPE		__BITS(53,50)
#define			MEM_TYPE_UC		0
#define			MEM_TYPE_WB		6
#define		IA32_VMX_BASIC_IO_REPORT	__BIT(54)
#define		IA32_VMX_BASIC_TRUE_CTLS	__BIT(55)

#define MSR_IA32_VMX_PINBASED_CTLS		0x0481
#define MSR_IA32_VMX_PROCBASED_CTLS		0x0482
#define MSR_IA32_VMX_EXIT_CTLS			0x0483
#define MSR_IA32_VMX_ENTRY_CTLS			0x0484
#define MSR_IA32_VMX_PROCBASED_CTLS2		0x048B

#define MSR_IA32_VMX_TRUE_PINBASED_CTLS		0x048D
#define MSR_IA32_VMX_TRUE_PROCBASED_CTLS	0x048E
#define MSR_IA32_VMX_TRUE_EXIT_CTLS		0x048F
#define MSR_IA32_VMX_TRUE_ENTRY_CTLS		0x0490

#define MSR_IA32_VMX_CR0_FIXED0			0x0486
#define MSR_IA32_VMX_CR0_FIXED1			0x0487
#define MSR_IA32_VMX_CR4_FIXED0			0x0488
#define MSR_IA32_VMX_CR4_FIXED1			0x0489

#define MSR_IA32_VMX_EPT_VPID_CAP	0x048C
#define		IA32_VMX_EPT_VPID_XO			__BIT(0)
#define		IA32_VMX_EPT_VPID_WALKLENGTH_4		__BIT(6)
#define		IA32_VMX_EPT_VPID_UC			__BIT(8)
#define		IA32_VMX_EPT_VPID_WB			__BIT(14)
#define		IA32_VMX_EPT_VPID_2MB			__BIT(16)
#define		IA32_VMX_EPT_VPID_1GB			__BIT(17)
#define		IA32_VMX_EPT_VPID_INVEPT		__BIT(20)
#define		IA32_VMX_EPT_VPID_FLAGS_AD		__BIT(21)
#define		IA32_VMX_EPT_VPID_ADVANCED_VMEXIT_INFO	__BIT(22)
#define		IA32_VMX_EPT_VPID_SHSTK			__BIT(23)
#define		IA32_VMX_EPT_VPID_INVEPT_CONTEXT	__BIT(25)
#define		IA32_VMX_EPT_VPID_INVEPT_ALL		__BIT(26)
#define		IA32_VMX_EPT_VPID_INVVPID		__BIT(32)
#define		IA32_VMX_EPT_VPID_INVVPID_ADDR		__BIT(40)
#define		IA32_VMX_EPT_VPID_INVVPID_CONTEXT	__BIT(41)
#define		IA32_VMX_EPT_VPID_INVVPID_ALL		__BIT(42)
#define		IA32_VMX_EPT_VPID_INVVPID_CONTEXT_NOG	__BIT(43)

/* -------------------------------------------------------------------------- */

/* 16-bit control fields */
#define VMCS_VPID				0x00000000
#define VMCS_PIR_VECTOR				0x00000002
#define VMCS_EPTP_INDEX				0x00000004
/* 16-bit guest-state fields */
#define VMCS_GUEST_ES_SELECTOR			0x00000800
#define VMCS_GUEST_CS_SELECTOR			0x00000802
#define VMCS_GUEST_SS_SELECTOR			0x00000804
#define VMCS_GUEST_DS_SELECTOR			0x00000806
#define VMCS_GUEST_FS_SELECTOR			0x00000808
#define VMCS_GUEST_GS_SELECTOR			0x0000080A
#define VMCS_GUEST_LDTR_SELECTOR		0x0000080C
#define VMCS_GUEST_TR_SELECTOR			0x0000080E
#define VMCS_GUEST_INTR_STATUS			0x00000810
#define VMCS_PML_INDEX				0x00000812
/* 16-bit host-state fields */
#define VMCS_HOST_ES_SELECTOR			0x00000C00
#define VMCS_HOST_CS_SELECTOR			0x00000C02
#define VMCS_HOST_SS_SELECTOR			0x00000C04
#define VMCS_HOST_DS_SELECTOR			0x00000C06
#define VMCS_HOST_FS_SELECTOR			0x00000C08
#define VMCS_HOST_GS_SELECTOR			0x00000C0A
#define VMCS_HOST_TR_SELECTOR			0x00000C0C
/* 64-bit control fields */
#define VMCS_IO_BITMAP_A			0x00002000
#define VMCS_IO_BITMAP_B			0x00002002
#define VMCS_MSR_BITMAP				0x00002004
#define VMCS_EXIT_MSR_STORE_ADDRESS		0x00002006
#define VMCS_EXIT_MSR_LOAD_ADDRESS		0x00002008
#define VMCS_ENTRY_MSR_LOAD_ADDRESS		0x0000200A
#define VMCS_EXECUTIVE_VMCS			0x0000200C
#define VMCS_PML_ADDRESS			0x0000200E
#define VMCS_TSC_OFFSET				0x00002010
#define VMCS_VIRTUAL_APIC			0x00002012
#define VMCS_APIC_ACCESS			0x00002014
#define VMCS_PIR_DESC				0x00002016
#define VMCS_VM_CONTROL				0x00002018
#define VMCS_EPTP				0x0000201A
#define		EPTP_TYPE			__BITS(2,0)
#define			EPTP_TYPE_UC		0
#define			EPTP_TYPE_WB		6
#define		EPTP_WALKLEN			__BITS(5,3)
#define		EPTP_FLAGS_AD			__BIT(6)
#define		EPTP_SSS			__BIT(7)
#define		EPTP_PHYSADDR			__BITS(63,12)
#define VMCS_EOI_EXIT0				0x0000201C
#define VMCS_EOI_EXIT1				0x0000201E
#define VMCS_EOI_EXIT2				0x00002020
#define VMCS_EOI_EXIT3				0x00002022
#define VMCS_EPTP_LIST				0x00002024
#define VMCS_VMREAD_BITMAP			0x00002026
#define VMCS_VMWRITE_BITMAP			0x00002028
#define VMCS_VIRTUAL_EXCEPTION			0x0000202A
#define VMCS_XSS_EXIT_BITMAP			0x0000202C
#define VMCS_ENCLS_EXIT_BITMAP			0x0000202E
#define VMCS_SUBPAGE_PERM_TABLE_PTR		0x00002030
#define VMCS_TSC_MULTIPLIER			0x00002032
#define VMCS_ENCLV_EXIT_BITMAP			0x00002036
/* 64-bit read-only fields */
#define VMCS_GUEST_PHYSICAL_ADDRESS		0x00002400
/* 64-bit guest-state fields */
#define VMCS_LINK_POINTER			0x00002800
#define VMCS_GUEST_IA32_DEBUGCTL		0x00002802
#define VMCS_GUEST_IA32_PAT			0x00002804
#define VMCS_GUEST_IA32_EFER			0x00002806
#define VMCS_GUEST_IA32_PERF_GLOBAL_CTRL	0x00002808
#define VMCS_GUEST_PDPTE0			0x0000280A
#define VMCS_GUEST_PDPTE1			0x0000280C
#define VMCS_GUEST_PDPTE2			0x0000280E
#define VMCS_GUEST_PDPTE3			0x00002810
#define VMCS_GUEST_BNDCFGS			0x00002812
#define VMCS_GUEST_RTIT_CTL			0x00002814
#define VMCS_GUEST_PKRS				0x00002818
/* 64-bit host-state fields */
#define VMCS_HOST_IA32_PAT			0x00002C00
#define VMCS_HOST_IA32_EFER			0x00002C02
#define VMCS_HOST_IA32_PERF_GLOBAL_CTRL		0x00002C04
#define VMCS_HOST_IA32_PKRS			0x00002C06
/* 32-bit control fields */
#define VMCS_PINBASED_CTLS			0x00004000
#define		PIN_CTLS_INT_EXITING		__BIT(0)
#define		PIN_CTLS_NMI_EXITING		__BIT(3)
#define		PIN_CTLS_VIRTUAL_NMIS		__BIT(5)
#define		PIN_CTLS_ACTIVATE_PREEMPT_TIMER	__BIT(6)
#define		PIN_CTLS_PROCESS_POSTED_INTS	__BIT(7)
#define VMCS_PROCBASED_CTLS			0x00004002
#define		PROC_CTLS_INT_WINDOW_EXITING	__BIT(2)
#define		PROC_CTLS_USE_TSC_OFFSETTING	__BIT(3)
#define		PROC_CTLS_HLT_EXITING		__BIT(7)
#define		PROC_CTLS_INVLPG_EXITING	__BIT(9)
#define		PROC_CTLS_MWAIT_EXITING		__BIT(10)
#define		PROC_CTLS_RDPMC_EXITING		__BIT(11)
#define		PROC_CTLS_RDTSC_EXITING		__BIT(12)
#define		PROC_CTLS_RCR3_EXITING		__BIT(15)
#define		PROC_CTLS_LCR3_EXITING		__BIT(16)
#define		PROC_CTLS_RCR8_EXITING		__BIT(19)
#define		PROC_CTLS_LCR8_EXITING		__BIT(20)
#define		PROC_CTLS_USE_TPR_SHADOW	__BIT(21)
#define		PROC_CTLS_NMI_WINDOW_EXITING	__BIT(22)
#define		PROC_CTLS_DR_EXITING		__BIT(23)
#define		PROC_CTLS_UNCOND_IO_EXITING	__BIT(24)
#define		PROC_CTLS_USE_IO_BITMAPS	__BIT(25)
#define		PROC_CTLS_MONITOR_TRAP_FLAG	__BIT(27)
#define		PROC_CTLS_USE_MSR_BITMAPS	__BIT(28)
#define		PROC_CTLS_MONITOR_EXITING	__BIT(29)
#define		PROC_CTLS_PAUSE_EXITING		__BIT(30)
#define		PROC_CTLS_ACTIVATE_CTLS2	__BIT(31)
#define VMCS_EXCEPTION_BITMAP			0x00004004
#define VMCS_PF_ERROR_MASK			0x00004006
#define VMCS_PF_ERROR_MATCH			0x00004008
#define VMCS_CR3_TARGET_COUNT			0x0000400A
#define VMCS_EXIT_CTLS				0x0000400C
#define		EXIT_CTLS_SAVE_DEBUG_CONTROLS	__BIT(2)
#define		EXIT_CTLS_HOST_LONG_MODE	__BIT(9)
#define		EXIT_CTLS_LOAD_PERFGLOBALCTRL	__BIT(12)
#define		EXIT_CTLS_ACK_INTERRUPT		__BIT(15)
#define		EXIT_CTLS_SAVE_PAT		__BIT(18)
#define		EXIT_CTLS_LOAD_PAT		__BIT(19)
#define		EXIT_CTLS_SAVE_EFER		__BIT(20)
#define		EXIT_CTLS_LOAD_EFER		__BIT(21)
#define		EXIT_CTLS_SAVE_PREEMPT_TIMER	__BIT(22)
#define		EXIT_CTLS_CLEAR_BNDCFGS		__BIT(23)
#define		EXIT_CTLS_CONCEAL_PT		__BIT(24)
#define		EXIT_CTLS_CLEAR_RTIT_CTL	__BIT(25)
#define		EXIT_CTLS_LOAD_CET		__BIT(28)
#define		EXIT_CTLS_LOAD_PKRS		__BIT(29)
#define VMCS_EXIT_MSR_STORE_COUNT		0x0000400E
#define VMCS_EXIT_MSR_LOAD_COUNT		0x00004010
#define VMCS_ENTRY_CTLS				0x00004012
#define		ENTRY_CTLS_LOAD_DEBUG_CONTROLS	__BIT(2)
#define		ENTRY_CTLS_LONG_MODE		__BIT(9)
#define		ENTRY_CTLS_SMM			__BIT(10)
#define		ENTRY_CTLS_DISABLE_DUAL		__BIT(11)
#define		ENTRY_CTLS_LOAD_PERFGLOBALCTRL	__BIT(13)
#define		ENTRY_CTLS_LOAD_PAT		__BIT(14)
#define		ENTRY_CTLS_LOAD_EFER		__BIT(15)
#define		ENTRY_CTLS_LOAD_BNDCFGS		__BIT(16)
#define		ENTRY_CTLS_CONCEAL_PT		__BIT(17)
#define		ENTRY_CTLS_LOAD_RTIT_CTL	__BIT(18)
#define		ENTRY_CTLS_LOAD_CET		__BIT(20)
#define		ENTRY_CTLS_LOAD_PKRS		__BIT(22)
#define VMCS_ENTRY_MSR_LOAD_COUNT		0x00004014
#define VMCS_ENTRY_INTR_INFO			0x00004016
#define		INTR_INFO_VECTOR		__BITS(7,0)
#define		INTR_INFO_TYPE			__BITS(10,8)
#define			INTR_TYPE_EXT_INT	0
#define			INTR_TYPE_NMI		2
#define			INTR_TYPE_HW_EXC	3
#define			INTR_TYPE_SW_INT	4
#define			INTR_TYPE_PRIV_SW_EXC	5
#define			INTR_TYPE_SW_EXC	6
#define			INTR_TYPE_OTHER		7
#define		INTR_INFO_ERROR			__BIT(11)
#define		INTR_INFO_VALID			__BIT(31)
#define VMCS_ENTRY_EXCEPTION_ERROR		0x00004018
#define VMCS_ENTRY_INSTRUCTION_LENGTH		0x0000401A
#define VMCS_TPR_THRESHOLD			0x0000401C
#define VMCS_PROCBASED_CTLS2			0x0000401E
#define		PROC_CTLS2_VIRT_APIC_ACCESSES	__BIT(0)
#define		PROC_CTLS2_ENABLE_EPT		__BIT(1)
#define		PROC_CTLS2_DESC_TABLE_EXITING	__BIT(2)
#define		PROC_CTLS2_ENABLE_RDTSCP	__BIT(3)
#define		PROC_CTLS2_VIRT_X2APIC		__BIT(4)
#define		PROC_CTLS2_ENABLE_VPID		__BIT(5)
#define		PROC_CTLS2_WBINVD_EXITING	__BIT(6)
#define		PROC_CTLS2_UNRESTRICTED_GUEST	__BIT(7)
#define		PROC_CTLS2_APIC_REG_VIRT	__BIT(8)
#define		PROC_CTLS2_VIRT_INT_DELIVERY	__BIT(9)
#define		PROC_CTLS2_PAUSE_LOOP_EXITING	__BIT(10)
#define		PROC_CTLS2_RDRAND_EXITING	__BIT(11)
#define		PROC_CTLS2_INVPCID_ENABLE	__BIT(12)
#define		PROC_CTLS2_VMFUNC_ENABLE	__BIT(13)
#define		PROC_CTLS2_VMCS_SHADOWING	__BIT(14)
#define		PROC_CTLS2_ENCLS_EXITING	__BIT(15)
#define		PROC_CTLS2_RDSEED_EXITING	__BIT(16)
#define		PROC_CTLS2_PML_ENABLE		__BIT(17)
#define		PROC_CTLS2_EPT_VIOLATION	__BIT(18)
#define		PROC_CTLS2_CONCEAL_VMX_FROM_PT	__BIT(19)
#define		PROC_CTLS2_XSAVES_ENABLE	__BIT(20)
#define		PROC_CTLS2_MODE_BASED_EXEC_EPT	__BIT(22)
#define		PROC_CTLS2_SUBPAGE_PERMISSIONS	__BIT(23)
#define		PROC_CTLS2_PT_USES_GPA		__BIT(24)
#define		PROC_CTLS2_USE_TSC_SCALING	__BIT(25)
#define		PROC_CTLS2_WAIT_PAUSE_ENABLE	__BIT(26)
#define		PROC_CTLS2_ENCLV_EXITING	__BIT(28)
#define VMCS_PLE_GAP				0x00004020
#define VMCS_PLE_WINDOW				0x00004022
/* 32-bit read-only data fields */
#define VMCS_INSTRUCTION_ERROR			0x00004400
#define VMCS_EXIT_REASON			0x00004402
#define VMCS_EXIT_INTR_INFO			0x00004404
#define VMCS_EXIT_INTR_ERRCODE			0x00004406
#define VMCS_IDT_VECTORING_INFO			0x00004408
#define VMCS_IDT_VECTORING_ERROR		0x0000440A
#define VMCS_EXIT_INSTRUCTION_LENGTH		0x0000440C
#define VMCS_EXIT_INSTRUCTION_INFO		0x0000440E
/* 32-bit guest-state fields */
#define VMCS_GUEST_ES_LIMIT			0x00004800
#define VMCS_GUEST_CS_LIMIT			0x00004802
#define VMCS_GUEST_SS_LIMIT			0x00004804
#define VMCS_GUEST_DS_LIMIT			0x00004806
#define VMCS_GUEST_FS_LIMIT			0x00004808
#define VMCS_GUEST_GS_LIMIT			0x0000480A
#define VMCS_GUEST_LDTR_LIMIT			0x0000480C
#define VMCS_GUEST_TR_LIMIT			0x0000480E
#define VMCS_GUEST_GDTR_LIMIT			0x00004810
#define VMCS_GUEST_IDTR_LIMIT			0x00004812
#define VMCS_GUEST_ES_ACCESS_RIGHTS		0x00004814
#define VMCS_GUEST_CS_ACCESS_RIGHTS		0x00004816
#define VMCS_GUEST_SS_ACCESS_RIGHTS		0x00004818
#define VMCS_GUEST_DS_ACCESS_RIGHTS		0x0000481A
#define VMCS_GUEST_FS_ACCESS_RIGHTS		0x0000481C
#define VMCS_GUEST_GS_ACCESS_RIGHTS		0x0000481E
#define VMCS_GUEST_LDTR_ACCESS_RIGHTS		0x00004820
#define VMCS_GUEST_TR_ACCESS_RIGHTS		0x00004822
#define VMCS_GUEST_INTERRUPTIBILITY		0x00004824
#define		INT_STATE_STI			__BIT(0)
#define		INT_STATE_MOVSS			__BIT(1)
#define		INT_STATE_SMI			__BIT(2)
#define		INT_STATE_NMI			__BIT(3)
#define		INT_STATE_ENCLAVE		__BIT(4)
#define VMCS_GUEST_ACTIVITY			0x00004826
#define VMCS_GUEST_SMBASE			0x00004828
#define VMCS_GUEST_IA32_SYSENTER_CS		0x0000482A
#define VMCS_PREEMPTION_TIMER_VALUE		0x0000482E
/* 32-bit host state fields */
#define VMCS_HOST_IA32_SYSENTER_CS		0x00004C00
/* Natural-Width control fields */
#define VMCS_CR0_MASK				0x00006000
#define VMCS_CR4_MASK				0x00006002
#define VMCS_CR0_SHADOW				0x00006004
#define VMCS_CR4_SHADOW				0x00006006
#define VMCS_CR3_TARGET0			0x00006008
#define VMCS_CR3_TARGET1			0x0000600A
#define VMCS_CR3_TARGET2			0x0000600C
#define VMCS_CR3_TARGET3			0x0000600E
/* Natural-Width read-only fields */
#define VMCS_EXIT_QUALIFICATION			0x00006400
#define VMCS_IO_RCX				0x00006402
#define VMCS_IO_RSI				0x00006404
#define VMCS_IO_RDI				0x00006406
#define VMCS_IO_RIP				0x00006408
#define VMCS_GUEST_LINEAR_ADDRESS		0x0000640A
/* Natural-Width guest-state fields */
#define VMCS_GUEST_CR0				0x00006800
#define VMCS_GUEST_CR3				0x00006802
#define VMCS_GUEST_CR4				0x00006804
#define VMCS_GUEST_ES_BASE			0x00006806
#define VMCS_GUEST_CS_BASE			0x00006808
#define VMCS_GUEST_SS_BASE			0x0000680A
#define VMCS_GUEST_DS_BASE			0x0000680C
#define VMCS_GUEST_FS_BASE			0x0000680E
#define VMCS_GUEST_GS_BASE			0x00006810
#define VMCS_GUEST_LDTR_BASE			0x00006812
#define VMCS_GUEST_TR_BASE			0x00006814
#define VMCS_GUEST_GDTR_BASE			0x00006816
#define VMCS_GUEST_IDTR_BASE			0x00006818
#define VMCS_GUEST_DR7				0x0000681A
#define VMCS_GUEST_RSP				0x0000681C
#define VMCS_GUEST_RIP				0x0000681E
#define VMCS_GUEST_RFLAGS			0x00006820
#define VMCS_GUEST_PENDING_DBG_EXCEPTIONS	0x00006822
#define VMCS_GUEST_IA32_SYSENTER_ESP		0x00006824
#define VMCS_GUEST_IA32_SYSENTER_EIP		0x00006826
#define VMCS_GUEST_IA32_S_CET			0x00006828
#define VMCS_GUEST_SSP				0x0000682A
#define VMCS_GUEST_IA32_INTR_SSP_TABLE		0x0000682C
/* Natural-Width host-state fields */
#define VMCS_HOST_CR0				0x00006C00
#define VMCS_HOST_CR3				0x00006C02
#define VMCS_HOST_CR4				0x00006C04
#define VMCS_HOST_FS_BASE			0x00006C06
#define VMCS_HOST_GS_BASE			0x00006C08
#define VMCS_HOST_TR_BASE			0x00006C0A
#define VMCS_HOST_GDTR_BASE			0x00006C0C
#define VMCS_HOST_IDTR_BASE			0x00006C0E
#define VMCS_HOST_IA32_SYSENTER_ESP		0x00006C10
#define VMCS_HOST_IA32_SYSENTER_EIP		0x00006C12
#define VMCS_HOST_RSP				0x00006C14
#define VMCS_HOST_RIP				0x00006C16
#define VMCS_HOST_IA32_S_CET			0x00006C18
#define VMCS_HOST_SSP				0x00006C1A
#define VMCS_HOST_IA32_INTR_SSP_TABLE		0x00006C1C

static bool vmx_ept_has_ad;

static uint64_t vmx_pinbased_ctls;
static uint64_t vmx_procbased_ctls;
static uint64_t vmx_procbased_ctls2;
static uint64_t vmx_entry_ctls;
static uint64_t vmx_exit_ctls;

static uint64_t vmx_cr0_fixed0;
static uint64_t vmx_cr0_fixed1;
static uint64_t vmx_cr4_fixed0;
static uint64_t vmx_cr4_fixed1;

#define VMX_PINBASED_CTLS_ONE	\
	(PIN_CTLS_INT_EXITING| \
	 PIN_CTLS_NMI_EXITING| \
	 PIN_CTLS_VIRTUAL_NMIS)

#define VMX_PINBASED_CTLS_ZERO	0

#define VMX_PROCBASED_CTLS_ONE	\
	(PROC_CTLS_USE_TSC_OFFSETTING| \
	 PROC_CTLS_HLT_EXITING| \
	 PROC_CTLS_MWAIT_EXITING | \
	 PROC_CTLS_RDPMC_EXITING | \
	 PROC_CTLS_RCR8_EXITING | \
	 PROC_CTLS_LCR8_EXITING | \
	 PROC_CTLS_UNCOND_IO_EXITING | /* no I/O bitmap */ \
	 PROC_CTLS_USE_MSR_BITMAPS | \
	 PROC_CTLS_MONITOR_EXITING | \
	 PROC_CTLS_ACTIVATE_CTLS2)

#define VMX_PROCBASED_CTLS_ZERO	\
	(PROC_CTLS_RCR3_EXITING| \
	 PROC_CTLS_LCR3_EXITING)

#define VMX_PROCBASED_CTLS2_ONE	\
	(PROC_CTLS2_ENABLE_EPT| \
	 PROC_CTLS2_ENABLE_VPID| \
	 PROC_CTLS2_UNRESTRICTED_GUEST)

#define VMX_PROCBASED_CTLS2_ZERO	0

#define VMX_ENTRY_CTLS_ONE	\
	(ENTRY_CTLS_LOAD_DEBUG_CONTROLS| \
	 ENTRY_CTLS_LOAD_EFER| \
	 ENTRY_CTLS_LOAD_PAT)

#define VMX_ENTRY_CTLS_ZERO	\
	(ENTRY_CTLS_SMM| \
	 ENTRY_CTLS_DISABLE_DUAL)

#define VMX_EXIT_CTLS_ONE	\
	(EXIT_CTLS_SAVE_DEBUG_CONTROLS| \
	 EXIT_CTLS_HOST_LONG_MODE| \
	 EXIT_CTLS_SAVE_PAT| \
	 EXIT_CTLS_LOAD_PAT| \
	 EXIT_CTLS_SAVE_EFER| \
	 EXIT_CTLS_LOAD_EFER)

#define VMX_EXIT_CTLS_ZERO	0

static inline int
vmx_check_cr(uint64_t crval, uint64_t fixed0, uint64_t fixed1)
{
	/* Bits set to 1 in fixed0 are fixed to 1. */
	if ((crval & fixed0) != fixed0) {
		return -1;
	}
	/* Bits set to 0 in fixed1 are fixed to 0. */
	if (crval & ~fixed1) {
		return -1;
	}
	return 0;
}

#define CTLS_ONE_ALLOWED(msrval, bitoff) \
	((msrval & __BIT(32 + bitoff)) != 0)
#define CTLS_ZERO_ALLOWED(msrval, bitoff) \
	((msrval & __BIT(bitoff)) == 0)

static int
vmx_check_ctls(uint64_t msr_ctls, uint64_t msr_true_ctls, uint64_t set_one)
{
	uint64_t basic, val, true_val;
	bool has_true;
	size_t i;

	basic = rdmsr(MSR_IA32_VMX_BASIC);
	has_true = (basic & IA32_VMX_BASIC_TRUE_CTLS) != 0;

	val = rdmsr(msr_ctls);
	if (has_true) {
		true_val = rdmsr(msr_true_ctls);
	} else {
		true_val = val;
	}

	for (i = 0; i < 32; i++) {
		if (!(set_one & __BIT(i))) {
			continue;
		}
		if (!CTLS_ONE_ALLOWED(true_val, i)) {
			return -1;
		}
	}

	return 0;
}

static int
vmx_init_ctls(uint64_t msr_ctls, uint64_t msr_true_ctls,
    uint64_t set_one, uint64_t set_zero, uint64_t *res)
{
	uint64_t basic, val, true_val;
	bool one_allowed, zero_allowed, has_true;
	size_t i;

	basic = rdmsr(MSR_IA32_VMX_BASIC);
	has_true = (basic & IA32_VMX_BASIC_TRUE_CTLS) != 0;

	val = rdmsr(msr_ctls);
	if (has_true) {
		true_val = rdmsr(msr_true_ctls);
	} else {
		true_val = val;
	}

	for (i = 0; i < 32; i++) {
		one_allowed = CTLS_ONE_ALLOWED(true_val, i);
		zero_allowed = CTLS_ZERO_ALLOWED(true_val, i);

		if (zero_allowed && !one_allowed) {
			if (set_one & __BIT(i))
				return -1;
			*res &= ~__BIT(i);
		} else if (one_allowed && !zero_allowed) {
			if (set_zero & __BIT(i))
				return -1;
			*res |= __BIT(i);
		} else {
			if (set_zero & __BIT(i)) {
				*res &= ~__BIT(i);
			} else if (set_one & __BIT(i)) {
				*res |= __BIT(i);
			} else if (!has_true) {
				*res &= ~__BIT(i);
			} else if (CTLS_ZERO_ALLOWED(val, i)) {
				*res &= ~__BIT(i);
			} else if (CTLS_ONE_ALLOWED(val, i)) {
				*res |= __BIT(i);
			} else {
				return -1;
			}
		}
	}

	return 0;
}

bool
vmx_ident(void)
{
#if defined(__NetBSD__) || defined(__DragonFly__)
	cpuid_desc_t descs;
#endif
	uint64_t msr;
	int ret;

#if defined(__NetBSD__) || defined(__DragonFly__)
	x86_get_cpuid(0x00000001, &descs);
	if (!(descs.ecx & CPUID_0_01_ECX_VMX)) {
		return false;
	}
#elif __HAIKU__
	cpu_ent *cpu = get_cpu_struct();

	if (!(cpu->arch.feature[FEATURE_EXT] & IA32_FEATURE_EXT_VMX)) {
		os_printf("nvmm: VMX not supported\n");
		return false;
	}
#endif

	msr = rdmsr(MSR_IA32_FEATURE_CONTROL);
	if ((msr & IA32_FEATURE_CONTROL_LOCK) != 0 &&
	    (msr & IA32_FEATURE_CONTROL_OUT_SMX) == 0) {
		os_printf("nvmm: VMX disabled in BIOS\n");
		return false;
	}

	msr = rdmsr(MSR_IA32_VMX_BASIC);
	if ((msr & IA32_VMX_BASIC_IO_REPORT) == 0) {
		os_printf("nvmm: I/O reporting not supported\n");
		return false;
	}
	if (__SHIFTOUT(msr, IA32_VMX_BASIC_MEM_TYPE) != MEM_TYPE_WB) {
		os_printf("nvmm: WB memory not supported\n");
		return false;
	}

	/* PG and PE are reported, even if Unrestricted Guests is supported. */
	vmx_cr0_fixed0 = rdmsr(MSR_IA32_VMX_CR0_FIXED0) & ~(CR0_PG|CR0_PE);
	vmx_cr0_fixed1 = rdmsr(MSR_IA32_VMX_CR0_FIXED1) | (CR0_PG|CR0_PE);
	ret = vmx_check_cr(x86_get_cr0(), vmx_cr0_fixed0, vmx_cr0_fixed1);
	if (ret == -1) {
		os_printf("nvmm: CR0 requirements not satisfied\n");
		return false;
	}

	vmx_cr4_fixed0 = rdmsr(MSR_IA32_VMX_CR4_FIXED0);
	vmx_cr4_fixed1 = rdmsr(MSR_IA32_VMX_CR4_FIXED1);
	ret = vmx_check_cr(x86_get_cr4() | CR4_VMXE, vmx_cr4_fixed0,
	    vmx_cr4_fixed1);
	if (ret == -1) {
		os_printf("nvmm: CR4 requirements not satisfied\n");
		return false;
	}

	/* Init the CTLSs right now, and check for errors. */
	ret = vmx_init_ctls(
	    MSR_IA32_VMX_PINBASED_CTLS, MSR_IA32_VMX_TRUE_PINBASED_CTLS,
	    VMX_PINBASED_CTLS_ONE, VMX_PINBASED_CTLS_ZERO,
	    &vmx_pinbased_ctls);
	if (ret == -1) {
		os_printf("nvmm: pin-based-ctls requirements not satisfied\n");
		return false;
	}
	ret = vmx_init_ctls(
	    MSR_IA32_VMX_PROCBASED_CTLS, MSR_IA32_VMX_TRUE_PROCBASED_CTLS,
	    VMX_PROCBASED_CTLS_ONE, VMX_PROCBASED_CTLS_ZERO,
	    &vmx_procbased_ctls);
	if (ret == -1) {
		os_printf("nvmm: proc-based-ctls requirements not satisfied\n");
		return false;
	}
	ret = vmx_init_ctls(
	    MSR_IA32_VMX_PROCBASED_CTLS2, MSR_IA32_VMX_PROCBASED_CTLS2,
	    VMX_PROCBASED_CTLS2_ONE, VMX_PROCBASED_CTLS2_ZERO,
	    &vmx_procbased_ctls2);
	if (ret == -1) {
		os_printf("nvmm: proc-based-ctls2 requirements not satisfied\n");
		return false;
	}
	ret = vmx_check_ctls(
	    MSR_IA32_VMX_PROCBASED_CTLS2, MSR_IA32_VMX_PROCBASED_CTLS2,
	    PROC_CTLS2_INVPCID_ENABLE);
	if (ret != -1) {
		vmx_procbased_ctls2 |= PROC_CTLS2_INVPCID_ENABLE;
	}
	ret = vmx_init_ctls(
	    MSR_IA32_VMX_ENTRY_CTLS, MSR_IA32_VMX_TRUE_ENTRY_CTLS,
	    VMX_ENTRY_CTLS_ONE, VMX_ENTRY_CTLS_ZERO,
	    &vmx_entry_ctls);
	if (ret == -1) {
		os_printf("nvmm: entry-ctls requirements not satisfied\n");
		return false;
	}
	ret = vmx_init_ctls(
	    MSR_IA32_VMX_EXIT_CTLS, MSR_IA32_VMX_TRUE_EXIT_CTLS,
	    VMX_EXIT_CTLS_ONE, VMX_EXIT_CTLS_ZERO,
	    &vmx_exit_ctls);
	if (ret == -1) {
		os_printf("nvmm: exit-ctls requirements not satisfied\n");
		return false;
	}

	msr = rdmsr(MSR_IA32_VMX_EPT_VPID_CAP);
	if ((msr & IA32_VMX_EPT_VPID_WALKLENGTH_4) == 0) {
		os_printf("nvmm: 4-level page tree not supported\n");
		return false;
	}
	if ((msr & IA32_VMX_EPT_VPID_INVEPT) == 0) {
		os_printf("nvmm: INVEPT not supported\n");
		return false;
	}
	if ((msr & IA32_VMX_EPT_VPID_INVVPID) == 0) {
		os_printf("nvmm: INVVPID not supported\n");
		return false;
	}
	if ((msr & IA32_VMX_EPT_VPID_FLAGS_AD) != 0) {
		vmx_ept_has_ad = true;
	} else {
		vmx_ept_has_ad = false;
	}
#ifdef __NetBSD__
	pmap_ept_has_ad = vmx_ept_has_ad;
#endif
	if (!(msr & IA32_VMX_EPT_VPID_UC) && !(msr & IA32_VMX_EPT_VPID_WB)) {
		os_printf("nvmm: EPT UC/WB memory types not supported\n");
		return false;
	}

	return true;
}

