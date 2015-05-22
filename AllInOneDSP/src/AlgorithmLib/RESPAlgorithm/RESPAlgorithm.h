/*****************************************************************************
 * RESPAlgorithm.h
 *****************************************************************************/
#ifndef RESP_ALGORITHM_H_
#define RESP_ALGORITHM_H_
      
#ifdef __cplusplus
     extern "C" {
#endif

#if 1//_RESP_ALGORITHM_C_

/**
  * @brief  :RESPÀ„∑®ø‚≥ı ºªØ
  *         
  * @param  :void
  * @retval :void
  */
extern void l_resp_AlgorithmInit(void);

/**
  */
extern int l_resp_PushAlgorithmData(u32* pu32Tick, s32* ps32resp);

/**
  */
extern int l_resp_PopAlgorithmData(void* pBuf, int i32CRESPOutItemNum);


extern int l_resp_DebugInterface(RESP_ALG_DEBUG_TypeDef type, char* pBuf, unsigned char nLen);

#endif //_RESP_ALGORITHM_C_

#ifdef __cplusplus
}
#endif
 
#endif /* RESP_ALGORITHM_H_ */

