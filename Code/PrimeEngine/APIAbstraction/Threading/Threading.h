#ifndef __PYENGINE_2_0_THREADING_H___
#define __PYENGINE_2_0_THREADING_H___

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

#include "PrimeEngine/Utils/ErrorHandling.h"

#if APIABSTRACTION_IOS
#include <pthread/pthread.h>
#endif

// Outer-Engine includes
#if APIABSTRACTION_D3D9 || APIABSTRACTION_D3D11

#ifdef _WIN32
#ifdef _XBOX
#include <xtl.h>
#else
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#include <windows.h>
#endif
#endif
#elif PE_PLAT_IS_PSVITA
#include <kernel.h>
#endif

// Inter-Engine includes

namespace PE {
namespace Threading {
	enum ThreadOwnedContexts
	{
		GameContext = 1 << 0,
		RenderContext = 1 << 1,
	};

	typedef unsigned int ThreadId;
	struct Mutex
	{
		int memCheck;
#if APIABSTRACTION_PS3 || APIABSTRACTION_IOS
		pthread_mutex_t m_osLock;
#elif  PE_PLAT_IS_PSVITA
		SceUID m_osLock;
#else
		CRITICAL_SECTION m_osLock;
#endif
		ThreadId m_threadId;
		Mutex()
		{
			memCheck = 0x12121212;
			PEINFO("memCheck addr: %x", &m_osLock);
#if APIABSTRACTION_PS3 || APIABSTRACTION_IOS
			pthread_mutex_init(&m_osLock, 0);
#elif PE_PLAT_IS_PSVITA
			m_osLock = sceKernelCreateMutex(
				"pe-mutex", SCE_KERNEL_MUTEX_ATTR_TH_PRIO, 0, 0);
#else
			InitializeCriticalSection(&m_osLock);
#endif
		}

		~Mutex()
		{
#if APIABSTRACTION_PS3 || APIABSTRACTION_IOS
			pthread_mutex_destroy(&m_osLock);
#elif PE_PLAT_IS_PSVITA
			sceKernelDeleteMutex(m_osLock);
#else
			DeleteCriticalSection(&m_osLock);
#endif
		}

		bool lock(ThreadId threadId = 0)
		{
			m_threadId = threadId;
#if APIABSTRACTION_PS3 || APIABSTRACTION_IOS
			return pthread_mutex_lock(&m_osLock) == 0;
#elif PE_PLAT_IS_PSVITA
			return sceKernelLockMutex(m_osLock, 1, NULL) == SCE_OK;
#else
			EnterCriticalSection(&m_osLock);
			return true;
#endif
		}

		void unlock()
		{
#if APIABSTRACTION_PS3 || APIABSTRACTION_IOS
			pthread_mutex_unlock(&m_osLock);
#elif PE_PLAT_IS_PSVITA
			sceKernelUnlockMutex(m_osLock, 1);
#else
			LeaveCriticalSection(&m_osLock);
#endif
		}
	};

	struct ConditionVariable
	{
#if APIABSTRACTION_PS3 || APIABSTRACTION_IOS
		pthread_cond_t m_osCV;
#elif PE_PLAT_IS_PSVITA
		SceUID m_osCV;
#else
	#if !APIABSTRACTION_X360
		CONDITION_VARIABLE m_osCV;
	#endif
#endif
		Mutex &m_associatedLock;
		ConditionVariable(Mutex &lock)
			: m_associatedLock(lock)
		{
#if APIABSTRACTION_PS3 || APIABSTRACTION_IOS
			pthread_cond_init(&m_osCV, 0);
#elif PE_PLAT_IS_PSVITA
			m_osCV = sceKernelCreateCond("pe-condition-var", SCE_KERNEL_COND_ATTR_TH_FIFO, lock.m_osLock, NULL);
#else
			#if !APIABSTRACTION_X360
				 InitializeConditionVariable(&m_osCV);
			#endif
#endif
		}

