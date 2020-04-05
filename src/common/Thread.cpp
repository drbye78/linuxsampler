/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2020 Christian Schoenebeck                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#include "Thread.h"

#if HAVE_CONFIG_H
# include <config.h>
#endif
#include "global_private.h"

#include <list>
#if DEBUG
# include <assert.h>
#endif

#if !CONFIG_PTHREAD_TESTCANCEL
# warning No pthread_testcancel() available: this may lead to mutex dead locks when threads are stopped!
#endif

// this is the minimum stack size a thread will be spawned with
// if this value is too small, the OS will allocate memory on demand and
// thus might lead to dropouts in realtime threads
// TODO: should be up for testing to get a reasonable good value
#define MIN_STACK_SIZE		524288

#if !defined(WIN32)
static thread_local std::list<int> cancelStates;
#endif

namespace LinuxSampler {

Thread::Thread(bool LockMemory, bool RealTime, int PriorityMax, int PriorityDelta) {
    this->bLockedMemory     = LockMemory;
    this->isRealTime        = RealTime;
    this->PriorityDelta     = PriorityDelta;
    this->PriorityMax       = PriorityMax;
    this->state = NOT_RUNNING;
#if defined(WIN32)
# if defined(WIN32_SIGNALSTARTTHREAD_WORKAROUND)
    win32isRunning = false;
# endif
#else
    __thread_destructor_key = 0;
    pthread_attr_init(&__thread_attr);
#endif
}

Thread::~Thread() {
    // The thread must no longer be running at this point, otherwise it is an
    // error (we should avoid an implied call of StopThread() in the destructor,
    // because trying to do so might cause undefined behavior).
#if DEBUG
    assert(!RunningCondition.GetUnsafe());
#else
    if (RunningCondition.GetUnsafe()) {
        std::cerr << "WARNING: Thread destructed while still running!\n" << std::flush;
        StopThread();
    }
#endif
#if !defined(WIN32)
    pthread_attr_destroy(&__thread_attr);
#endif
}

/**
 *  Starts the thread synchronously. This method will block until the thread
 *  actually started it's execution before it will return. The abstract method
 *  Main() is the entry point for the new thread. You have to implement the
 *  Main() method in your subclass.
 *
 *  If this thread is already running when this method is called, then this
 *  method will detect this and return accordingly without further actions.
 *
 *  @returns   0 on success, any other value if thread could not be launched
 */
int Thread::StartThread() {
    int res = -1;
#if defined (WIN32_SIGNALSTARTTHREAD_WORKAROUND)
    // poll the win32isRunning variable and sleep 1msec inbetween
    if(!win32isRunning) {
        res = SignalStartThread();
        if (res == 0) {
            while (true) {
                Sleep(1);
                if (win32isRunning) break;
            }
        }
    } else res = 0;
#else
    LockGuard g(RunningCondition);
    // If the thread terminated on its own (i.e. returned from Main()) without
    // any thread calling StopThread() yet, then the OS blocks termination of
    // the thread waiting for a pthread_join() call. So we must detach the
    // thread in this case, because otherwise it will cause a thread leak.
    if (state == PENDING_JOIN) {
        state = DETACHED;
        #if !defined(WIN32)
        pthread_detach(__thread_id);
        #endif
    }
    if (!RunningCondition.GetUnsafe()) {
        res = SignalStartThread();
        // if thread was triggered successfully, wait until thread actually
        // started execution
        if (res == 0)
            RunningCondition.PreLockedWaitIf(false);
    } else {
        res = 0;
    }
#endif
    return res;
}

/**
 *  Starts the thread. This method will signal to start the thread and
 *  return immediately. Note that the thread might not yet run when this
 *  method returns! The abstract method Main() is the entry point for the
 *  new thread. You have to implement the Main() method in your subclass.
 *
 *  @b IMPORTANT: Calling this method assumes that this thread is not yet
 *  running! Calling this method if the thread is already running causes
 *  undefined behavior!
 *
 *  @see StartThread()
 */
int Thread::SignalStartThread() {
    state = RUNNING;
#if defined(WIN32)
    LPVOID lpParameter;
    hThread = CreateThread(
               NULL, // no security attributes
               MIN_STACK_SIZE,
               win32threadLauncher,
               this,
               0,
               &lpThreadId);
    if(hThread == NULL) {
        std::cerr << "Thread creation failed: Error" << GetLastError() << std::endl << std::flush;
        #if defined(WIN32_SIGNALSTARTTHREAD_WORKAROUND)
        win32isRunning = false;
        #else
        RunningCondition.Set(false);
        #endif
        return -1;
    }
    return 0;
#else
    // prepare the thread properties
    int res = pthread_attr_setinheritsched(&__thread_attr, PTHREAD_EXPLICIT_SCHED);
    if (res) {
        std::cerr << "Thread creation failed: Could not inherit thread properties."
                  << std::endl << std::flush;
        RunningCondition.Set(false);
        return res;
    }
    res = pthread_attr_setdetachstate(&__thread_attr, PTHREAD_CREATE_JOINABLE);
    if (res) {
        std::cerr << "Thread creation failed: Could not request a joinable thread."
                  << std::endl << std::flush;
        RunningCondition.Set(false);
        return res;
    }
    res = pthread_attr_setscope(&__thread_attr, PTHREAD_SCOPE_SYSTEM);
    if (res) {
        std::cerr << "Thread creation failed: Could not request system scope for thread scheduling."
                  << std::endl << std::flush;
        RunningCondition.Set(false);
        return res;
    }
    res = pthread_attr_setstacksize(&__thread_attr, MIN_STACK_SIZE);
    if (res) {
        std::cerr << "Thread creation failed: Could not set minimum stack size."
                  << std::endl << std::flush;
        RunningCondition.Set(false);
        return res;
    }

    // Create and run the thread
    res = pthread_create(&this->__thread_id, &__thread_attr, pthreadLauncher, this);
    switch (res) {
        case 0: // Success
            break;
        case EAGAIN:
            std::cerr << "Thread creation failed: System doesn't allow to create another thread."
                      << std::endl << std::flush;
            RunningCondition.Set(false);
            break;
        case EPERM:
            std::cerr << "Thread creation failed: You're lacking permisssions to set required scheduling policy and parameters."
                      << std::endl << std::flush;
            RunningCondition.Set(false);
            break;
        default:
            std::cerr << "Thread creation failed: Unknown cause."
                      << std::endl << std::flush;
            RunningCondition.Set(false);
            break;
    }
    return res;
#endif
}

/**
 *  Stops the thread synchronously. This method will block until the thread
 *  actually stopped its execution before it will return from this method.
 *
 *  If the thread is not running when calling this method, this will be detected
 *  and the call will be ignored. So it is safe to call this method both if the
 *  thread never started, as well as if the thread has already been stopped. And
 *  in fact you should explicitly call StopThread() before the Thread object is
 *  going to be destructured!
 *
 *  @see SignalStopThread()
 */
int Thread::StopThread() {
#if defined(WIN32_SIGNALSTARTTHREAD_WORKAROUND)
    SignalStopThread();
    win32isRunning = false;
    return 0;
#else
    // LockGuard cannot be used here, because this is a bit more tricky here
    RunningCondition.Lock();
    #if !defined(WIN32)
    // if thread was calling StopThread() on itself
    if (pthread_equal(__thread_id, pthread_self())) {
        RunningCondition.PreLockedSet(false);
        state = DETACHED;
        pthread_detach(__thread_id);
        RunningCondition.Unlock();
        pthread_exit(NULL);
    }
    #endif
    // if we are here, then any other thread called StopThread() but not the thread itself
    if (RunningCondition.GetUnsafe()) {
        SignalStopThread();
        // wait until thread stopped execution
        RunningCondition.PreLockedWaitAndUnlockIf(true);
        #if !defined(WIN32)
        pthread_join(__thread_id, NULL);
        #endif
        RunningCondition.Lock();
    }
    // If the thread terminated on its own (i.e. returned from Main()) without
    // any thread calling StopThread() yet, then the OS blocks termination of
    // the thread waiting for a pthread_join() call. So we must detach the
    // thread in this case, because otherwise it will cause a thread leak.
    if (state == PENDING_JOIN) {
        state = DETACHED;
        #if !defined(WIN32)
        pthread_detach(__thread_id);
        #endif
    }
    RunningCondition.Unlock();
    return 0;
#endif
}

/**
 *  Stops the thread asynchronously. This method will signal to stop the thread
 *  and return immediately. Note that due to this the thread might still run
 *  when this method returns!
 *
 *  @b IMPORTANT: You @ MUST still call StopThread() before destructing the
 *  Thread object, even if you called SignalStopThread() before and the thread
 *  is no longer running! Otherwise this may lead to a thread leak!
 *
 *  @see StopThread()
 */
int Thread::SignalStopThread() {
    //FIXME: segfaults when thread is not yet running
#if defined(WIN32)
    BOOL res;
    res = TerminateThread(hThread, 0); // we set ExitCode to 0
    //res = WaitForSingleObject( hThread, INFINITE);
    //myprint(("Thread::SignalStopThread:  WaitForSingleObject( hThread, INFINITE) res=%d\n",res));
    #if defined(WIN32_SIGNALSTARTTHREAD_WORKAROUND)
    win32isRunning = false;
    #else
    RunningCondition.Set(false);
    #endif
#else
    pthread_cancel(__thread_id);
#endif	
    return 0;
}

/**
 * Returns @c true in case the thread is currently running. This method does not
 * block and returns immediately.
 *
 * Note that no synchronization is performed when calling this method. So the
 * returned result is a very volatile information which must be processed with
 * precautions, that is it may not be used for code that might cause a race
 * condition.
 */
bool Thread::IsRunning() {
    #if defined(WIN32_SIGNALSTARTTHREAD_WORKAROUND)
    return win32isRunning;
    #else
    return RunningCondition.GetUnsafe();
    #endif
}

/**
 *  Sets the process SCHED_FIFO policy,  if max=1 then set at max priority,
 *  else use min priority. delta is added to the priority so that we can
 *  for example set 3 SCHED_FIFO tasks to different priorities by specifying
 *  delta  0 , -1 , -2  ( 0 = highest priority because -1 is subtracted to the
 *  current priority).
 */
int Thread::SetSchedulingPriority() {
#if defined(WIN32)
    DWORD dwPriorityClass;
    int nPriority;

    if(isRealTime) {
        dwPriorityClass = REALTIME_PRIORITY_CLASS;
        if (this->PriorityMax == 1) {
            if(this->PriorityDelta == 0) nPriority = THREAD_PRIORITY_TIME_CRITICAL;
            else nPriority = 7 + this->PriorityDelta;
        }
        else nPriority = THREAD_PRIORITY_NORMAL + this->PriorityDelta;
    }
    else {
        dwPriorityClass = NORMAL_PRIORITY_CLASS;
        nPriority = THREAD_PRIORITY_NORMAL + this->PriorityDelta;
    }

    BOOL res;
    // FIXME: priority class (realtime) does not work yet, gives error. check why.
    #if 0
    res = SetPriorityClass( hThread, dwPriorityClass );
    if(res == false) {
        std::cerr << "Thread: WARNING, setPriorityClass " << dwPriorityClass << "failed. Error " << GetLastError() << "\n";
        return -1;
    }

    res = SetThreadPriority( hThread, nPriority );
    if(res == false) {
        std::cerr << "Thread: WARNING, setThreadPriority " << nPriority << "failed. Error " << GetLastError() << "\n";
        return -1;
    }
    #endif
    return 0;
#else
#if !defined(__APPLE__)
    int policy;
    const char* policyDescription = NULL;
    if (isRealTime) { // becomes a RT thread
        policy = SCHED_FIFO;
        policyDescription = "realtime";
    } else { // 'normal', non-RT thread
        policy = SCHED_OTHER;
        policyDescription = "normal (non-RT)";
    }
    // set selected scheduling policy and priority
    struct sched_param schp;
    memset(&schp, 0, sizeof(schp));
    if (isRealTime) { // it is not possible to change priority for the SCHED_OTHER policy
        if (this->PriorityMax == 1) {
            schp.sched_priority = sched_get_priority_max(policy) + this->PriorityDelta;
        }
        if (this->PriorityMax == -1) {
            schp.sched_priority = sched_get_priority_min(policy) + this->PriorityDelta;
        }
    }
    if (pthread_setschedparam(__thread_id, policy, &schp) != 0) {
        std::cerr << "Thread: WARNING, can't assign "
                  << policyDescription
                  << " scheduling to thread!"
                  << std::endl << std::flush;
        return -1;
    }
#endif
    return 0;
#endif	
}

/**
 * Locks the memory so it will not be swapped out by the operating system.
 */
int Thread::LockMemory() {
#if defined(WIN32)
    return 0;
#else
#if !defined(__APPLE__)
    if (!bLockedMemory) return 0;
    if (mlockall(MCL_CURRENT | MCL_FUTURE) < 0) {
        std::cerr << "Thread: WARNING, can't mlockall() memory!\n"
                  << std::flush;
        return -1;
    }
#endif
    return 0;
#endif	
}

/**
 *  Registers thread destructor callback function which will be executed when
 *  the thread stops it's execution and sets the 'Running' flag to true. This
 *  method will be called by the pthreadLauncher callback function, DO NOT
 *  CALL THIS METHOD YOURSELF!
 */
void Thread::EnableDestructor() {
#if defined(WIN32_SIGNALSTARTTHREAD_WORKAROUND)
    win32isRunning = true;
    return;	
#endif
    LockGuard g(RunningCondition);
#if !defined(WIN32)
    pthread_key_create(&__thread_destructor_key, pthreadDestructor);
    pthread_setspecific(__thread_destructor_key, this);
#endif
    RunningCondition.PreLockedSet(true);
}

/**
 *  May be overridden by deriving classes to add additional custom cleanup
 *  code if necessary for the event when thread terminates. Currently this
 *  default implementation does nothing.
 */
int Thread::onThreadEnd() {
    return 0;
}

void Thread::TestCancel() {
#if !defined(WIN32)
    pthread_testcancel();
#endif
}

#if defined(WIN32)
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2)
// make sure stack is 16-byte aligned for SSE instructions
__attribute__((force_align_arg_pointer))
#endif
DWORD WINAPI Thread::win32threadLauncher(LPVOID lpParameter) {
    Thread* t;
    t = (Thread*) lpParameter;
    t->SetSchedulingPriority();
    t->LockMemory();
    t->EnableDestructor();
    t->Main();
    return 0;
}
#else
/// Callback function for the POSIX thread API
void* Thread::pthreadLauncher(void* thread) {
#if !CONFIG_PTHREAD_TESTCANCEL
    // let the thread be killable under any circumstances
    if (pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL)) {
        std::cerr << "Thread: WARNING, PTHREAD_CANCEL_ASYNCHRONOUS not supported!\n" << std::flush;
    }
