/* =============================================================================
 *
 *  Description: This is a C++ implementation for Thread LedThread
 *
 * -----------------------------------------------------------------------------
 *  Comments:
 *
 * ===========================================================================*/
#include "LedThread.h"
#include "Config/includes.h"
#include <new>

#pragma file_attr("OS_Component=Threads")
#pragma file_attr("Threads")

/******************************************************************************
 *  LedThread Run Function (LedThread's main{})
 */
 
void
LedThread::Run()
{
    ADI_FLAG_RESULT   flagResult  = ADI_FLAG_RESULT_SUCCESS;
    
    flagResult = adi_flag_Open (ADI_FLAG_PF6);
    while (ADI_FLAG_RESULT_SUCCESS != flagResult){};
    flagResult = adi_flag_SetDirection(ADI_FLAG_PF6, ADI_FLAG_DIRECTION_OUTPUT);
    while (ADI_FLAG_RESULT_SUCCESS != flagResult){};
    flagResult = adi_flag_Set(ADI_FLAG_PF6);
    while (ADI_FLAG_RESULT_SUCCESS != flagResult){};
//    flagResult = adi_flag_Clear(ADI_FLAG_PF6);
//    while (ADI_FLAG_RESULT_SUCCESS != flagResult){};
    
    VDK::PostSemaphore( kLedThreadInitDone );
//    VDK::Sleep(2500u);
    while (ADI_FLAG_RESULT_SUCCESS == flagResult)
    {
        fprintf(stdout,"\r\n>>LedThread Running...");
        flagResult = adi_flag_Toggle (ADI_FLAG_PF6);
        VDK::Sleep(5000u);
    }
}

/******************************************************************************
 *  LedThread Error Handler
 */
 
int
LedThread::ErrorHandler()
{
    /* TODO - Put this thread's error handling code HERE */

    /* The default ErrorHandler (called below)  raises
     * a kernel panic and stops the system */
    return (VDK::Thread::ErrorHandler());
}

/******************************************************************************
 *  LedThread Constructor
 */
 
LedThread::LedThread(VDK::Thread::ThreadCreationBlock &tcb)
    : VDK::Thread(tcb)
{
    /* This routine does NOT run in new thread's context.  Any non-static thread
     *   initialization should be performed at the beginning of "Run()."
     */

    // TODO - Put code to be executed when this thread has just been created HERE

}

/******************************************************************************
 *  LedThread Destructor
 */
 
LedThread::~LedThread()
{
    /* This routine does NOT run in the thread's context.  Any VDK API calls
     *   should be performed at the end of "Run()."
     */

    // TODO - Put code to be executed just before this thread is destroyed HERE

}

/******************************************************************************
 *  LedThread Externally Accessible, Pre-Constructor Create Function
 */
 
VDK::Thread*
LedThread::Create(VDK::Thread::ThreadCreationBlock &tcb)
{
    /* This routine does NOT run in new thread's context.  Any non-static thread
     *   initialization should be performed at the beginning of "Run()."
     */

    	return new (tcb) LedThread(tcb);
}

/* ========================================================================== */
