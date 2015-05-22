#ifndef SYSTEM_TWI_H_
#define SYSTEM_TWI_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

extern void SystemTWI_Init(void);
extern int TWI_MasterMode_Read( const unsigned short DeviceAddr,
                                unsigned char *TWI_Data_Pointer,
                                const unsigned short TWI_Length);
extern int TWI_MasterMode_Write(const unsigned short DeviceAddr,
                                unsigned char *TWI_Data_Pointer,
                                const unsigned short Count,
                                const unsigned short TWI_Length);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_TWI_H_ */

