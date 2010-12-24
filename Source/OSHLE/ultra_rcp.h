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

#define SP_DMEM_START		0x04000000
#define SP_DMEM_END			0x04000FFF
#define SP_IMEM_START		0x04001000
#define SP_IMEM_END			0x04001FFF

#define SP_BASE_REG			0x04040000

#define SP_MEM_ADDR_REG		(SP_BASE_REG+0x00)
#define SP_DRAM_ADDR_REG	(SP_BASE_REG+0x04)
#define SP_RD_LEN_REG		(SP_BASE_REG+0x08)
#define SP_WR_LEN_REG		(SP_BASE_REG+0x0C)
#define SP_STATUS_REG		(SP_BASE_REG+0x10)
#define SP_DMA_FULL_REG		(SP_BASE_REG+0x14)
#define SP_DMA_BUSY_REG		(SP_BASE_REG+0x18)
#define SP_SEMAPHORE_REG	(SP_BASE_REG+0x1C)


#define SP_LAST_REG SP_SEMAPHORE_REG
#define SP_PC_REG			0x04080000
#define SP_DMA_DMEM			0x0000
#define SP_DMA_IMEM			0x1000

#define SP_CLR_HALT			0x0000001
#define SP_SET_HALT			0x0000002
#define SP_CLR_BROKE		0x0000004
#define SP_CLR_INTR			0x0000008
#define SP_SET_INTR			0x0000010
#define SP_CLR_SSTEP		0x0000020
#define SP_SET_SSTEP		0x0000040
#define SP_CLR_INTR_BREAK	0x0000080	    
#define SP_SET_INTR_BREAK	0x0000100	    
#define SP_CLR_SIG0			0x0000200	    
#define SP_SET_SIG0			0x0000400	    
#define SP_CLR_SIG1			0x0000800	    
#define SP_SET_SIG1			0x0001000	    
#define SP_CLR_SIG2			0x0002000	    
#define SP_SET_SIG2			0x0004000	    
#define SP_CLR_SIG3			0x0008000	    
#define SP_SET_SIG3			0x0010000	    
#define SP_CLR_SIG4			0x0020000	    
#define SP_SET_SIG4			0x0040000	    
#define SP_CLR_SIG5			0x0080000	    
#define SP_SET_SIG5			0x0100000	    
#define SP_CLR_SIG6			0x0200000	    
#define SP_SET_SIG6			0x0400000	    
#define SP_CLR_SIG7			0x0800000	    
#define SP_SET_SIG7			0x1000000	    

#define SP_STATUS_HALT			0x001		
#define SP_STATUS_BROKE			0x002		
#define SP_STATUS_DMA_BUSY		0x004		
#define SP_STATUS_DMA_FULL		0x008		
#define SP_STATUS_IO_FULL		0x010		
#define SP_STATUS_SSTEP			0x020		
#define SP_STATUS_INTR_BREAK	0x040		
#define SP_STATUS_SIG0			0x080		
#define SP_STATUS_SIG1			0x100		
#define SP_STATUS_SIG2			0x200		
#define SP_STATUS_SIG3			0x400		
#define SP_STATUS_SIG4			0x800		
#define SP_STATUS_SIG5	       0x1000		
#define SP_STATUS_SIG6	       0x2000		
#define SP_STATUS_SIG7	       0x4000		

#define SP_CLR_YIELD		SP_CLR_SIG0
#define SP_SET_YIELD		SP_SET_SIG0
#define SP_STATUS_YIELD		SP_STATUS_SIG0
#define SP_CLR_YIELDED		SP_CLR_SIG1
#define SP_SET_YIELDED		SP_SET_SIG1
#define SP_STATUS_YIELDED	SP_STATUS_SIG1
#define SP_CLR_TASKDONE		SP_CLR_SIG2
#define SP_SET_TASKDONE		SP_SET_SIG2
#define SP_STATUS_TASKDONE	SP_STATUS_SIG2

#define SP_IBIST_REG	0x04080004

#define SP_IBIST_CHECK		0x01		
#define SP_IBIST_GO			0x02		
#define SP_IBIST_CLEAR		0x04		

