#ifndef __drv_l1_TFT_H__
#define __drv_l1_TFT_H__

#include "driver_l1.h"

#define TFT_ENABLE    0xFFFFFFFF
#define TFT_DIS       0

// tft clock divide
#define TFT_CLK_DIVIDE_1        0x0
#define TFT_CLK_DIVIDE_2        0x2
#define TFT_CLK_DIVIDE_3        0x4
#define TFT_CLK_DIVIDE_4        0x6
#define TFT_CLK_DIVIDE_5        0x8
#define TFT_CLK_DIVIDE_6        0xA
#define TFT_CLK_DIVIDE_7        0xC
#define TFT_CLK_DIVIDE_8        0xE
#define TFT_CLK_DIVIDE_9        0x10
#define TFT_CLK_DIVIDE_10       0x12
#define TFT_CLK_DIVIDE_11       0x14
#define TFT_CLK_DIVIDE_12       0x16
#define TFT_CLK_DIVIDE_13       0x18
#define TFT_CLK_DIVIDE_14       0x1A
#define TFT_CLK_DIVIDE_15       0x1C
#define TFT_CLK_DIVIDE_16       0x1E
#define TFT_CLK_DIVIDE_17       0x20
#define TFT_CLK_DIVIDE_18       0x22
#define TFT_CLK_DIVIDE_19       0x24
#define TFT_CLK_DIVIDE_20       0x26
#define TFT_CLK_DIVIDE_21       0x28
#define TFT_CLK_DIVIDE_22       0x2A
#define TFT_CLK_DIVIDE_23       0x2C
#define TFT_CLK_DIVIDE_24       0x2E
#define TFT_CLK_DIVIDE_25       0x30
#define TFT_CLK_DIVIDE_26       0x32
#define TFT_CLK_DIVIDE_27       0x34
#define TFT_CLK_DIVIDE_28       0x36
#define TFT_CLK_DIVIDE_29       0x38
#define TFT_CLK_DIVIDE_30       0x3A
#define TFT_CLK_DIVIDE_31       0x3C
#define TFT_CLK_DIVIDE_32       0x3E

// P_TFT_CTRL
#define TFT_EN                  (1<<0)
#define TFT_CLK_SEL             (7<<1)
#define TFT_MODE                (0xF<<4)
#define TFT_VSYNC_INV           (1<<8)
#define TFT_HSYNC_INV           (1<<9)
#define TFT_DCLK_INV            (1<<10)
#define TFT_DE_INV              (1<<11)
#define TFT_H_COMPRESS          (1<<12)
#define TFT_MEM_BYTE_EN         (1<<13)
#define TFT_INTERLACE_MOD       (1<<14)
#define TFT_VSYNC_UNIT          (1<<15)

// P_TFT_LINE_RGB_ORDER
#define TFT_REG_POL_1LINE       0x0
#define TFT_REG_POL_2LINE       0x1
#define TFT_NEW_POL_EN          0x80

// P_TFT_TS_MISC
#define TFT_REG_POL             (1<<4)
#define TFT_REG_REV             (1<<5)
#define TFT_UD_I                (1<<6)
#define TFT_RL_I                (1<<7)
#define TFT_DITH_EN             (1<<8)
#define TFT_DITH_MODE           (1<<9)
#define TFT_DATA_MODE           (3<<10)
#define TFT_SWITCH_EN           (1<<12)
#define TFT_GAMMA_EN            (1<<13)
#define TFT_DCLK_SEL            (3<<14)
#define TFT_DCLK_DELAY          (7<<18)
#define TFT_SLIDE_EN            (1<<21)

// tft mode
#define TFT_MODE_UPS051         0x0
#define TFT_MODE_UPS052         0x10
#define TFT_MODE_CCIR656        0x20
#define TFT_MODE_PARALLEL       0x30
#define TFT_MODE_TCON           0x40

#define TFT_MODE_MEM_CMD_WR     0x80
#define TFT_MODE_MEM_CMD_RD     0x90
#define TFT_MODE_MEM_DATA_WR    0xa0
#define TFT_MODE_MEM_DATA_RD    0xb0
#define TFT_MODE_MEM_CONTI      0xc0
#define TFT_MODE_MEM_ONCE       0xd0

// tft data mode
#define TFT_DATA_MODE_8         0x0             
#define TFT_DATA_MODE_565	    0x400
#define TFT_DATA_MODE_666       0x800
#define TFT_DATA_MODE_888       0xC00

// tft clock phase select
#define TFT_DCLK_SEL_0          0x0000
#define TFT_DCLK_SEL_90         0x4000
#define TFT_DCLK_SEL_180        0x8000
#define TFT_DCLK_SEL_270        0xC000

// tft clock delay select
#define TFT_DCLK_DELAY_0        0x000000
#define TFT_DCLK_DELAY_1        0x040000
#define TFT_DCLK_DELAY_2        0x080000
#define TFT_DCLK_DELAY_3        0x0C0000
#define TFT_DCLK_DELAY_4        0x100000
#define TFT_DCLK_DELAY_5        0x140000
#define TFT_DCLK_DELAY_6        0x180000
#define TFT_DCLK_DELAY_7        0x1C0000

extern void drv_l1_tft_init(void);
extern void drv_l1_tft_en_set(INT32U enable);
extern void drv_l1_tft_clk_set(INT32U clk);
extern void drv_l1_tft_mode_set(INT32U mode);
extern void drv_l1_tft_signal_inv_set(INT32U mask, INT32U value);
extern void drv_l1_tft_h_compress_set(INT32U enable);
extern void drv_l1_tft_mem_unit_set(INT8U data_width);
extern void drv_l1_tft_interlace_set(INT32U interlace);
extern void drv_l1_tft_vsync_unit_set(INT32U base_dclk);

extern void drv_l1_tft_data_mode_set(INT32U mode);
extern void drv_l1_tft_rb_switch_set(INT32U enable);
extern void drv_l1_tft_slide_en_set(INT32U enable);
extern void drv_l1_tft_dclk_sel_set(INT32U sel);
extern void drv_l1_tft_dclk_delay_set(INT32U delay);
extern void drv_l1_tft_display_buffer_set(INT32U addr);
extern void drv_l1_tft_reg_pol_set(INT8U pol);

extern void drv_l1_tft_i80_wr_cmd(INT16U command);
extern void drv_l1_tft_i80_wr_data(INT16U data);
extern INT16U drv_l1_tft_i80_read_data(INT16U command);
#endif 		/* __drv_l1_TFT_H__ */