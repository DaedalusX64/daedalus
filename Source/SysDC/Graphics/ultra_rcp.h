/*
Copyright (C) 2001 StrmnNrmn

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef __ULTRA_RCP_H__
#define __ULTRA_RCP_H__

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//                   SP Memory                     //
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////


#define SP_DMEM_START		0x04000000	// read/write
#define SP_DMEM_END			0x04000FFF
#define SP_IMEM_START		0x04001000	// read/write 
#define SP_IMEM_END			0x04001FFF


/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//                SP CP0 Registers                 //
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

#define SP_BASE_REG		0x04040000

// SP memory address (R/W): [11:0] DMEM/IMEM address; [12] 0=DMEM,1=IMEM 
#define SP_MEM_ADDR_REG		(SP_BASE_REG+0x00)	// Master 
						
// SP DRAM DMA address (R/W): [23:0] RDRAM address 
#define SP_DRAM_ADDR_REG	(SP_BASE_REG+0x04)	// Slave 

// SP read DMA length (R/W): [11:0] length, [19:12] count, [31:20] skip 
// direction: I/DMEM <- RDRAM 
#define SP_RD_LEN_REG		(SP_BASE_REG+0x08)	// R/W: read len 

// SP write DMA length (R/W): [11:0] length, [19:12] count, [31:20] skip 
// direction: I/DMEM -> RDRAM 
#define SP_WR_LEN_REG		(SP_BASE_REG+0x0C)	// R/W: write len 

// SP status (R/W): [14:0] valid bits; see below for write/read mode 
#define SP_STATUS_REG		(SP_BASE_REG+0x10)

// SP DMA full (R): [0] valid bit; dma full 
#define SP_DMA_FULL_REG		(SP_BASE_REG+0x14)

// SP DMA busy (R): [0] valid bit; dma busy 
#define SP_DMA_BUSY_REG		(SP_BASE_REG+0x18)

// SP semaphore (R/W): Read:  [0] semaphore flag (set on read) 
//                     Write: [] clear semaphore flag 
#define SP_SEMAPHORE_REG	(SP_BASE_REG+0x1C)


#define SP_LAST_REG SP_SEMAPHORE_REG

// SP PC (R/W): [11:0] program counter 
#define SP_PC_REG		0x04080000

// SP MEM address: bit 12 specifies if address is IMEM or DMEM 
#define SP_DMA_DMEM		0x0000		// Bit 12: 0=DMEM, 1=IMEM 
#define SP_DMA_IMEM		0x1000		// Bit 12: 0=DMEM, 1=IMEM 

//
// Values to clear/set bit in status reg (SP_STATUS_REG - write)
//
#define SP_CLR_HALT			0x00001	    // Bit  0: clear halt 
#define SP_SET_HALT			0x00002	    // Bit  1: set halt 
#define SP_CLR_BROKE		0x00004	    // Bit  2: clear broke 
#define SP_CLR_INTR			0x00008	    // Bit  3: clear intr 
#define SP_SET_INTR			0x00010	    // Bit  4: set intr 
#define SP_CLR_SSTEP		0x00020	    // Bit  5: clear sstep 
#define SP_SET_SSTEP		0x00040	    // Bit  6: set sstep 
#define SP_CLR_INTR_BREAK	0x00080	    // Bit  7: clear intr on break 
#define SP_SET_INTR_BREAK	0x00100	    // Bit  8: set intr on break 
#define SP_CLR_SIG0			0x00200	    // Bit  9: clear signal 0 
#define SP_SET_SIG0			0x00400	    // Bit 10: set signal 0 
#define SP_CLR_SIG1			0x00800	    // Bit 11: clear signal 1 
#define SP_SET_SIG1			0x01000	    // Bit 12: set signal 1 
#define SP_CLR_SIG2			0x02000	    // Bit 13: clear signal 2 
#define SP_SET_SIG2			0x04000	    // Bit 14: set signal 2 
#define SP_CLR_SIG3			0x08000	    // Bit 15: clear signal 3 
#define SP_SET_SIG3			0x10000	    // Bit 16: set signal 3 
#define SP_CLR_SIG4			0x20000	    // Bit 17: clear signal 4 
#define SP_SET_SIG4			0x40000	    // Bit 18: set signal 4 
#define SP_CLR_SIG5			0x80000	    // Bit 19: clear signal 5 
#define SP_SET_SIG5		   0x100000	    // Bit 20: set signal 5 
#define SP_CLR_SIG6	       0x200000	    // Bit 21: clear signal 6 
#define SP_SET_SIG6	       0x400000	    // Bit 22: set signal 6 
#define SP_CLR_SIG7	       0x800000	    // Bit 23: clear signal 7 
#define SP_SET_SIG7	      0x1000000	    // Bit 24: set signal 7 

//
// Patterns to interpret status reg (SP_STATUS_REG - read)
//
#define SP_STATUS_HALT			0x001		// Bit  0: halt 
#define SP_STATUS_BROKE			0x002		// Bit  1: broke 
#define SP_STATUS_DMA_BUSY		0x004		// Bit  2: dma busy 
#define SP_STATUS_DMA_FULL		0x008		// Bit  3: dma full 
#define SP_STATUS_IO_FULL		0x010		// Bit  4: io full 
#define SP_STATUS_SSTEP			0x020		// Bit  5: single step 
#define SP_STATUS_INTR_BREAK	0x040		// Bit  6: interrupt on break 
#define SP_STATUS_SIG0			0x080		// Bit  7: signal 0 set 
#define SP_STATUS_SIG1			0x100		// Bit  8: signal 1 set 
#define SP_STATUS_SIG2			0x200		// Bit  9: signal 2 set 
#define SP_STATUS_SIG3			0x400		// Bit 10: signal 3 set 
#define SP_STATUS_SIG4			0x800		// Bit 11: signal 4 set 
#define SP_STATUS_SIG5	       0x1000		// Bit 12: signal 5 set 
#define SP_STATUS_SIG6	       0x2000		// Bit 13: signal 6 set 
#define SP_STATUS_SIG7	       0x4000		// Bit 14: signal 7 set 

// 
// Use of SIG bits
//
#define SP_CLR_YIELD		SP_CLR_SIG0
#define SP_SET_YIELD		SP_SET_SIG0
#define SP_STATUS_YIELD		SP_STATUS_SIG0
#define SP_CLR_YIELDED		SP_CLR_SIG1
#define SP_SET_YIELDED		SP_SET_SIG1
#define SP_STATUS_YIELDED	SP_STATUS_SIG1
#define SP_CLR_TASKDONE		SP_CLR_SIG2
#define SP_SET_TASKDONE		SP_SET_SIG2
#define SP_STATUS_TASKDONE	SP_STATUS_SIG2


// SP IMEM BIST REG (R/W): [6:0] BIST status bits; see below for detail 
#define SP_IBIST_REG	0x04080004

//
// Patterns to interpret status reg (SP_BIST_REG - write)
 //
#define SP_IBIST_CHECK		0x01		// Bit 0: BIST check 
#define SP_IBIST_GO			0x02		// Bit 1: BIST go 
#define SP_IBIST_CLEAR		0x04		// Bit 2: BIST clear 

//
// Patterns to interpret status reg (SP_BIST_REG - read)
//
// First 2 bits are same as in write mode:  
// Bit 0: BIST check; Bit 1: BIST go 
//
#define SP_IBIST_DONE		0x04		// Bit 2: BIST done 
#define SP_IBIST_FAILED		0x78		// Bit [6:3]: BIST fail 


#define SP_LAST_REG SP_SEMAPHORE_REG

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//             DP Command Registers                //
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

#define DPC_BASE_REG		0x04100000

// DP CMD DMA start (R/W): [23:0] DMEM/RDRAM start address 
#define DPC_START_REG		(DPC_BASE_REG+0x00)

// DP CMD DMA end (R/W): [23:0] DMEM/RDRAM end address 
#define DPC_END_REG		(DPC_BASE_REG+0x04)

// DP CMD DMA end (R): [23:0] DMEM/RDRAM current address 
#define DPC_CURRENT_REG		(DPC_BASE_REG+0x08)	

// DP CMD status (R/W): [9:0] valid bits - see below for definitions 
#define DPC_STATUS_REG		(DPC_BASE_REG+0x0C)

// DP clock counter (R): [23:0] clock counter 
#define DPC_CLOCK_REG		(DPC_BASE_REG+0x10)	

// DP buffer busy counter (R): [23:0] clock counter 
#define DPC_BUFBUSY_REG		(DPC_BASE_REG+0x14)

// DP pipe busy counter (R): [23:0] clock counter 
#define DPC_PIPEBUSY_REG	(DPC_BASE_REG+0x18)

// DP TMEM load counter (R): [23:0] clock counter 
#define DPC_TMEM_REG		(DPC_BASE_REG+0x1C)

//
// Values to clear/set bit in status reg (DPC_STATUS_REG - write)
//
#define DPC_CLR_XBUS_DMEM_DMA		0x0001		// Bit 0: clear xbus_dmem_dma 
#define DPC_SET_XBUS_DMEM_DMA		0x0002		// Bit 1: set xbus_dmem_dma 
#define DPC_CLR_FREEZE				0x0004		// Bit 2: clear freeze 
#define DPC_SET_FREEZE				0x0008		// Bit 3: set freeze 
#define DPC_CLR_FLUSH				0x0010		// Bit 4: clear flush 
#define DPC_SET_FLUSH				0x0020		// Bit 5: set flush 
#define DPC_CLR_TMEM_CTR			0x0040		// Bit 6: clear tmem ctr 
#define DPC_CLR_PIPE_CTR			0x0080		// Bit 7: clear pipe ctr 
#define DPC_CLR_CMD_CTR				0x0100		// Bit 8: clear cmd ctr 
#define DPC_CLR_CLOCK_CTR			0x0200		// Bit 9: clear clock ctr 

//
// Patterns to interpret status reg (DPC_STATUS_REG - read)
//
#define DPC_STATUS_XBUS_DMEM_DMA	0x001	// Bit  0: xbus_dmem_dma 
#define DPC_STATUS_FREEZE			0x002	// Bit  1: freeze 
#define DPC_STATUS_FLUSH			0x004	// Bit  2: flush 
//#define DPC_STATUS_FROZEN			0x008	// Bit  3: frozen 
#define DPC_STATUS_START_GCLK		0x008	// Bit  3: start gclk 
#define DPC_STATUS_TMEM_BUSY		0x010	// Bit  4: tmem busy 
#define DPC_STATUS_PIPE_BUSY		0x020	// Bit  5: pipe busy 
#define DPC_STATUS_CMD_BUSY			0x040	// Bit  6: cmd busy 
#define DPC_STATUS_CBUF_READY		0x080	// Bit  7: cbuf ready 
#define DPC_STATUS_DMA_BUSY			0x100	// Bit  8: dma busy 
#define DPC_STATUS_END_VALID		0x200	// Bit  9: end valid 
#define DPC_STATUS_START_VALID		0x400	// Bit 10: start valid 

#define DPC_LAST_REG	DPC_TMEM_REG

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//         MIPS Interface (MI) Registers           //
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////


#define MI_BASE_REG		0x04300000

// 
// MI init mode (W): [6:0] init length, [7] clear init mode, [8] set init mode
//                   [9/10] clear/set ebus test mode, [11] clear DP interrupt
//              (R): [6:0] init length, [7] init mode, [8] ebus test mode
//
#define MI_INIT_MODE_REG	(MI_BASE_REG+0x00)
#define MI_MODE_REG		MI_INIT_MODE_REG

//
// Values to clear/set bit in mode reg (MI_MODE_REG - write)
//
#define MI_CLR_INIT			0x0080		// Bit  7: clear init mode 
#define MI_SET_INIT			0x0100		// Bit  8: set init mode 
#define MI_CLR_EBUS			0x0200		// Bit  9: clear ebus test 
#define MI_SET_EBUS			0x0400		// Bit 10: set ebus test mode 
#define MI_CLR_DP_INTR		0x0800		// Bit 11: clear dp interrupt 
#define MI_CLR_RDRAM		0x1000		// Bit 12: clear RDRAM reg 
#define MI_SET_RDRAM		0x2000		// Bit 13: set RDRAM reg mode 

//
// Patterns to interpret mode reg (MI_MODE_REG - read)
//
#define MI_MODE_INIT		0x0080		// Bit  7: init mode 
#define MI_MODE_EBUS		0x0100		// Bit  8: ebus test mode 
#define MI_MODE_RDRAM		0x0200		// Bit  9: RDRAM reg mode 

// MI version (R): [7:0] io, [15:8] rac, [23:16] rdp, [31:24] rsp 
#define MI_VERSION_REG		(MI_BASE_REG+0x04)
#define MI_NOOP_REG		MI_VERSION_REG

// MI interrupt (R): [5:0] valid bits - see below for bit patterns 
#define MI_INTR_REG		(MI_BASE_REG+0x08)

// 
// MI interrupt mask (W): [11:0] valid bits - see below for bit patterns 
//                   (R): [5:0] valid bits - see below for bit patterns 
//
#define MI_INTR_MASK_REG	(MI_BASE_REG+0x0C)

//
// The following are values to check for interrupt setting (MI_INTR_REG)
//
#define MI_INTR_SP		0x01		// Bit 0: SP intr 
#define MI_INTR_SI		0x02		// Bit 1: SI intr 
#define MI_INTR_AI		0x04		// Bit 2: AI intr 
#define MI_INTR_VI		0x08		// Bit 3: VI intr 
#define MI_INTR_PI		0x10		// Bit 4: PI intr 
#define MI_INTR_DP		0x20		// Bit 5: DP intr 

//
// The following are values to clear/set various interrupt bit mask
// They can be ORed together to manipulate multiple bits 
// (MI_INTR_MASK_REG - write)
//
#define MI_INTR_MASK_CLR_SP	0x0001		// Bit  0: clear SP mask 
#define MI_INTR_MASK_SET_SP	0x0002		// Bit  1: set SP mask 
#define MI_INTR_MASK_CLR_SI	0x0004		// Bit  2: clear SI mask 
#define MI_INTR_MASK_SET_SI	0x0008		// Bit  3: set SI mask 
#define MI_INTR_MASK_CLR_AI	0x0010		// Bit  4: clear AI mask 
#define MI_INTR_MASK_SET_AI	0x0020		// Bit  5: set AI mask 
#define MI_INTR_MASK_CLR_VI	0x0040		// Bit  6: clear VI mask 
#define MI_INTR_MASK_SET_VI	0x0080		// Bit  7: set VI mask 
#define MI_INTR_MASK_CLR_PI	0x0100		// Bit  8: clear PI mask 
#define MI_INTR_MASK_SET_PI	0x0200		// Bit  9: set PI mask 
#define MI_INTR_MASK_CLR_DP	0x0400		// Bit 10: clear DP mask 
#define MI_INTR_MASK_SET_DP	0x0800		// Bit 11: set DP mask 

//
// The following are values to check for interrupt mask setting 
// (MI_INTR_MASK_REG - read)
//
#define MI_INTR_MASK_SP		0x01		// Bit 0: SP intr mask 
#define MI_INTR_MASK_SI		0x02		// Bit 1: SI intr mask 
#define MI_INTR_MASK_AI		0x04		// Bit 2: AI intr mask 
#define MI_INTR_MASK_VI		0x08		// Bit 3: VI intr mask 
#define MI_INTR_MASK_PI		0x10		// Bit 4: PI intr mask 
#define MI_INTR_MASK_DP		0x20		// Bit 5: DP intr mask 


#define MI_LAST_REG MI_INTR_MASK_REG

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//         Video Interface (VI) Registers           //
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////


#define VI_BASE_REG		0x04400000

// VI status/control (R/W): [15-0] valid bits: 
//	[1:0]   = type[1:0] (pixel size) 
//			            0: blank (no data, no sync)
//                      1: reserved
//                      2: 5/5/5/3 ("16" bit)
//                      3: 8/8/8/8 (32 bit)
//	[2]     = gamma_dither_enable (normally on, unless "special effect")
//	[3]     = gamma_enable (normally on, unless MPEG/JPEG)
//	[4]     = divot_enable (normally on if antialiased, unless decal lines)
//	[5]     = reserved - always off
//	[6]     = serrate (always on if interlaced, off if not)
//	[7]     = reserved - diagnostics only
//	[9:8]   = anti-alias (aa) mode[1:0] 
//			            0: aa & resamp (always fetch extra lines)
//                      1: aa & resamp (fetch extra lines if needed)
//                      2: resamp only (treat as all fully covered)
//                      3: neither (replicate pixels, no interpolate)
//	[11]    = reserved - diagnostics only
//	[15:12] = reserved
//
//
#define VI_STATUS_REG		(VI_BASE_REG+0x00)
#define VI_CONTROL_REG		VI_STATUS_REG

// VI origin (R/W): [23:0] frame buffer origin in bytes
#define VI_ORIGIN_REG		(VI_BASE_REG+0x04)
#define VI_DRAM_ADDR_REG	VI_ORIGIN_REG

// VI width (R/W): [11:0] frame buffer line width in pixels
#define VI_WIDTH_REG		(VI_BASE_REG+0x08)	
#define VI_H_WIDTH_REG		VI_WIDTH_REG

// VI vertical intr (R/W): [9:0] interrupt when current half-line = V_INTR
#define VI_INTR_REG		(VI_BASE_REG+0x0C)	
#define VI_V_INTR_REG		VI_INTR_REG

// 
// VI current vertical line (R/W): [9:0] current half line, sampled once per
//	line (the lsb of V_CURRENT is constant within a field, and in
//	interlaced modes gives the field number - which is constant for non-
//	interlaced modes)
// 	- Any write to this register will clear interrupt line
//
#define VI_CURRENT_REG		(VI_BASE_REG+0x10)	
#define VI_V_CURRENT_LINE_REG	VI_CURRENT_REG

// 
// VI video timing (R/W): [ 7: 0] horizontal sync width in pixels, 
//			  [15: 8] color burst width in pixels,
//                        [19:16] vertical sync width in half lines,   
//			  [29:20] start of color burst in pixels from h-sync
//
#define VI_BURST_REG		(VI_BASE_REG+0x14)	
#define VI_TIMING_REG		VI_BURST_REG

// VI vertical sync (R/W): [9:0] number of half-lines per field
#define VI_V_SYNC_REG		(VI_BASE_REG+0x18)	

// VI horizontal sync (R/W): [11: 0] total duration of a line in 1/4 pixel 
//						     [20:16] a 5-bit leap pattern used for PAL only 
//				                     (h_sync_period)
//
#define VI_H_SYNC_REG		(VI_BASE_REG+0x1C)	

// 
// VI horizontal sync leap (R/W): [11: 0] identical to h_sync_period
//                                [27:16] identical to h_sync_period
//
#define VI_LEAP_REG		(VI_BASE_REG+0x20)	
#define VI_H_SYNC_LEAP_REG	VI_LEAP_REG

// 
// VI horizontal video (R/W): [ 9: 0] end of active video in screen pixels
//                          : [25:16] start of active video in screen pixels
//
#define VI_H_START_REG		(VI_BASE_REG+0x24)
#define VI_H_VIDEO_REG		VI_H_START_REG

// 
// VI vertical video (R/W): [ 9: 0] end of active video in screen half-lines
//                        : [25:16] start of active video in screen half-lines
//
#define VI_V_START_REG		(VI_BASE_REG+0x28)
#define VI_V_VIDEO_REG		VI_V_START_REG

// 
// VI vertical burst (R/W): [ 9: 0] end of color burst enable in half-lines
//                        : [25:16] start of color burst enable in half-lines
//
#define VI_V_BURST_REG		(VI_BASE_REG+0x2C)	

// VI x-scale (R/W): [11: 0] 1/horizontal scale up factor (2.10 format)
//		             [27:16] horizontal subpixel offset (2.10 format)
//
#define VI_X_SCALE_REG		(VI_BASE_REG+0x30)	

// VI y-scale (R/W): [11: 0] 1/vertical scale up factor (2.10 format)
//		             [27:16] vertical subpixel offset (2.10 format)
//
#define VI_Y_SCALE_REG		(VI_BASE_REG+0x34)	

//
// Patterns to interpret VI_CONTROL_REG
//
#define VI_CTRL_TYPE_16				0x00002    // Bit [1:0] pixel size: 16 bit
#define VI_CTRL_TYPE_32				0x00003    // Bit [1:0] pixel size: 32 bit
#define VI_CTRL_GAMMA_DITHER_ON		0x00004    // Bit 2: default = on
#define VI_CTRL_GAMMA_ON			0x00008    // Bit 3: default = on
#define VI_CTRL_DIVOT_ON			0x00010    // Bit 4: default = on
#define VI_CTRL_SERRATE_ON			0x00040    // Bit 6: on if interlaced
#define VI_CTRL_ANTIALIAS_MASK		0x00300    // Bit [9:8] anti-alias mode
#define VI_CTRL_DITHER_FILTER_ON	0x10000    // Bit 16: dither-filter mode

//
// Possible video clocks (NTSC or PAL)
//
#define VI_NTSC_CLOCK		48681812        // Hz = 48.681812 MHz
#define VI_PAL_CLOCK		49656530        // Hz = 49.656530 MHz
#define VI_MPAL_CLOCK		48628316        // Hz = 48.628316 MHz


#define VI_LAST_REG	VI_Y_SCALE_REG

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//         Audio Interface (AI) Registers          //
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////


//
// The address and length registers are double buffered; that is, they
// can be written twice before becoming full.
// The address must be written before the length.
//
#define AI_BASE_REG		0x04500000

// AI DRAM address (W): [23:0] starting RDRAM address (8B-aligned) 
#define AI_DRAM_ADDR_REG	(AI_BASE_REG+0x00)	// R0: DRAM address 

// AI length (R/W): [14:0] transfer length (v1.0) - Bottom 3 bits are ignored 
//                  [17:0] transfer length (v2.0) - Bottom 3 bits are ignored 
#define AI_LEN_REG		(AI_BASE_REG+0x04)	// R1: Length 

// AI control (W): [0] DMA enable - if LSB == 1, DMA is enabled 
#define AI_CONTROL_REG		(AI_BASE_REG+0x08)	// R2: DMA Control 

// 
// AI status (R): [31]/[0] ai_full (addr & len buffer full), [30] ai_busy 
//		  Note that a 1->0 transition in ai_full will set interrupt
//           (W): clear audio interrupt 
// 
#define AI_STATUS_REG		(AI_BASE_REG+0x0C)	// R3: Status 

// 
// AI DAC sample period register (W): [13:0] dac rate 
//   - vid_clock/(dperiod + 1) is the DAC sample rate
//   - (dperiod + 1) >= 66// (aclockhp + 1) must be true
// 
#define AI_DACRATE_REG		(AI_BASE_REG+0x10)	// R4: DAC rate 14-lsb

// 
// AI bit rate (W): [3:0] bit rate (abus clock half period register - aclockhp)
//   - vid_clock/(2// (aclockhp + 1)) is the DAC clock rate
//   - The abus clock stops if aclockhp is zero
// 
#define AI_BITRATE_REG		(AI_BASE_REG+0x14)	// R5: Bit rate 4-lsb 

// Value for control register 
#define AI_CONTROL_DMA_ON	0x01			// LSB = 1: DMA enable
#define AI_CONTROL_DMA_OFF	0x00			// LSB = 1: DMA enable

// Value for status register 
#define AI_STATUS_FIFO_FULL	0x80000000		// Bit 31: full 
#define AI_STATUS_DMA_BUSY	0x40000000		// Bit 30: busy 

// DAC rate = video clock / audio frequency
//   - DAC rate >= (66// Bit rate) must be true
// 
#define AI_MAX_DAC_RATE         16384           // 14-bit+1 
#define AI_MIN_DAC_RATE         132

// Bit rate <= (DAC rate / 66) 
#define AI_MAX_BIT_RATE         16              // 4-bit+1 
#define AI_MIN_BIT_RATE         2

//
// Maximum and minimum values for audio frequency based on video clocks
//   max frequency = (video clock / min dac rate)
//   min frequency = (video clock / max dac rate)
//
#define AI_NTSC_MAX_FREQ        368000          // 368 KHz 
#define AI_NTSC_MIN_FREQ        3000            //   3 KHz ~ 2971 Hz 

#define AI_PAL_MAX_FREQ         376000          // 376 KHz 
#define AI_PAL_MIN_FREQ         3050            //   3 KHz ~ 3031 Hz 

#define AI_MPAL_MAX_FREQ        368000          // 368 KHz 
#define AI_MPAL_MIN_FREQ        3000            //   3 KHz ~ 2968 Hz 


#define AI_LAST_REG	AI_BITRATE_REG

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//       Peripheral Interface (PI) Registers       //
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////


#define PI_BASE_REG		0x04600000

// PI DRAM address (R/W): [23:0] starting RDRAM address 
#define PI_DRAM_ADDR_REG	(PI_BASE_REG+0x00)	// DRAM address 

// PI pbus (cartridge) address (R/W): [31:0] starting AD16 address 
#define PI_CART_ADDR_REG	(PI_BASE_REG+0x04)

// PI read length (R/W): [23:0] read data length 
#define PI_RD_LEN_REG		(PI_BASE_REG+0x08)

// PI write length (R/W): [23:0] write data length 
#define PI_WR_LEN_REG		(PI_BASE_REG+0x0C)

// 
// PI status (R): [0] DMA busy, [1] IO busy, [2], error
//           (W): [0] reset controller (and abort current op), [1] clear intr
 
#define PI_STATUS_REG		(PI_BASE_REG+0x10)

// PI dom1 latency (R/W): [7:0] domain 1 device latency 
#define PI_BSD_DOM1_LAT_REG	(PI_BASE_REG+0x14)

// PI dom1 pulse width (R/W): [7:0] domain 1 device R/W strobe pulse width 
#define PI_BSD_DOM1_PWD_REG	(PI_BASE_REG+0x18)

// PI dom1 page size (R/W): [3:0] domain 1 device page size 
#define PI_BSD_DOM1_PGS_REG	(PI_BASE_REG+0x1C)    //   page size 

// PI dom1 release (R/W): [1:0] domain 1 device R/W release duration 
#define PI_BSD_DOM1_RLS_REG	(PI_BASE_REG+0x20)

// PI dom2 latency (R/W): [7:0] domain 2 device latency 
#define PI_BSD_DOM2_LAT_REG	(PI_BASE_REG+0x24)    // Domain 2 latency 

// PI dom2 pulse width (R/W): [7:0] domain 2 device R/W strobe pulse width 
#define PI_BSD_DOM2_PWD_REG	(PI_BASE_REG+0x28)    //   pulse width 

// PI dom2 page size (R/W): [3:0] domain 2 device page size 
#define PI_BSD_DOM2_PGS_REG	(PI_BASE_REG+0x2C)    //   page size 

// PI dom2 release (R/W): [1:0] domain 2 device R/W release duration 
#define PI_BSD_DOM2_RLS_REG	(PI_BASE_REG+0x30)    //   release duration 

#define	PI_DOMAIN1_REG		PI_BSD_DOM1_LAT_REG
#define	PI_DOMAIN2_REG		PI_BSD_DOM2_LAT_REG

#define PI_DOM_LAT_OFS		0x00
#define PI_DOM_PWD_OFS		0x04
#define PI_DOM_PGS_OFS		0x08
#define PI_DOM_RLS_OFS		0x0C

//
// PI status register has 3 bits active when read from (PI_STATUS_REG - read)
//	Bit 0: DMA busy - set when DMA is in progress
//	Bit 1: IO busy  - set when IO is in progress
//	Bit 2: Error    - set when CPU issues IO request while DMA is busy
 
#define	PI_STATUS_ERROR		0x04
#define	PI_STATUS_IO_BUSY	0x02
#define	PI_STATUS_DMA_BUSY	0x01

// PI status register has 2 bits active when written to:
//	Bit 0: When set, reset PIC
//	Bit 1: When set, clear interrupt flag
// The values of the two bits can be ORed together to both reset PIC and 
// clear interrupt at the same time.
//
// Note: 
//	- The PIC does generate an interrupt at the end of each DMA. CPU 
//	needs to clear the interrupt flag explicitly (from an interrupt 
//	handler) by writing into the STATUS register with bit 1 set.
//
//	- When a DMA completes, the interrupt flag is set.  CPU can issue
//	another request even while the interrupt flag is set (as long as
//	PIC is idle).  However, it is the CPU's responsibility for
//	maintaining accurate correspondence between DMA completions and
//	interrupts.
//
//	- When PIC is reset, if PIC happens to be busy, an interrupt will
//	be generated as PIC returns to idle.  Otherwise, no interrupt will
//	be generated and PIC remains idle.
//
//
// Values to clear interrupt/reset PIC (PI_STATUS_REG - write)
//
#define	PI_STATUS_RESET		0x01
#define	PI_SET_RESET		PI_STATUS_RESET

#define	PI_STATUS_CLR_INTR	0x02
#define	PI_CLR_INTR		PI_STATUS_CLR_INTR

#define	PI_DMA_BUFFER_SIZE	128

#define PI_DOM1_ADDR1		0x06000000	// to 0x07FFFFFF 
#define PI_DOM1_ADDR2		0x10000000	// to 0x1FBFFFFF 
#define PI_DOM1_ADDR3		0x1FD00000	// to 0x7FFFFFFF 
#define PI_DOM2_ADDR1		0x05000000	// to 0x05FFFFFF 
#define PI_DOM2_ADDR2		0x08000000	// to 0x0FFFFFFF 



#define PI_LAST_REG		PI_BSD_DOM2_RLS_REG

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//         RDRAM Interface (RI) Registers          //
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////


#define RI_BASE_REG		0x04700000

// RI mode (R/W): [1:0] operating mode, [2] stop T active, [3] stop R active
#define RI_MODE_REG		(RI_BASE_REG+0x00)	

// RI config (R/W): [5:0] current control input, [6] current control enable
#define RI_CONFIG_REG		(RI_BASE_REG+0x04)

// RI current load (W): [] any write updates current control register
#define RI_CURRENT_LOAD_REG	(RI_BASE_REG+0x08)

// RI select (R/W): [2:0] receive select, [2:0] transmit select 
#define RI_SELECT_REG		(RI_BASE_REG+0x0C)

// RI refresh (R/W): [7:0] clean refresh delay, [15:8] dirty refresh delay,
//		     [16] refresh bank, [17] refresh enable 
//		     [18] refresh optimize 
//
#define RI_REFRESH_REG		(RI_BASE_REG+0x10)
#define RI_COUNT_REG		RI_REFRESH_REG

// RI latency (R/W): [3:0] DMA latency/overlap 
#define RI_LATENCY_REG		(RI_BASE_REG+0x14)

// RI error (R): [0] nack error, [1] ack error 
#define RI_RERROR_REG		(RI_BASE_REG+0x18)

// RI error (W): [] any write clears all error bits 
#define RI_WERROR_REG		(RI_BASE_REG+0x1C)


#define RI_LAST_REG RI_WERROR_REG

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////
//         Serial Interface (SI) Registers         //
/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

#define SI_BASE_REG		0x04800000

// SI DRAM address (R/W): [23:0] starting RDRAM address
#define SI_DRAM_ADDR_REG	(SI_BASE_REG+0x00)	// R0: DRAM address

// SI address read 64B (W): [] any write causes a 64B DMA write
#define SI_PIF_ADDR_RD64B_REG	(SI_BASE_REG+0x04)	// R1: 64B PIF->DRAM

// Address SI_BASE_REG + (0x08, 0x0c, 0x14) are reserved

// SI address write 64B (W): [] any write causes a 64B DMA read
#define SI_PIF_ADDR_WR64B_REG	(SI_BASE_REG+0x10)	// R4: 64B DRAM->PIF

// 
// SI status (W): [] any write clears interrupt
//           (R): [0] DMA busy, [1] IO read busy, [2] reserved
//                [3] DMA error, [12] interrupt
//
#define SI_STATUS_REG		(SI_BASE_REG+0x18)	// R6: Status

// SI status register has the following bits active:
//	0:   DMA busy		- set when DMA is in progress
//	1:   IO busy		- set when IO access is in progress
//	3:   DMA error		- set when there are overlapping DMA requests
// 12:   Interrupt		- Interrupt set
//
#define	SI_STATUS_DMA_BUSY	0x0001
#define	SI_STATUS_RD_BUSY	0x0002
#define	SI_STATUS_DMA_ERROR	0x0008
#define	SI_STATUS_INTERRUPT	0x1000

#define SI_LAST_REG		SI_STATUS_REG



#define PIF_ROM_START	0x1FC00000
#define PIF_ROM_END		0x1FC007BF
#define PIF_RAM_START	0x1FC007C0
#define PIF_RAM_END		0x1FC007FF


#endif // __ULTRA_RCP_H__
