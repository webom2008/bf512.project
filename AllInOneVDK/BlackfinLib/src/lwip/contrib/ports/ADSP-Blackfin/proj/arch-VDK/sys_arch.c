
#include "lwip/sys.h"
#include "lwip/opt.h"
#include "lwip/stats.h"
#include <stdlib.h>
#include <string.h>

// information about each thread that lwIP creates
#include "sys_arch_thread.h"
#include <cglobals.h>
#include <kernel_abs.h>

//
// information about each mailbox that lwIP creates
//
#define SYS_MBOX_SIZE   128      // number of elements per mbox queue
struct sys_arch_mbox {
  ker_sem_id_t  id;
  s16_t           count;                // number of messages in queue
  s16_t           rndx;                 // index of next message to read
  s16_t           wndx;                 // index of next message to write
  void*           msgs[SYS_MBOX_SIZE];  // the message queue
  s16_t            valid;
};

// Keep track of opened semaphores
#define SYS_MAX_SEMAPHORES 40                   
static int sem_ids[SYS_MAX_SEMAPHORES];
err_t nifce_shutdown(int nwifce_no);

// head of lists of all thread structs, active or free
//
static struct   sys_arch_thread* thread_list;
static volatile sys_prot_t prot_level;

void
sys_init(void)
{
int i=0;

        thread_list = NULL;
        prot_level = 0; // presumably

        for(i=0;i<SYS_MAX_SEMAPHORES;i++)
                sem_ids[i] = -1;
}
//--------------------------------------------------------------------------
static void remove_zombie_threads(void)
{
struct sys_arch_thread* t,*p=NULL,*c=NULL;
    
    t = thread_list; 
            
    while(t != NULL)
    {   
        /* check if the thread id is valid */           
        if(ker_get_thread_status(t->id) == UINT_MAX)
        {
            c = t; 
            
            if(p==NULL) 
            { 
                t= t->next; 
                thread_list = t;                                            
            }
            else
            {
                p->next = t->next;
                t = t->next;                            
            }                                   
            
            c->next = NULL; 
            free(c);            
        }
        else
        {
            p = t;
            t = t->next;
        }
        
    }
    
}

//--------------------------------------------------------------------------
static struct sys_arch_thread*
new_sys_arch_thread(ker_thread_id_t id, void (*function)(void *arg), void *arg)
{
struct sys_arch_thread* t;
sys_prot_t old_level;

        old_level = sys_arch_protect();

        // check if the thread-id is already present. If so do not allocate 
        // another block.This is required because multiple id entries in the list
        // raises kernel panic whilst destroying the threads.   
        t = thread_list;  
        
        while (t) 
        {
                if (t->id == id)   
                        break;    
                t = t->next;    
        }

        if(t == NULL)
        {
                t = (struct sys_arch_thread*)calloc(1, sizeof(struct sys_arch_thread));
                LWIP_ASSERT("new_sys_arch_thread: struct allocated", t != NULL);

                t->id = id;
                t->function = function;
                t->arg = arg;

                t->next = thread_list;
                thread_list = t;
        }

        sys_arch_unprotect(old_level);

        return t;
}

//--------------------------------------------------------------------------
sys_thread_t sys_thread_new(const char *name, lwip_thread_fn thread, void *arg, int stacksize, int prio)
{
struct sys_arch_thread* t;
sys_prot_t old_level;

        old_level = sys_arch_protect();

        // find a free sys_arch_thread struct or create a new one
        t = thread_list;
        while (t) 
        {
                if (t->id == (ker_thread_id_t)UINT_MAX) 
                {
                        // stop optimiser throwing this assignment away
                        *(volatile ker_thread_id_t*)&t->id = (ker_thread_id_t)(UINT_MAX - 1);
                        break;
                }
                t = t->next;
        }

        if (t == NULL) 
        {
                t = new_sys_arch_thread((ker_thread_id_t)(UINT_MAX - 1), thread, arg);
        }
        else
        {
                t->function = thread;
                t->arg = arg;
        }
        sys_arch_unprotect(old_level);
        
        // Disabling the scheduler is critical here as we do not want schedule out
        // while t->id is being updated. sys_arch_timeouts() may also make an entry
        // into the thread list which may result in duplicate thread ids.
        ker_disable_scheduler();
        ker_create_thread((void*)thread,(void*)arg,0,0,prio,"L",(ker_thread_id_t*)&t->id);
        ker_enable_scheduler();
 
        return t;
}