#define SP_IBIST_DONE		0x04		
#define SP_IBIST_FAILED		0x78		

#define SP_LAST_REG SP_SEMAPHORE_REG

#define DPC_BASE_REG		0x04100000

#define DPC_START_REG		(DPC_BASE_REG+0x00)
#define DPC_END_REG			(DPC_BASE_REG+0x04)
#define DPC_CURRENT_REG		(DPC_BASE_REG+0x08)	
#define DPC_STATUS_REG		(DPC_BASE_REG+0x0C)
#define DPC_CLOCK_REG		(DPC_BASE_REG+0x10)	
#define DPC_BUFBUSY_REG		(DPC_BASE_REG+0x14)
#define DPC_PIPEBUSY_REG	(DPC_BASE_REG+0x18)
#define DPC_TMEM_REG		(DPC_BASE_REG+0x1C)

#define DPC_CLR_XBUS_DMEM_DMA		0x0001		
#define DPC_SET_XBUS_DMEM_DMA		0x0002		
#define DPC_CLR_FREEZE				0x0004		
#define DPC_SET_FREEZE				0x0008		
#define DPC_CLR_FLUSH				0x0010		
#define DPC_SET_FLUSH				0x0020		
#define DPC_CLR_TMEM_CTR			0x0040		
#define DPC_CLR_PIPE_CTR			0x0080		
#define DPC_CLR_CMD_CTR				0x0100		
#define DPC_CLR_CLOCK_CTR			0x0200		

#define DPC_STATUS_XBUS_DMEM_DMA	0x001	
#define DPC_STATUS_FREEZE			0x002	
#define DPC_STATUS_FLUSH			0x004	
//#define DPC_STATUS_FROZEN			0x008	// Bit  3: frozen 
#define DPC_STATUS_START_GCLK		0x008	
#define DPC_STATUS_TMEM_BUSY		0x010	
#define DPC_STATUS_PIPE_BUSY		0x020	
#define DPC_STATUS_CMD_BUSY			0x040	
#define DPC_STATUS_CBUF_READY		0x080	
#define DPC_STATUS_DMA_BUSY			0x100	
#define DPC_STATUS_END_VALID		0x200	
#define DPC_STATUS_START_VALID		0x400	

#define DPC_LAST_REG	DPC_TMEM_REG

#define MI_BASE_REG		0x04300000

#define MI_INIT_MODE_REG	(MI_BASE_REG+0x00)
#define MI_MODE_REG			MI_INIT_MODE_REG

#define MI_CLR_INIT			0x0080		
#define MI_SET_INIT			0x0100		
#define MI_CLR_EBUS			0x0200		
#define MI_SET_EBUS			0x0400		
#define MI_CLR_DP_INTR		0x0800		
#define MI_CLR_RDRAM		0x1000		
#define MI_SET_RDRAM		0x2000		

#define MI_MODE_INIT		0x0080		
#define MI_MODE_EBUS		0x0100		
#define MI_MODE_RDRAM		0x0200		

#define MI_VERSION_REG		(MI_BASE_REG+0x04)
#define MI_NOOP_REG			MI_VERSION_REG
#define MI_INTR_REG			(MI_BASE_REG+0x08)
#define MI_INTR_MASK_REG	(MI_BASE_REG+0x0C)

#define MI_INTR_SP		0x01		
#define MI_INTR_SI		0x02		
#define MI_INTR_AI		0x04		
#define MI_INTR_VI		0x08		
#define MI_INTR_PI		0x10		
#define MI_INTR_DP		0x20		

#define MI_INTR_MASK_CLR_SP	0x0001		
#define MI_INTR_MASK_SET_SP	0x0002		
#define MI_INTR_MASK_CLR_SI	0x0004		
#define MI_INTR_MASK_SET_SI	0x0008		
#define MI_INTR_MASK_CLR_AI	0x0010		
#define MI_INTR_MASK_SET_AI	0x0020		
#define MI_INTR_MASK_CLR_VI	0x0040		
#define MI_INTR_MASK_SET_VI	0x0080		
#define MI_INTR_MASK_CLR_PI	0x0100		
#define MI_INTR_MASK_SET_PI	0x0200		
#define MI_INTR_MASK_CLR_DP	0x0400		
#define MI_INTR_MASK_SET_DP	0x0800		

