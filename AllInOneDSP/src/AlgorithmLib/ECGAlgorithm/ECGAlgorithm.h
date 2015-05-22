/*****************************************************************************
 * ECGAlgorithm.h
 *****************************************************************************/
#ifndef ECG_ALGORITHM_H_
#define ECG_ALGORITHM_H_
 
#ifdef __cplusplus
extern "C" {
#endif 

/**
  * @brief  :ECG算法库初始化
  *         
  * @param  :void
  * @retval :void
  */
extern void l_ecg_AlgorithmInit(void);

/**
  * @brief  :ECG算法库输入接口
  *         
  * @param  :u32* pu32Tick,输入,系统时标
  *         :s32* ps32I,输入,ECG-I 的数据
  *         :s32* ps32II,输入,ECG-II 的数据
  *         :s32* ps32V,输入,ECG-V 的数据
  * @retval :int,0正常 -1异常
  */
extern int l_ecg_PushAlgorithmData(u32* pu32Tick, s32* ps32I, s32* ps32II, s32* ps32V, char* pPace);

/**
  * @brief  :ECG算法库输出接口
  *         
  * @param  :void* pBuf,输出,指向CECG_Out_Item的指针
  *         :int i32CECGOutItemNum,输入,CECG_Out_Item的个数
  * @retval :int,实际获取算法库的CECG_Out_Item的个数
  */
extern int l_ecg_PopAlgorithmData(void* pBuf, int i32CECGOutItemNum);

extern int l_ecg_DebugInterface(ECG_ALG_DEBUG_TypeDef type, void* pBuf, unsigned char nLen);

#ifdef __cplusplus
}
#endif
 
#endif /* ECG_ALGORITHM_H_ */