#endif
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    Thread* t;
    t = (Thread*) thread;
    t->SetSchedulingPriority();
    t->LockMemory();
    t->EnableDestructor();
    t->Main();
    return NULL;
}
#endif

#if !defined(WIN32)
/// Callback function for the POSIX thread API
void Thread::pthreadDestructor(void* thread) {
    Thread* t;
    t = (Thread*) thread;
    LockGuard g(t->RunningCondition);
    t->onThreadEnd();
    pthread_key_delete(t->__thread_destructor_key);
    // inform that thread termination blocks waiting for pthread_join()
    // (not detaching the thread here already, because otherwise this might lead
    // to a data race of the vpointer with the Thread object destructor)
    t->state = PENDING_JOIN;
    t->RunningCondition.PreLockedSet(false);
}
#endif

/**
 * Allow or prohibit whether the calling thread may be cancelled, remember its
 * previous setting on a stack.
 *
 * @discussion The POSIX standard defines certaing system calls as implied
 * thread cancellation points. For instance when a thread calls @c usleep(), the
 * thread would immediately terminate if the thread was requested to be stopped.
 * The problem with this is that a thread typically has critical sections where
 * it may not simply terminate unexpectedly, e.g. if a thread is currently
 * holding a mutex lock and then would call @c usleep() this may end up in a
 * dead lock, since the lock would then never be unlocked again. For that reason
 * a thread should first disable itself being cancelable before entering a
 * critical section by calling @c pushCancelable(false) and it should
 * re-enable thread cancelation by calling @c popCancelable() after having left
 * the critical section(s). Refer to the following link for a list of functions
 * being defined as implied cancelation points:
 * http://man7.org/linux/man-pages/man7/pthreads.7.html
 *
 * @b NOTE: This method is currently not implemented for Windows yet!
 *
 * @param cancel - @c true: thread may be cancelled, @c false: thread may not
 *                 be cancelled until re-enabled with either pushCancelable() or
 *                 popCancelable()
 * @see popCancelable() as counter part
 */
