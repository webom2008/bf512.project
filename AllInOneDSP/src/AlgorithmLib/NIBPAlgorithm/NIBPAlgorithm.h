/*****************************************************************************
 * NIBPAlgorithm.h
 *****************************************************************************/
#ifndef NIBP_ALGORITHM_H_
#define NIBP_ALGORITHM_H_
      
#ifdef __cplusplus
     extern "C" {
#endif

/**
  * @brief  :
  *         
  * @param  :void
  * @retval :void
  */
extern void l_nibp_AlgorithmInit(void);

/**
  */
extern int l_nibp_PushAlgorithmData(u32* pu32Tick, u32* pu32mmHg, u16* pu32mmHgRising);

/**
  */
extern int l_nibp_PopAlgorithmData(void *pResult);

extern int l_nibp_SetAlgorithmPatient(char newType);

extern void l_nibp_pushLastRet(int *pDP, int *pSP, int *pBPM);

#ifdef __cplusplus
}
#endif
 
#endif /* NIBP_ALGORITHM_H_ */

