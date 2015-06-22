/* =============================================================================
 *
 *  Description: This is a C++ header file for Thread LedThread
 *
 * -----------------------------------------------------------------------------
 *  Comments:
 *
 * ===========================================================================*/

#ifndef _LedThread_H_    
#define _LedThread_H_

#pragma diag(push)
#pragma diag(suppress: 177,401,451,826,831,1462)

#include "VDK.h"

#pragma diag(pop)

class LedThread : public VDK::Thread 
{
public:
    LedThread(VDK::Thread::ThreadCreationBlock&);
    virtual ~LedThread();
    virtual void Run();
    virtual int ErrorHandler();
    static VDK::Thread* Create(VDK::Thread::ThreadCreationBlock&);
};

#endif /* _LedThread_H_ */

/* ========================================================================== */