		~ConditionVariable()
		{
#if APIABSTRACTION_PS3 || APIABSTRACTION_IOS
			pthread_cond_destroy(&m_osCV);
#elif PE_PLAT_IS_PSVITA
			sceKernelDeleteCond(m_osCV);
#else
			// I dont think there is a function for this?
			// DeleteConditionVariable(&m_osCV);
#endif
		}

		bool sleep()
		{
			
#if APIABSTRACTION_PS3 || APIABSTRACTION_IOS
			return pthread_cond_wait(&m_osCV, &m_associatedLock.m_osLock) == 0;
#elif PE_PLAT_IS_PSVITA
			return sceKernelWaitCond(m_osCV, NULL) == SCE_OK;
#else
			#if !APIABSTRACTION_X360
				return SleepConditionVariableCS(&m_osCV, &m_associatedLock.m_osLock, INFINITE) == TRUE;
			#else
				// very poor implementation of condition variable with lock only. it will work for now
				// but this will not work if multiple CVs use same lock
				m_associatedLock.unlock();
				//Sleep(0);
				m_associatedLock.lock();
				return true;
			#endif
#endif
		}

		void signal()
		{
#if APIABSTRACTION_PS3 || APIABSTRACTION_IOS
			pthread_cond_signal(&m_osCV);
#elif PE_PLAT_IS_PSVITA
			sceKernelSignalCond(m_osCV);
#else
			#if !APIABSTRACTION_X360
				WakeConditionVariable(&m_osCV);
			#endif
#endif
		}
	};
#if APIABSTRACTION_PS3 || APIABSTRACTION_IOS
	typedef pthread_t PEOsThread;
#elif PE_PLAT_IS_PSVITA
	typedef SceUID PEOsThread;
#else
	typedef HANDLE PEOsThread;
#endif

	 typedef void (*ThreadFunction)(void *params);


	struct PEThread
	{
		int m_start;
		PEOsThread m_osThread;
		ThreadFunction m_function;
		void *m_pParams;
		const static int PE_THREAD_STACK_SIZE = 1024 * 1024;
		// wrapper to call m_function
#if APIABSTRACTION_PS3 || APIABSTRACTION_IOS
		static void *OSThreadFunction(void *params)
        {
#elif PE_PLAT_IS_PSVITA
		struct VitaArg
		{
			void *param;
		} m_vitaArg;
		static SceInt32 OSThreadFunction(SceSize argSize, void *vitaParams)
		{
			VitaArg *pArg = static_cast<VitaArg*>(vitaParams);
			void *params = pArg->param;
#else
		static unsigned int __stdcall OSThreadFunction(void *params)
		{

#endif
			PEThread *pThread = static_cast<PEThread *>(params);
			PEASSERT(pThread->m_start == 0xdeadbeef, "bad ptr");
			(*pThread->m_function)(pThread->m_pParams);
			return 0;
		}

		void run()
		{
			m_start = 0xdeadbeef;
#if APIABSTRACTION_PS3 || APIABSTRACTION_IOS
			pthread_create(&m_osThread, NULL, PEThread::OSThreadFunction, this);
#elif PE_PLAT_IS_PSVITA
			m_osThread = sceKernelCreateThread(
				"PEThread::OSThreadFunction",
				PEThread::OSThreadFunction,
				SCE_KERNEL_DEFAULT_PRIORITY_USER,
				PE_THREAD_STACK_SIZE,
				0,
				SCE_KERNEL_THREAD_CPU_AFFINITY_MASK_DEFAULT,
				NULL
				);

			PEASSERT(m_osThread >= 0, "Error creating a thread\n");
			m_vitaArg.param = this;
			SceInt32 res = sceKernelStartThread(
				m_osThread,
				sizeof(m_vitaArg),
				&m_vitaArg
				);
#else
			unsigned int threadId;
			m_osThread =(HANDLE)_beginthreadex(NULL,
				0,
				PEThread::OSThreadFunction,
				this,
				CREATE_SUSPENDED,
				&threadId);
			ResumeThread(m_osThread);
#endif
		}
	};
}; // Threading
}; // PE

#endif
