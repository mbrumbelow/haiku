/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Copyright 2024, Enrique Medina Gremaldos, quique@necos.es.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _DW_I2C_HARDWARE_H
#define _DW_I2C_HARDWARE_H


#define DW_CLK_STANDARD		100000
#define DW_CLK_FAST			400000
#define DW_CLK_HIGH			1000000


#define DW_IC_CON				0x00
#define DW_IC_TAR				0x04
#define DW_IC_SAR				0x08
#define DW_IC_DATA_CMD			0x10
#define DW_IC_SS_SCL_HCNT		0x14
#define DW_IC_SS_SCL_LCNT		0x18
#define DW_IC_FS_SCL_HCNT		0x1c
#define DW_IC_FS_SCL_LCNT		0x20
#define DW_IC_HS_SCL_HCNT		0x24
#define DW_IC_HS_SCL_LCNT		0x28
#define DW_IC_INTR_STAT			0x2c
#define DW_IC_INTR_MASK			0x30
#define DW_IC_RX_TL				0x38
#define DW_IC_TX_TL				0x3c
#define DW_IC_CLR_INTR			0x40
#define DW_IC_CLR_RX_UNDER		0x44
#define DW_IC_CLR_RX_OVER		0x48
#define DW_IC_CLR_TX_OVER		0x4c
#define DW_IC_CLR_RD_REQ		0x50
#define DW_IC_CLR_TX_ABRT		0x54
#define DW_IC_CLR_RX_DONE		0x58
#define DW_IC_CLR_ACTIVITY		0x5c
#define DW_IC_CLR_STOP_DET		0x60
#define DW_IC_CLR_START_DET		0x64
#define DW_IC_CLR_GEN_CALL		0x68


#define DW_IC_ENABLE			0x6c
#define DW_IC_STATUS			0x70
#define DW_IC_TXFLR				0x74
#define DW_IC_RXFLR				0x78
#define DW_IC_SDA_HOLD			0x7c
#define DW_IC_ENABLE_STATUS		0x9c
#define DW_IC_FS_SPKLEN			0xa0
#define DW_IC_COMP_PARAM1		0xf4
#define DW_IC_COMP_VERSION		0xf8
#define DW_IC_COMP_TYPE			0xfc

#define DW_IC_COMP_PARAM1_RX(x)	(1 + (((x) >> 8) & 0xff))
#define DW_IC_COMP_PARAM1_TX(x)	(1 + (((x) >> 16) & 0xff))


#define DW_IC_CON_MASTER				(1 << 0)
#define DW_IC_CON_SPEED_STANDARD			0x1
#define DW_IC_CON_SPEED_FAST				0x2
#define DW_IC_CON_SPEED_FAST_PLUS			0x2
#define DW_IC_CON_SPEED_HIGH				0x3
#define DW_IC_CON_10BITADDR_SLAVE		(1 << 3)
#define DW_IC_CON_10BITADDR_MASTER		(1 << 4)
#define DW_IC_CON_RESTART_EN			(1 << 5)
#define DW_IC_CON_SLAVE_DISABLE			(1 << 6)

#define DW_IC_DATA_CMD_READ				(1 << 8)
#define DW_IC_DATA_CMD_STOP				(1 << 9)
#define DW_IC_DATA_CMD_RESTART			(1 << 10)

#define DW_IC_INTR_STAT_RX_UNDER		(1 << 0)
#define DW_IC_INTR_STAT_RX_OVER			(1 << 1)
#define DW_IC_INTR_STAT_RX_FULL			(1 << 2)
#define DW_IC_INTR_STAT_TX_OVER			(1 << 3)
#define DW_IC_INTR_STAT_TX_EMPTY		(1 << 4)
#define DW_IC_INTR_STAT_RD_REQ			(1 << 5)
#define DW_IC_INTR_STAT_TX_ABRT			(1 << 6)
#define DW_IC_INTR_STAT_RX_DONE			(1 << 7)
#define DW_IC_INTR_STAT_ACTIVITY		(1 << 8)
#define DW_IC_INTR_STAT_STOP_DET		(1 << 9)
#define DW_IC_INTR_STAT_START_DET		(1 << 10)
#define DW_IC_INTR_STAT_GEN_CALL		(1 << 11)
#define DW_IC_INTR_STAT_RESTART_DET		(1 << 12)
#define DW_IC_INTR_STAT_MST_ON_HOLD		(1 << 13)

#define DW_IC_STATUS_MASTER_ACTIVITY	(1 << 5)



#endif // _DW_I2C_HARDWARE_H
