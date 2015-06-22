/* =============================================================================
 *
 *  Description: This is a C++ implementation for Thread BootThread
 *
 * -----------------------------------------------------------------------------
 *  Comments:
 *
 * ===========================================================================*/

#include "BootThread.h"
#include "Config/includes.h"
#include <new>

#pragma file_attr("OS_Component=Threads")
#pragma file_attr("Threads")


static VDK::ThreadID Ledthread;

/******************************************************************************
 *  BootThread Run Function (BootThread's main{})
 */
 
void
BootThread::Run()
{
    // TODO - Put the thread's "main" Initialization HERE

    Ledthread = VDK::CreateThread( kLedThread );
    VDK::PendSemaphore( kLedThreadInitDone, 0);

    while (1)
    {
        fprintf(stdout,"\r\n>>BootThread Running...");
        VDK::Sleep(5000u);
    }

    // TODO - Put the thread's exit from "main" HERE
	// A thread is automatically Destroyed when it exits its run function
}

/******************************************************************************
 *  BootThread Error Handler
 */
 
int
BootThread::ErrorHandler()
{
    /* TODO - Put this thread's error handling code HERE */

    /* The default ErrorHandler (called below)  raises
     * a kernel panic and stops the system */
    return (VDK::Thread::ErrorHandler());
}

/******************************************************************************
 *  BootThread Constructor
 */
 
BootThread::BootThread(VDK::Thread::ThreadCreationBlock &tcb)
    : VDK::Thread(tcb)
{
    /* This routine does NOT run in new thread's context.  Any non-static thread
     *   initialization should be performed at the beginning of "Run()."
     */

    // TODO - Put code to be executed when this thread has just been created HERE

}

/******************************************************************************
 *  BootThread Destructor
 */
 
BootThread::~BootThread()
{
    /* This routine does NOT run in the thread's context.  Any VDK API calls
     *   should be performed at the end of "Run()."
     */

    // TODO - Put code to be executed just before this thread is destroyed HERE

}

/******************************************************************************
 *  BootThread Externally Accessible, Pre-Constructor Create Function
 */
 
VDK::Thread*
BootThread::Create(VDK::Thread::ThreadCreationBlock &tcb)
{
    /* This routine does NOT run in new thread's context.  Any non-static thread
     *   initialization should be performed at the beginning of "Run()."
     */

    	return new (tcb) BootThread(tcb);
}

/* ========================================================================== */
