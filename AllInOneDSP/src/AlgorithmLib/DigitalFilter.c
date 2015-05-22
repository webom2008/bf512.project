
#include "DigitalFilter.h"

static float s_dHrB[5][2] = \
{
    {-1.62978f,   0.85983f},
    {-1.43808f,   0.64107f},
    {-1.30728f,   0.49181f},
    {-1.22625f,   0.39933f},
    {-1.18755f,   0.35517f}
};

static float s_dHrB1[3] = \
{1.0f, 2.0f, 1.0f};

static float s_dHrGain[5] = \
{0.05751f, 0.05074f, 0.04613f, 0.04327f, 0.04190f};

// 500Hz采样率
int	Filt40HzLowPass(FILTER_BUFFER* pFt, int wave)
{
	int i,j;
    float X,Y,T;
    int F;

	X = (float)wave;
	for(i=0;i<5;i++)
	{
		for(j=2;j>0;j--)
		{
            pFt->fHrX[i][j] = pFt->fHrX[i][j-1];
		}
        pFt->fHrX[i][0] = X;
        Y = s_dHrB[i][0] * pFt->fHrY[i][0] + s_dHrB[i][1] * pFt->fHrY[i][1];
        X = (s_dHrB1[0] * pFt->fHrX[i][0] + s_dHrB1[1] * pFt->fHrX[i][1] + s_dHrB1[2] * pFt->fHrX[i][2]) * s_dHrGain[i];
		T = X - Y;
		
		for(j=2;j>0;j--)
		{
            pFt->fHrY[i][j] = pFt->fHrY[i][j-1];
		}
        pFt->fHrY[i][0] = T;
		X = T;
	}
	
    F = (int)T;
	
	return F;
}



// 360Hz采样率
static float s_dHrB_360Hz[5][2] = \
{
    {-1.62978f,   0.85983f},
    {-1.43808f,   0.64107f},
    {-1.30728f,   0.49181f},
    {-1.22625f,   0.39933f},
    {-1.18755f,   0.35517f}
};

static float s_dHrB1_360Hz[3] = \
{1.0f, 2.0f, 1.0f};

static float s_dHrGain_360Hz[5] = \
{0.05751f, 0.05074f, 0.04613f, 0.04327f, 0.04190f};

int	Filt40HzLowPass_360Hz(FILTER_BUFFER* pFt, int wave)
{
	int i,j;
    float X,Y,T;
    int F;

	X = (float)wave;
	for(i=0;i<5;i++)
	{
		for(j=2;j>0;j--)
		{
            pFt->fHrX[i][j] = pFt->fHrX[i][j-1];
		}
        pFt->fHrX[i][0] = X;
        Y = s_dHrB_360Hz[i][0] * pFt->fHrY[i][0] + s_dHrB_360Hz[i][1] * pFt->fHrY[i][1];
        X = (s_dHrB1_360Hz[0] * pFt->fHrX[i][0] + s_dHrB1_360Hz[1] * pFt->fHrX[i][1] + s_dHrB1_360Hz[2] * pFt->fHrX[i][2]) * s_dHrGain_360Hz[i];
		T = X - Y;
		
		for(j=2;j>0;j--)
		{
            pFt->fHrY[i][j] = pFt->fHrY[i][j-1];
		}
        pFt->fHrY[i][0] = T;
		X = T;
	}
	
    F = (int)T;
	
	return F;
}

// 诊断模式
// IIR，ButterWorth, BandPass
// Fs:	      500Hz
// Fstop1:     0.05Hz  -8.34dB
// Fpass1:     0.1Hz	-1.12dB	
// Fpass2:     30Hz    -1dB
// Fstop2:     150Hz	-28.5dB
// Order:		 4
// Sections:	 2

static float HrB_ZhenDuan[2][2]={
    {-1.99873,   0.99873},
    {-1.28366,   0.48000},
};
static float HrB1_ZhenDuan[3] = {1, 0, -1};
static float HrGainZhenDuan[2] = {0.22148, 0.22148};
int	Filter_ZhenDuan(FILTER_BUFFER_2* pFt, int wave)
{
	int i,j;
    float X,Y,T;
    int F;

	X = (float)wave;
	for(i = 0; i < 2; i++) {
		for(j = 2; j > 0; j--) {
            pFt->fHrX[i][j] = pFt->fHrX[i][j-1];
		}
        pFt->fHrX[i][0] = X;
        Y = HrB_ZhenDuan[i][0] * pFt->fHrY[i][0] + HrB_ZhenDuan[i][1] * pFt->fHrY[i][1];
        X = (HrB1_ZhenDuan[0] * pFt->fHrX[i][0] + HrB1_ZhenDuan[1] * pFt->fHrX[i][1] \
            + HrB1_ZhenDuan[2] * pFt->fHrX[i][2]) * HrGainZhenDuan[i];
		T = X - Y;
		
		for(j = 2; j > 0; j--) {
            pFt->fHrY[i][j] = pFt->fHrY[i][j-1];
		}
        pFt->fHrY[i][0] = T;
		X = T;
	}
	
    F = (int)T;
	
	return F;
}