//--------------------------------------------------------------------------
// this function either finds the currently running VDK thread in the list
// of sys_arch_threads or creates a new entry for it
//
// the function is global so that ErrorFunction() in VDK threads not created
// by sys_thread_new() can duplicate the action of ADI_TOOLS_IOEThread's
// function when a sys_arch_sem_wait() or sys_arch_mbox_fetch() times out;
// also the
// Destroy() function in such threads can mark the sys_arch_thread as free
// and thus avoid a memory leak
//
struct sys_arch_thread*
sys_arch_current_thread(void)
{
struct sys_arch_thread* t;
sys_prot_t old_level;
ker_thread_id_t id;

        // find out our VDK thread identifier
        id = ker_get_current_thread();

        old_level = sys_arch_protect();

        // see whether there is an entry for it in the thread list already
        for (t = thread_list; t != NULL; t = t->next) 
        {
                if (t->id == id)
                        break;
        }

        sys_arch_unprotect(old_level);

        // if we are an unknown VDK thread create a new sys_arch_thread for ourself
        if (t == NULL)
                t = new_sys_arch_thread(id, 0, 0);

        return t;
}


//--------------------------------------------------------------------------
struct sys_timeouts*
sys_arch_timeouts(void)
{
struct sys_arch_thread* t;
        remove_zombie_threads();
        t = sys_arch_current_thread();
        return (struct sys_timeouts*)&t->timeouts;
}

//--------------------------------------------------------------------------
err_t sys_sem_new(sys_sem_t *semp, u8_t count)
{
ker_sem_id_t sem;
int index,i;

#ifdef SYS_STATS
  lwip_stats.sys.sem.used++;
  if (lwip_stats.sys.sem.used > lwip_stats.sys.sem.max)
    lwip_stats.sys.sem.max = lwip_stats.sys.sem.used;
#endif /* SYS_STATS */

        LWIP_ASSERT("sys_sem_new: null pointer passed", semp != NULL);

        sem = ker_get_semaphore(count,UINT_MAX);

        // add only valid semaphores
        if(sem != ker_err_sem_cre)
        {
                for(i=0;i<SYS_MAX_SEMAPHORES;i++)
                {
                        if(sem_ids[i] == -1)
                                break;
                }

                if(i<SYS_MAX_SEMAPHORES)
                        sem_ids[i] = sem;
        }
        *semp = (sys_sem_t)sem;
        return (ERR_OK);
}

//--------------------------------------------------------------------------
void
sys_sem_free(sys_sem_t *semp)
{
sys_sem_t	sem;
int i;

        LWIP_ASSERT("sys_sem_free: invalid semaphore pointer", semp != NULL);

        sem = *semp;
        if (sem != SYS_SEM_NULL) 
        {
#ifdef SYS_STATS
    lwip_stats.sys.sem.used--;
#endif /* SYS_STATS */
                ker_rel_semaphore((ker_sem_id_t)sem);

                for(i=0;i<SYS_MAX_SEMAPHORES;i++)
                {
                        if(sem_ids[i] == (ker_sem_id_t)sem) 
                        {
                                sem_ids[i] = -1;
                                break;
                        }
                }
        }
}

//--------------------------------------------------------------------------
void
sys_sem_signal(sys_sem_t *semp)
{
        LWIP_ASSERT("sys_sem_signal: invalid semaphore pointer", semp != NULL);
        ker_post_semaphore((ker_sem_id_t)*semp);
}

