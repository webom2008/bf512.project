/*****************************************************************************
 Symbolic constants / definitions
******************************************************************************/

#define PRESCALE_VALUE 8    /* PRESCALE = 80/10 = 8 */
#define TWI_RESET_VAL 0     /* RESET_TWI value for controller */
#define CLKDIV_HI 66        /* SCL high period */
#define CLKDIV_LO 67        /* SCL low period */

/*****************************************************************************
 Prototypes / Functions
******************************************************************************/

void twi_reset(void);
void twi_mastermode_write(u16, u8*, u16, u16);
void twi_mastermode_read(u16, u8*, u16);

/****************************************************************************
 EOF
*****************************************************************************/