#define MI_INTR_MASK_SP		0x01		
#define MI_INTR_MASK_SI		0x02		
#define MI_INTR_MASK_AI		0x04		
#define MI_INTR_MASK_VI		0x08		
#define MI_INTR_MASK_PI		0x10		
#define MI_INTR_MASK_DP		0x20		

#define MI_LAST_REG MI_INTR_MASK_REG

#define VI_BASE_REG		0x04400000

#define VI_STATUS_REG		(VI_BASE_REG+0x00)
#define VI_CONTROL_REG		VI_STATUS_REG
#define VI_ORIGIN_REG		(VI_BASE_REG+0x04)
#define VI_DRAM_ADDR_REG	VI_ORIGIN_REG
#define VI_WIDTH_REG		(VI_BASE_REG+0x08)	
#define VI_H_WIDTH_REG		VI_WIDTH_REG
#define VI_INTR_REG			(VI_BASE_REG+0x0C)	
#define VI_V_INTR_REG		VI_INTR_REG
#define VI_CURRENT_REG		(VI_BASE_REG+0x10)	
#define VI_V_CURRENT_LINE_REG	VI_CURRENT_REG
#define VI_BURST_REG		(VI_BASE_REG+0x14)	
#define VI_TIMING_REG		VI_BURST_REG
#define VI_V_SYNC_REG		(VI_BASE_REG+0x18)	
#define VI_H_SYNC_REG		(VI_BASE_REG+0x1C)	
#define VI_LEAP_REG			(VI_BASE_REG+0x20)	
#define VI_H_SYNC_LEAP_REG	VI_LEAP_REG
#define VI_H_START_REG		(VI_BASE_REG+0x24)
#define VI_H_VIDEO_REG		VI_H_START_REG
#define VI_V_START_REG		(VI_BASE_REG+0x28)
#define VI_V_VIDEO_REG		VI_V_START_REG
#define VI_V_BURST_REG		(VI_BASE_REG+0x2C)	
#define VI_X_SCALE_REG		(VI_BASE_REG+0x30)	
#define VI_Y_SCALE_REG		(VI_BASE_REG+0x34)	

#define VI_CTRL_TYPE_16				0x00002    
#define VI_CTRL_TYPE_32				0x00003    
#define VI_CTRL_GAMMA_DITHER_ON		0x00004    
#define VI_CTRL_GAMMA_ON			0x00008    
#define VI_CTRL_DIVOT_ON			0x00010    
#define VI_CTRL_SERRATE_ON			0x00040    
#define VI_CTRL_ANTIALIAS_MASK		0x00300    
#define VI_CTRL_DITHER_FILTER_ON	0x10000    

#define VI_NTSC_CLOCK		48681812        
#define VI_PAL_CLOCK		49656530        
#define VI_MPAL_CLOCK		48628316        


#define VI_LAST_REG	VI_Y_SCALE_REG

#define AI_BASE_REG		0x04500000
#define AI_DRAM_ADDR_REG	(AI_BASE_REG+0x00)	
#define AI_LEN_REG		(AI_BASE_REG+0x04)	
#define AI_CONTROL_REG		(AI_BASE_REG+0x08)	
#define AI_STATUS_REG		(AI_BASE_REG+0x0C)	
#define AI_DACRATE_REG		(AI_BASE_REG+0x10)	
#define AI_BITRATE_REG		(AI_BASE_REG+0x14)	

#define AI_CONTROL_DMA_ON	0x01			
#define AI_CONTROL_DMA_OFF	0x00			

#define AI_STATUS_FIFO_FULL	0x80000000		
#define AI_STATUS_DMA_BUSY	0x40000000		

#define AI_MAX_DAC_RATE         16384           
#define AI_MIN_DAC_RATE         132

#define AI_MAX_BIT_RATE         16              
#define AI_MIN_BIT_RATE         2