//--------------------------------------------------------------------------
u32_t
sys_arch_sem_wait(sys_sem_t *semp, u32_t timeout)
{
unsigned int pend_time, resume_time;
int lastError;
u32_t time_spent;
kernel_results_e res;
sys_sem_t sem;

        LWIP_ASSERT("sys_sem_wait: invalid semaphore pointer", semp != NULL);

        sem = *semp;
        // get the current time
        pend_time = ker_get_systime();

        // wait for the semaphore
        res = (kernel_results_e)ker_pend_semaphore((ker_sem_id_t)sem,timeout);

        if((res == ker_err_sem_timeout) || (res == ker_err_sem_unavailable))
        return SYS_ARCH_TIMEOUT;

        resume_time = ker_get_systime();
        time_spent = resume_time - pend_time;

        return time_spent;
}
//--------------------------------------------------------------------------
int sys_sem_valid(sys_sem_t *semp)
{
    LWIP_ASSERT("sys_sem_valid: invalid semaphore pointer", semp != NULL);
    return ( ( *semp != 0) ? 1 : 0);
}
//--------------------------------------------------------------------------
void sys_sem_set_invalid(sys_sem_t *semp)
{
    LWIP_ASSERT("sys_sem_invalid: invalid semaphore pointer", semp != NULL);
    *semp = 0;
}
//--------------------------------------------------------------------------
err_t sys_mbox_new(sys_mbox_t *mbox, int size)
{
int i;

        LWIP_ASSERT("sys_mbox_new: invalid mbox pointer", mbox != NULL);
        LWIP_ASSERT("sys_mbox_new: size is 0", size != 0);
        
        struct sys_arch_mbox* m =
          (struct sys_arch_mbox*)calloc(size, sizeof(struct sys_arch_mbox));

        LWIP_ASSERT("sys_mbox_new: new mbox not allocated", m != NULL);

        // create associated VDK semaphore
        m->id  = ker_get_semaphore(0,SYS_MBOX_SIZE);

        // if valid semaphore
        if(m->id != ker_err_sem_cre)
        {
                for(i=0;i<SYS_MAX_SEMAPHORES;i++)
                {
                        if(sem_ids[i] == -1)
                                break;
                }
                if(i<SYS_MAX_SEMAPHORES)
                        sem_ids[i] = m->id;
        }
        else 
        {
                if(m != NULL)
                {
                        free(m);
                        m = NULL;
                }
        }
        
        m->valid = 1;

#ifdef SYS_STATS
  lwip_stats.sys.mbox.used++;
  if (lwip_stats.sys.mbox.used > lwip_stats.sys.mbox.max)
    lwip_stats.sys.mbox.max = lwip_stats.sys.mbox.used;
#endif /* SYS_STATS */

	*mbox = (sys_mbox_t *)m;
	return ERR_OK;
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
        LWIP_ASSERT("sys_mbox_valid: invalid mbox pointer", mbox != NULL);
	return( (*mbox != 0)? 1 : 0);
}

void sys_mbox_set_invalid(sys_mbox_t *mbox)
{
        LWIP_ASSERT("sys_mbox_set_invalid: invalid mbox pointer", mbox != NULL);
	*mbox = 0;
}


//--------------------------------------------------------------------------
void
sys_mbox_free(sys_mbox_t *mbox)
{
int i;
ker_sem_id_t sem;
struct sys_arch_mbox* m;

        LWIP_ASSERT("sys_mbox_free: invalid mbox", mbox != NULL);
        m = (struct sys_arch_mbox*)*mbox;

        sem = m->id;
        for(i=0;i< SYS_MAX_SEMAPHORES;i++)
        {
                if(sem_ids[i] == sem)
                        sem_ids[i] = -1;
        }

        ker_rel_semaphore((ker_sem_id_t)m->id);
        m->valid = 0;

    free(m);

#ifdef SYS_STATS
  lwip_stats.sys.mbox.used--;
#endif /* SYS_STATS */

}

err_t
sys_mbox_trypost(sys_mbox_t *mbox, void *msg)
{
sys_prot_t old_level;
struct sys_arch_mbox* m;

        LWIP_ASSERT("sys_mbox_trypost: invalid mbox", mbox != NULL);
        m = (struct sys_arch_mbox*)*mbox;
        old_level = sys_arch_protect();

        LWIP_DEBUGF(SYS_DEBUG, ("sys_mbox_trypost: mbox %p msg %p\n",
                          (void *)mbox, (void *)msg));  
        LWIP_ASSERT("sys_mbox_trypost: mbox is full", m->count < SYS_MBOX_SIZE);

        if (m->count >= SYS_MBOX_SIZE)
        {
            sys_arch_unprotect(old_level);
            return ERR_MEM;
        }
        
        m->msgs[m->wndx++] = msg;
        if (m->wndx == SYS_MBOX_SIZE)
                m->wndx = 0;
        m->count += 1;

        ker_post_semaphore((ker_sem_id_t)m->id);

        sys_arch_unprotect(old_level);
  
        return ERR_OK;
}

//--------------------------------------------------------------------------
void
sys_mbox_post(sys_mbox_t *mbox, void *msg)
{
sys_prot_t old_level;
struct sys_arch_mbox* m;

        LWIP_ASSERT("sys_mbox_post: invalid mbox", mbox != NULL);
        m = (struct sys_arch_mbox*)*mbox;
        old_level = sys_arch_protect();

        LWIP_ASSERT("sys_mbox_post: mbox not full", m->count < SYS_MBOX_SIZE);

        m->msgs[m->wndx++] = msg;
        if (m->wndx == SYS_MBOX_SIZE)
                m->wndx = 0;
        m->count += 1;

        ker_post_semaphore((ker_sem_id_t)m->id);

        sys_arch_unprotect(old_level);
}

