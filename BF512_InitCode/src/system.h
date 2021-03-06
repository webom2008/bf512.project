/*****************************************************************************
 Prototypes
******************************************************************************/

void full_on(void);
u32 get_vco_hz(void);
u32 get_cclk_hz(void);
u32 get_sclk_hz(void);
void async_mem_en(void);
void sdram_en(void);
u16 get_rdiv (void);
ERROR_CODE verify_clocks(u8, u8, u8);

/****************************************************************************
 EOF
*****************************************************************************/
