
/*****************************************************************************
 * DigitalFilter.h
 *****************************************************************************/
#ifndef _DIGITALFILTER_H_
#define _DIGITALFILTER_H_
 
#ifdef __cplusplus
extern "C" {
#endif 

typedef struct _FILTER_BUFFER
{
	float fHrX[5][3];
	float fHrY[5][3];
} FILTER_BUFFER;


typedef struct _FILTER_BUFFER_1
{
	float fHrX[3];
	float fHrY[3];
} FILTER_BUFFER_1;


typedef struct _FILTER_BUFFER_2
{
	float fHrX[2][3];
	float fHrY[2][3];
} FILTER_BUFFER_2;

extern int	Filt40HzLowPass(FILTER_BUFFER* pFt, int wave);

extern int	Filt40HzLowPass_360Hz(FILTER_BUFFER* pFt, int wave);

extern int	Filter_ZhenDuan(FILTER_BUFFER_2* pFt, int wave);
extern int	Filter_Jianhu(FILTER_BUFFER_2* pFt, int wave);
extern int	Filter_Shoushu(FILTER_BUFFER_2* pFt, int wave);

extern int	Filter_Notch50Hz(FILTER_BUFFER_1* pFt, int wave);
extern int	Filter_Notch60Hz(FILTER_BUFFER_1* pFt, int wave);

extern int	resp_newborn_filter(FILTER_BUFFER_1* pFt, int wave);
extern int	resp_adult_filter(FILTER_BUFFER_1* pFt, int wave);


#ifdef __cplusplus
}
#endif

#endif /* _DIGITALFILTER_H_ */

