/**
  *(C) COPYRIGHT 2014 CVTE.SEECARE
  ******************************************************************************
  * @file    readme.txt
  * @author  CVTE.SEECARE.QiuWeibo
  * @version V1.0.0
  * @date    2014/01/21
  * @brief   
  ******************************************************************************
  */

@note
1. I2C-IAP protocal detail from:
	./doc/AN3078*.pdf "2.2 Software description"
2. I2C-IAP Read command Received valueis different with AN3078, as follows:
	master send : (Figure9 .1st sequence)
	slaver repond : Data[0]=0xAA, Data[1]=0x55 (Read_Memory_Command())


