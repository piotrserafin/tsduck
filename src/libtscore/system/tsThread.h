//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Declare the ts::Thread class.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsThreadAttributes.h"
#include "tsException.h"

namespace ts {
    //!
    //! Base class for threads.
    //! @ingroup libtscore thread
    //!
    //! This is a base class for threads.
    //! A thread object is typically implemented as a subclass of ts::Thread.
    //! The code to be executed in the thread shall be implemented in the
    //! method main().
    //!
    //! This class implements operating system threads.
    //! Its implementation is operating system dependent.
    //!
    //! The C++11 standard class std::thread is not used because it does not provide
    //! any way to set specific thread properties such as stack size. std::thread
    //! provides a method named native_handle() which may be used to call non-portable
    //! features. However, native_handle() becomes accessible only after the thread is
    //! started, too late to set properties such as stack size. This makes std::thread
    //! unusable outside basic simple tasks without specific requirements.
    //!
    class TSCOREDLL Thread
    {
        TS_NOCOPY(Thread);
    public:
        //!
        //! Default constructor (all attributes have their default values).
        //!
        //! @see ThreadAttributes
        //!
        Thread();

        //!
        //! Constructor from specified attributes.
        //!
        //! For convenience, all setters of the ThreadAttributes class
        //! return a reference to the ThreadAttributes object. Thus, it is
        //! possible to build a Thread with selected attributes in one shot,
        //! using the following method:
        //!
        //! @code
        //! class MyThread: public Thread {...};
        //!
        //! MyThread thread (ThreadAttributes().setStackSize(XX).setPriority(YY));
        //! @endcode
        //!
        //! @param [in] attributes The set of attributes.
        //!
        Thread(const ThreadAttributes& attributes);

        //!
        //! Destructor.
        //!
        //! All subclasses should wait for termination before exiting their destructor.
        //! If this is not done, the thread could continue its parallel execution while
        //! the member fields are destructed, which is invalid. As a fool-proof check,
        //! the parent class Thread checks that the thread is actually terminated in
        //! its own destructor and report a fatal error message on standard error if
        //! this is not the case.
        //!
        //! @see waitForTermination()
        //!
        virtual ~Thread();

        //!
        //! Set new attributes to the thread.
        //!
        //! New attributes can be set as long as the thread is not
        //! started, i.e. as long as start() is not invoked.
        //!
        //! @param [in] attributes New attributes to set.
        //! @return True on success, false on error (if the thread
        //! is already started).
        //!
        bool setAttributes(const ThreadAttributes& attributes);

        //!
        //! Get a copy of the attributes of the thread.
        //!
        //! @param [out] attributes Attributes of the thread.
        //!
        void getAttributes(ThreadAttributes& attributes);

        //!
        //! Get an implementation-specific name of the object class.
        //! @return An implementation-specific name of the object class.
        //! The result may be not portable. The returned value may be empty before start().
        //!
        UString getTypeName() const;

        //!
        //! Start the thread.
        //!
        //! The operating system thread is created and started.
        //! The code which is executed in the context of this thread
        //! is in the method main().
        //!
        //! @return True on success, false on error (operating system error or
        //! the thread is already started).
        //!
        bool start();

        //!
        //! Wait for thread termination.
        //!
        //! The thread which invokes this method is blocked until the execution
        //! of this thread object completes.
        //!
        //! Only one waiter thread is allowed. If several threads concurrently
        //! invoke waitForTermination() on the same Thread object, only the
        //! first one will wait. The method waitForTermination() returns an
        //! error to all other threads.
        //!
        //! This method is automatically invoked in the destructor.
        //! Thus, when a Thread object is declared in a control block
        //! and the thread has been started, the end of the control block
        //! hangs as long as the thread is not terminated.
        //! If the thread has not been started, however, the destructor
        //! does not wait (otherwise it would hang for ever).
        //!
        //! @b Important: When a subclass of Thread has non-static members,
        //! its destructor shall invoke waitForTermination(). Thus, it prevents
        //! its members from being destructed until the thread terminates.
        //! If the destructor of the subclass does not invoke waitForTermination() and the
        //! Thread object goes out of scope before the termination of the
        //! thread, the subclass part of the object is destroyed. Any
        //! attempt to access non-static members from the main() method
        //! in the context of the thread will give unexpected results.
        //! Most of the time, this will result in an error similar to
        //! <i>"pure virtual method called"</i>. To avoid this:
        //!
        //! @code
        //! class MyThread: public ts::Thread
        //! {
        //! public:
        //!     virtual ~MyThread()
        //!     {
        //!         waitForTermination();
        //!     }
        //! ...
        //! };
        //! @endcode
        //!
        //! Do not use this method if the thread was created with the
        //! <i>delete when terminated</i> flag
        //! (@link ts::ThreadAttributes::setDeleteWhenTerminated @endlink).
        //!
        //! @return True on success, false on error. Errors include operating
        //! system errors, the thread is not yet started, the caller thread
        //! is this thread (waiting for ourself would result in a dead-lock).
        //!
        bool waitForTermination();

        //!
        //! Check if the caller is running in the context of this thread.
        //!
        //! @return True if the caller of isCurrentThread() is running in
        //! the context of this thread.
        //!
        bool isCurrentThread() const;

        //!
        //! Yield execution of the current thread.
        //! Execution is passed to another thread, if any is waiting for execution.
        //! This should not change the behaviour of correctly-written applications.
        //!
        static void Yield();

    protected:
        //!
        //! This hook is invoked in the context of the thread.
        //!
        //! Concrete thread classes shall implement this pure virtual
        //! method. This method is invoked in the context of the created
        //! thread when it is started.
        //!
        virtual void main() = 0;

        //!
        //! Set the type name.
        //! @param [in] name The type name to set. If empty, the subclass type name is used.
        //!
        void setTypeName(const UString& name = UString());

    private:
        mutable std::recursive_mutex _mutex {};
        ThreadAttributes _attributes {};
        UString          _typename {};
        volatile bool    _started = false;
        volatile bool    _waiting = false;

        // Internal version of isCurrentThread(), bypass checks
        bool isCurrentThreadUnchecked() const;

        // Wrapper around main() plus system-specific base code.
        void mainWrapper();

        #if defined(TS_WINDOWS)
            ::HANDLE _handle = INVALID_HANDLE_VALUE;
            ::DWORD _thread_id = 0;
            // Actual starting point of thread. Parameter is "this".
            static ::DWORD WINAPI ThreadProc(::LPVOID parameter);
        #else
            #if defined(GPROF)
                // When using gprof, we need to pass the ITIMER_PROF value to the thread.
                ::itimerval _itimer {};
                bool _itimer_valid = false;
            #endif
            TS_PUSH_WARNING()
            TS_GCC_NOWARNING(zero-as-null-pointer-constant)
            TS_LLVM_NOWARNING(zero-as-null-pointer-constant)
            pthread_t _pthread = 0;
            TS_POP_WARNING()
            // Actual starting point of thread. Parameter is "this".
            static void* ThreadProc(void* parameter);
        #endif
    };
}