void Thread::pushCancelable(bool cancel) {
    #if defined(WIN32)
    //TODO: implementation for Windows
    #else
    int old;
    pthread_setcancelstate(cancel ? PTHREAD_CANCEL_ENABLE : PTHREAD_CANCEL_DISABLE, &old);
    cancelStates.push_back(old);
    #endif
}

/**
 * Restore previous cancellation setting of calling thread by restoring it from
 * stack.
 *
 * @b NOTE: This method is currently not implemented for Windows yet!
 *
 * @see pushCancelable() for details
 */
void Thread::popCancelable() {
    #if defined(WIN32)
    //TODO: implementation for Windows
    #else
    int cancel = cancelStates.back();
    cancelStates.pop_back();
    pthread_setcancelstate(cancel ? PTHREAD_CANCEL_ENABLE : PTHREAD_CANCEL_DISABLE, NULL);
    #endif
}

/**
 * Return this thread's name (intended just for debugging purposes).
 *
 * @b NOTE: This method is currently not implemented for Windows yet!
 */
std::string Thread::name() {
    #if defined(WIN32)
    //TODO: implementation for Windows
    return "not implemented";
    #else
    char buf[16] = {};
    pthread_getname_np(__thread_id, buf, 16);
    std::string s = buf;
    if (s.empty())
        s = "tid=" + ToString(__thread_id);
    return s;
    #endif
}

/**
 * Return calling thread's name (intended just for debugging purposes).
 *
 * @b NOTE: This method is currently not implemented for Windows yet!
 */
std::string Thread::nameOfCaller() {
    #if defined(WIN32)
    //TODO: implementation for Windows
    return "not implemented";
    #else
    char buf[16] = {};
    pthread_getname_np(pthread_self(), buf, 16);
    std::string s = buf;
    if (s.empty())
        s = "tid=" + ToString(pthread_self());
    return s;
    #endif
}

/**
 * Give calling thread a name (intended just for debugging purposes).
 *
 * @b NOTE: POSIX defines a limit of max. 16 characters for @a name.
 *
 * @b NOTE: This method is currently not implemented for Windows yet!
 *
 * @param name - arbitrary, i.e. human readable name for calling thread
 */
void Thread::setNameOfCaller(std::string name) {
    #if defined(WIN32)
    //TODO: implementation for Windows
    #elif __APPLE__
    pthread_setname_np(name.c_str());
    #else // Linux, NetBSD, FreeBSD, ...
    pthread_setname_np(pthread_self(), name.c_str());
    #endif
}

} // namespace LinuxSampler
