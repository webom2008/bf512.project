/*
 * module_ibp.h
 *
 *  Created on: 2013-9-12
 *      Author: QiuWeibo
 */

#ifndef MODULE_IBP_H_
#define MODULE_IBP_H_

#ifdef __cplusplus
 extern "C" {
#endif 

#include "AllInOneDSP_conf.h"

extern bool IsIBPSample;
extern unsigned long g_IBPadc_Val[2];//分别存IBP1和IBP2的值

extern void IBP_Init(void);
extern void IBP_Handler(void);
extern void IBPReslultUpload(void);


#ifdef __cplusplus
}
#endif

#endif /* MODULE_IBP_H_ */