#define AI_NTSC_MAX_FREQ        368000          
#define AI_NTSC_MIN_FREQ        3000            

#define AI_PAL_MAX_FREQ         376000          
#define AI_PAL_MIN_FREQ         3050            

#define AI_MPAL_MAX_FREQ        368000          
#define AI_MPAL_MIN_FREQ        3000            


#define AI_LAST_REG	AI_BITRATE_REG

#define PI_BASE_REG		0x04600000

#define PI_DRAM_ADDR_REG	(PI_BASE_REG+0x00)	
#define PI_CART_ADDR_REG	(PI_BASE_REG+0x04)
#define PI_RD_LEN_REG		(PI_BASE_REG+0x08)
#define PI_WR_LEN_REG		(PI_BASE_REG+0x0C)

#define PI_STATUS_REG		(PI_BASE_REG+0x10)

#define PI_BSD_DOM1_LAT_REG	(PI_BASE_REG+0x14)
#define PI_BSD_DOM1_PWD_REG	(PI_BASE_REG+0x18)
#define PI_BSD_DOM1_PGS_REG	(PI_BASE_REG+0x1C)    
#define PI_BSD_DOM1_RLS_REG	(PI_BASE_REG+0x20)
#define PI_BSD_DOM2_LAT_REG	(PI_BASE_REG+0x24)    
#define PI_BSD_DOM2_PWD_REG	(PI_BASE_REG+0x28)    
#define PI_BSD_DOM2_PGS_REG	(PI_BASE_REG+0x2C)    
#define PI_BSD_DOM2_RLS_REG	(PI_BASE_REG+0x30)    

#define	PI_DOMAIN1_REG		PI_BSD_DOM1_LAT_REG
#define	PI_DOMAIN2_REG		PI_BSD_DOM2_LAT_REG

#define PI_DOM_LAT_OFS		0x00
#define PI_DOM_PWD_OFS		0x04
#define PI_DOM_PGS_OFS		0x08
#define PI_DOM_RLS_OFS		0x0C
 
#define	PI_STATUS_ERROR			0x04
#define	PI_STATUS_IO_BUSY		0x02
#define	PI_STATUS_DMA_BUSY		0x01
#define PI_STATUS_DMA_IO_BUSY	0x03

#define	PI_STATUS_RESET		0x01
#define	PI_SET_RESET		PI_STATUS_RESET

#define	PI_STATUS_CLR_INTR	0x02
#define	PI_CLR_INTR		PI_STATUS_CLR_INTR

#define	PI_DMA_BUFFER_SIZE	128

#define PI_DOM1_ADDR1		0x06000000	
#define PI_DOM1_ADDR2		0x10000000	
#define PI_DOM1_ADDR3		0x1FD00000	
#define PI_DOM2_ADDR1		0x05000000	
#define PI_DOM2_ADDR2		0x08000000	

#define PI_LAST_REG			PI_BSD_DOM2_RLS_REG
#define RI_BASE_REG			0x04700000
#define RI_MODE_REG			(RI_BASE_REG+0x00)	
#define RI_CONFIG_REG		(RI_BASE_REG+0x04)
#define RI_CURRENT_LOAD_REG	(RI_BASE_REG+0x08)
#define RI_SELECT_REG		(RI_BASE_REG+0x0C)
#define RI_REFRESH_REG		(RI_BASE_REG+0x10)
#define RI_COUNT_REG		RI_REFRESH_REG
#define RI_LATENCY_REG		(RI_BASE_REG+0x14)
#define RI_RERROR_REG		(RI_BASE_REG+0x18)
#define RI_WERROR_REG		(RI_BASE_REG+0x1C)
#define RI_LAST_REG			RI_WERROR_REG

#define SI_BASE_REG			0x04800000
#define SI_DRAM_ADDR_REG		(SI_BASE_REG+0x00)	
#define SI_PIF_ADDR_RD64B_REG	(SI_BASE_REG+0x04)	
#define SI_PIF_ADDR_WR64B_REG	(SI_BASE_REG+0x10)	
#define SI_STATUS_REG			(SI_BASE_REG+0x18)	

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