// 监护模式
// IIR，ButterWorth, BandPass
// Fs:	      500Hz
// Fstop1:     0.115Hz  -20dB
// Fpass1:     0.5Hz	 -0.284dB	
// Fpass2:     20Hz     -0.277dB
// Fstop2:     40Hz	 -6.8dB
// Order:		 4
// Sections:	 2

static float HrB_Jianhu[2][2]={
    {-1.99453,   0.99455},
    {-1.56086,   0.65974},
};
static float HrB1_Jianhu[3] = {1, 0, -1};
static float HrGainJianhu[2] = {0.15647, 0.15647};
int	Filter_Jianhu(FILTER_BUFFER_2* pFt, int wave)
{
	int i,j;
    float X,Y,T;
    int F;

	X = (float)wave;
	for(i = 0; i < 2; i++) {
		for(j = 2; j > 0; j--) {
            pFt->fHrX[i][j] = pFt->fHrX[i][j-1];
		}
        pFt->fHrX[i][0] = X;
        Y = HrB_Jianhu[i][0] * pFt->fHrY[i][0] + HrB_Jianhu[i][1] * pFt->fHrY[i][1];
        X = (HrB1_Jianhu[0] * pFt->fHrX[i][0] + HrB1_Jianhu[1] * pFt->fHrX[i][1] \
            + HrB1_Jianhu[2] * pFt->fHrX[i][2]) * HrGainJianhu[i];
		T = X - Y;
		
		for(j = 2; j > 0; j--) {
            pFt->fHrY[i][j] = pFt->fHrY[i][j-1];
		}
        pFt->fHrY[i][0] = T;
		X = T;
	}
	
    F = (int)T;
	
	return F;
}



// 手术模式
// IIR，ButterWorth, BandPass
// Fs:	      500Hz
// Fstop1:     0.5Hz   -20dB
// Fpass1:     1Hz		-7.7dB	
// Fpass2:     10Hz    -0.62dB
// Fstop2:     40Hz	-20dB
// Order:		 4
// Sections:	 2

static float HrB_Shoushu[2][2]={
    {-1.97604,   0.97640},
    {-1.79202,   0.81635},
};
static float HrB1_Shoushu[3] = {1, 0, -1};
static float HrGainShoushu[2] = {0.07593, 0.07593};
int	Filter_Shoushu(FILTER_BUFFER_2* pFt, int wave)
{
	int i,j;
    float X,Y,T;
    int F;

	X = (float)wave;
	for(i = 0; i < 2; i++) {
		for(j = 2; j > 0; j--) {
            pFt->fHrX[i][j] = pFt->fHrX[i][j-1];
		}
        pFt->fHrX[i][0] = X;
        Y = HrB_Shoushu[i][0] * pFt->fHrY[i][0] + HrB_Shoushu[i][1] * pFt->fHrY[i][1];
        X = (HrB1_Shoushu[0] * pFt->fHrX[i][0] + HrB1_Shoushu[1] * pFt->fHrX[i][1] \
            + HrB1_Shoushu[2] * pFt->fHrX[i][2]) * HrGainShoushu[i];
		T = X - Y;
		
		for(j = 2; j > 0; j--) {
            pFt->fHrY[i][j] = pFt->fHrY[i][j-1];
		}
        pFt->fHrY[i][0] = T;
		X = T;
	}
	
    F = (int)T;
	
	return F;
}


// 工频陷波
// IIR，ButterWorth, BandStop
// Fs:	      500Hz
// Fc1:     49Hz   
// Fc2:     51Hz
// Magnitude:     -42dB   
// Order:		 2
// Sections:	 1

static float HrB_Notch50Hz[2] = {-1.59808, 0.97518};
static float HrB1_Notch50Hz[3] = {1,  -1.61816,  1};
static float HrGainNotch50Hz = 0.98759;