//--------------------------------------------------------------------------
//
// A version of sys_mbox_post for use from ISRs. It does not invoke any
// support functions that might potentially attempt a reschedule.
//
void
sys_mbox_ISR_post(sys_mbox_t *mbox, void *msg)
{
struct sys_arch_mbox* m;

        LWIP_ASSERT("sys_mbox_ISR_post: mbox not full", mbox != NULL);
        m = (struct sys_arch_mbox*)*mbox;

        LWIP_ASSERT("sys_mbox_ISR_post: mbox not full", m->count < SYS_MBOX_SIZE);

        m->msgs[m->wndx++] = msg;
        if (m->wndx == SYS_MBOX_SIZE)
                m->wndx = 0;
        m->count += 1;

        ker_isr_post_semaphore((ker_sem_id_t)m->id);
}

//--------------------------------------------------------------------------
u32_t
sys_arch_mbox_fetch(sys_mbox_t *mbox, void **msg, u32_t timeout)
{
u32_t time_taken;
sys_prot_t old_level;
struct sys_arch_mbox* m = (struct sys_arch_mbox*)*mbox;

        LWIP_ASSERT("sys_arch_mbox_fetch: mbox not full", mbox != NULL);
        // wait for a message
        time_taken = sys_arch_sem_wait((sys_sem_t*)&m->id, timeout);

        old_level = sys_arch_protect();

        // if a message arrived return it to the caller
        if (time_taken != SYS_ARCH_TIMEOUT) 
        {
                if (msg != NULL)
                        *msg = m->msgs[m->rndx++];
                if (m->rndx == SYS_MBOX_SIZE)
                        m->rndx = 0;
                m->count -= 1;
        }

        sys_arch_unprotect(old_level);

        return time_taken;
}

//--------------------------------------------------------------------------
u32_t sys_arch_mbox_tryfetch(sys_mbox_t *mbox, void **msg)
{  
  kernel_results_e res;
  struct sys_arch_mbox* m;
  sys_prot_t old_level;
 
  LWIP_ASSERT("sys_arch_mbox_tryfetch: mbox not full", mbox != NULL);
  m = (struct sys_arch_mbox*)*mbox;
  res = (kernel_results_e)ker_pend_semaphore((ker_sem_id_t)m->id,UINT_MAX);

  if(res != ker_err_sem_unavailable)
  {
    old_level = sys_arch_protect();

    if(msg != NULL) 
    {
        *msg = m->msgs[m->rndx++];

        if (m->rndx == SYS_MBOX_SIZE)
            m->rndx = 0;

        m->count -= 1;
    }
    sys_arch_unprotect(old_level);

    return 0;
  }
  else /* TODO: check for other semaphore errors */
  {
      if(msg != NULL) 
         *msg  = NULL;

      return SYS_ARCH_TIMEOUT;
  }
}

//--------------------------------------------------------------------------
sys_prot_t
sys_arch_protect(void)
{
#ifdef ADI_PROTECT_USING_SCHEDULER
  ker_disable_scheduler();
#else
  ker_disable_interrupts(ker_kPriorityLevelAll);
#endif /* PROTECT_USING_SCHEDULER */

  prot_level += 1;
  return prot_level-1;
}

//--------------------------------------------------------------------------
void
sys_arch_unprotect(sys_prot_t pval)
{
  if (prot_level > pval) {
    prot_level -= 1;
#ifdef ADI_PROTECT_USING_SCHEDULER
     ker_enable_scheduler();
#else
     ker_enable_interrupts(ker_kPriorityLevelAll);
#endif /* PROTECT_USING_SCHEDULER */
  }
}

//------------------------------------------------------------------------

int stop_stack()
{
    int i;
    struct sys_arch_thread* t,*p;
    ker_thread_id_t id;
    sys_prot_t old_level;
    
    // find out VDK thread identifier
    id = ker_get_current_thread();

    // shutdown the network
    nifce_shutdown(0);
    
    ker_disable_scheduler();
    
    /* Enter critical region and disable scheduler */
    old_level = sys_arch_protect();
   
    // destroy all threads
    t = thread_list;
        
    while (t) 
    {
        if ((t->id != id) && (t->function != NULL))
        {
            ker_destroy_thread(t->id);
        }
        
        t=t->next;      
    }
    
    /* Enable Scheduler and exit critical region */
       
    sys_arch_unprotect(old_level);   
    ker_enable_scheduler();       
    
    // release all semaphores
    for(i=0;i< SYS_MAX_SEMAPHORES;i++)
    {
        if(sem_ids[i] != -1)
        {
            ker_rel_semaphore(sem_ids[i]);
        }
    }
        
    // release thread_list memory
    t = thread_list;
        
    while (t) 
    {
        p = t;
        t = t->next;
        free(p);                
    }
   
    thread_list = NULL;   
    
    return(1);    
}
