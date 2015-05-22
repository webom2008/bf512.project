/*****************************************************************************
 * ECGAlgorithm.h
 *****************************************************************************/
#ifndef ECG_ALGORITHM_H_
#define ECG_ALGORITHM_H_
 
#ifdef __cplusplus
extern "C" {
#endif 

/**
  * @brief  :ECG�㷨���ʼ��
  *         
  * @param  :void
  * @retval :void
  */
extern void l_ecg_AlgorithmInit(void);

/**
  * @brief  :ECG�㷨������ӿ�
  *         
  * @param  :u32* pu32Tick,����,ϵͳʱ��
  *         :s32* ps32I,����,ECG-I ������
  *         :s32* ps32II,����,ECG-II ������
  *         :s32* ps32V,����,ECG-V ������
  * @retval :int,0���� -1�쳣
  */
extern int l_ecg_PushAlgorithmData(u32* pu32Tick, s32* ps32I, s32* ps32II, s32* ps32V, char* pPace);

/**
  * @brief  :ECG�㷨������ӿ�
  *         
  * @param  :void* pBuf,���,ָ��CECG_Out_Item��ָ��
  *         :int i32CECGOutItemNum,����,CECG_Out_Item�ĸ���
  * @retval :int,ʵ�ʻ�ȡ�㷨���CECG_Out_Item�ĸ���
  */
extern int l_ecg_PopAlgorithmData(void* pBuf, int i32CECGOutItemNum);

extern int l_ecg_DebugInterface(ECG_ALG_DEBUG_TypeDef type, void* pBuf, unsigned char nLen);

#ifdef __cplusplus
}
#endif
 
#endif /* ECG_ALGORITHM_H_ */