int	Filter_Notch50Hz(FILTER_BUFFER_1* pFt, int wave)
{
	
	int j;
    float X,Y,T;
    int F;

	X = (float)wave;

	for(j=2;j>0;j--)
	{
        pFt->fHrX[j] = pFt->fHrX[j-1];
	}
    pFt->fHrX[0] = X;
    Y = HrB_Notch50Hz[0] * pFt->fHrY[0] + HrB_Notch50Hz[1] * pFt->fHrY[1];
    X = (HrB1_Notch50Hz[0] * pFt->fHrX[0] + HrB1_Notch50Hz[1] * pFt->fHrX[1] \
        + HrB1_Notch50Hz[2] * pFt->fHrX[2]) * HrGainNotch50Hz;
	T = X - Y;
		
	for(j=2;j>0;j--)
	{
        pFt->fHrY[j] = pFt->fHrY[j-1];
	}
    pFt->fHrY[0] = T;
	X = T;
	
    F = (int)T;
	
	return F;
}

// 工频陷波
// IIR，ButterWorth, BandStop
// Fs:	      500Hz
// Fc1:     59Hz   
// Fc2:     61Hz
// Magnitude:     -45.9dB   
// Order:		 2
// Sections:	 1

static float HrB_Notch60Hz[2] = {-1.43996, 0.97518};
static float HrB1_Notch60Hz[3] = {1,  -1.45805,  1};
static float HrGainNotch60Hz = 0.98759;

int	Filter_Notch60Hz(FILTER_BUFFER_1* pFt, int wave)
{
	
	int j;
    float X,Y,T;
    int F;

	X = (float)wave;

	for(j=2;j>0;j--)
	{
        pFt->fHrX[j] = pFt->fHrX[j-1];
	}
    pFt->fHrX[0] = X;
    Y = HrB_Notch60Hz[0] * pFt->fHrY[0] + HrB_Notch60Hz[1] * pFt->fHrY[1];
    X = (HrB1_Notch60Hz[0] * pFt->fHrX[0] + HrB1_Notch60Hz[1] * pFt->fHrX[1] \
        + HrB1_Notch60Hz[2] * pFt->fHrX[2]) * HrGainNotch60Hz;
	T = X - Y;
		
	for(j=2;j>0;j--)
	{
        pFt->fHrY[j] = pFt->fHrY[j-1];
	}
    pFt->fHrY[0] = T;
	X = T;
	
    F = (int)T;
	
	return F;
}


// 新生儿模式
// IIR，ButterWorth, BandPass
// Fs:	     100Hz
// FC1:	     0.25Hz   
// FC2:       2.5Hz
// Mag:		3dB
// Order:		2
// Sections:	1
static const float HrB_Newborn[2] = {-1.86545, 0.86776};
static const float HrB1_Newborn[3] = {1, 0, -1};
static const float HrGainNewborn = 0.06612;
int	resp_newborn_filter(FILTER_BUFFER_1* pFt, int wave)
{
	int j;
    float X,Y,T;
    int F;

	X = (float)wave;

	for(j=2; j>0; j--)
	{
        pFt->fHrX[j] = pFt->fHrX[j-1];
	}
    pFt->fHrX[0] = X;
    Y = HrB_Newborn[0] * pFt->fHrY[0] + HrB_Newborn[1] * pFt->fHrY[1];
    X = (HrB1_Newborn[0] * pFt->fHrX[0] + HrB1_Newborn[1] * pFt->fHrX[1] + \
        HrB1_Newborn[2] * pFt->fHrX[2]) * HrGainNewborn;
	T = X - Y;
		
	for(j=2; j>0; j--)
	{
        pFt->fHrY[j] = pFt->fHrY[j-1];
	}
    pFt->fHrY[0] = T;
	X = T;
	
    F = (int)T;
	
	return F;
}

// 成人模式
// IIR，ButterWorth, BandPass
// Fs:	     100Hz
// FC1:	     0.1Hz   
// FC2:       2.5Hz
// Mag:		3dB
// Order:		2
// Sections:	1
static const float HrB_Adult[2] = {-1.85861, 0.85953};
static const float HrB1_Adult[3] = {1, 0, -1};
static const float HrGainAdult = 0.07024;
int	resp_adult_filter(FILTER_BUFFER_1* pFt, int wave)
{
	int j;
    float X,Y,T;
    int F;

	X = (float)wave;

	for(j=2; j>0; j--)
	{
        pFt->fHrX[j] = pFt->fHrX[j-1];
	}
    pFt->fHrX[0] = X;
    Y = HrB_Adult[0] * pFt->fHrY[0] + HrB_Adult[1] * pFt->fHrY[1];
    X = (HrB1_Adult[0] * pFt->fHrX[0] + HrB1_Adult[1] * pFt->fHrX[1] + \
        HrB1_Adult[2] * pFt->fHrX[2]) * HrGainAdult;
	T = X - Y;
		
	for(j=2; j>0; j--)
	{
        pFt->fHrY[j] = pFt->fHrY[j-1];
	}
    pFt->fHrY[0] = T;
	X = T;
	
    F = (int)T;
	
	return F;
}


