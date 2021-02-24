/*******************************************************************************
 * Copyright (c) 1991, 2020 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include "modronopt.h"
#include "ModronAssertions.h"
#include "omrport.h"

#include "MainGCThread.hpp"

#include "CollectorLanguageInterfaceImpl.hpp"
#include "ConcurrentGMPStats.hpp"
#include "ConcurrentPhaseStatsBase.hpp"
#include "ParallelDispatcher.hpp"
#include "EnvironmentBase.hpp"
#include "Collector.hpp"

/**
 * Initialization
 */
MM_MainGCThread::MM_MainGCThread(MM_EnvironmentBase *env)
	: MM_BaseNonVirtual()
	, _collectorControlMutex(NULL)
	, _mainThreadState(STATE_ERROR)
	, _mainGCThread(NULL)
	, _incomingCycleState(NULL)
	, _allocDesc(NULL)
	, _extensions(env->getExtensions())
	, _collector(NULL)
	, _runAsImplicit(false)
	, _acquireVMAccessDuringConcurrent(false)
	, _concurrentResumable(false)
{
	_typeId = __FUNCTION__;
}

uintptr_t
MM_MainGCThread::main_thread_proc2(OMRPortLibrary* portLib, void *info)
{
	MM_MainGCThread *mainGCThread = (MM_MainGCThread*)info;
	/* jump into the main thread procedure and wait for work.  This method will NOT return */
	mainGCThread->mainThreadEntryPoint();
	Assert_MM_unreachable();
	return 0;
}

int J9THREAD_PROC
MM_MainGCThread::main_thread_proc(void *info)
{
	MM_MainGCThread *mainGCThread = (MM_MainGCThread*)info;
	MM_GCExtensionsBase *extensions = mainGCThread->_extensions;
	OMR_VM *omrVM = extensions->getOmrVM();
	OMRPORT_ACCESS_FROM_OMRVM(omrVM);
	uintptr_t rc = 0;
	omrsig_protect(main_thread_proc2, info,
			extensions->dispatcher->getSignalHandler(), omrVM,
		OMRPORT_SIG_FLAG_SIGALLSYNC | OMRPORT_SIG_FLAG_MAY_CONTINUE_EXECUTION,
		&rc);
	return 0;
}


bool
MM_MainGCThread::initialize(MM_Collector *collector, bool runAsImplicit, bool acquireVMAccessDuringConcurrent, bool concurrentResumable)
{
	bool success = true;
	if(omrthread_monitor_init_with_name(&_collectorControlMutex, 0, "MM_MainGCThread::_collectorControlMutex")) {
		success = false;
	}

	_collector = collector;
	_runAsImplicit = runAsImplicit;
	_acquireVMAccessDuringConcurrent = acquireVMAccessDuringConcurrent;
	_concurrentResumable = concurrentResumable;

	return success;
}

void
MM_MainGCThread::tearDown(MM_EnvironmentBase *env)
{
	if (NULL != _collectorControlMutex) {
		omrthread_monitor_destroy(_collectorControlMutex);
		_collectorControlMutex = NULL;
	}
	_collector = NULL;
}


bool
MM_MainGCThread::startup()
{
	/* set the success flag to false and we will set it true if everything succeeds */
	bool success = false;

	if (_extensions->fvtest_disableExplictMainThread) {
		/* GC should be able to act even if main thread is not created (or late) */
		_mainThreadState = STATE_DISABLED;
		success = true;
	} else {
		/* hold the monitor over start-up of this thread so that we eliminate any timing hole where it might notify us of its start-up state before we wait */
		omrthread_monitor_enter(_collectorControlMutex);
		_mainThreadState = STATE_STARTING;
		intptr_t forkResult = createThreadWithCategory(
			NULL,
			OMR_OS_STACK_SIZE,
			J9THREAD_PRIORITY_NORMAL,
			0,
			main_thread_proc,
			this,
			J9THREAD_CATEGORY_SYSTEM_GC_THREAD);
		if (forkResult == 0) {
			/* thread creation success */
			/* wait to find out if they started up, successfully */
			while (STATE_STARTING == _mainThreadState) {
				omrthread_monitor_wait(_collectorControlMutex);
			}
			if (STATE_ERROR != _mainThreadState) {
				/* the main thread managed to start up and is in the waiting state, ready for GC requests */
				success = true;
			}
		} else {
			_mainThreadState = STATE_ERROR;
		}
		omrthread_monitor_exit(_collectorControlMutex);
	}

	return success;
}

void
MM_MainGCThread::shutdown()
{
	Assert_MM_true(NULL != _collectorControlMutex);
	if ((STATE_ERROR != _mainThreadState) && (STATE_DISABLED != _mainThreadState)) {
		/* tell the background thread to shut down and then wait for it to exit */
		omrthread_monitor_enter(_collectorControlMutex);
		while(STATE_TERMINATED != _mainThreadState) {
			_mainThreadState = STATE_TERMINATION_REQUESTED;
			omrthread_monitor_notify(_collectorControlMutex);
			omrthread_monitor_wait(_collectorControlMutex);
		}
		omrthread_monitor_exit(_collectorControlMutex);
		
		/* don't NULL _collector as RAS could still trigger a collection after we've started shutting down */
	}
}


void
MM_MainGCThread::handleSTW(MM_EnvironmentBase *env)
{
	Assert_MM_true(NULL != _incomingCycleState);
	env->_cycleState = _incomingCycleState;

	printf("-|_|-|_|- TD#: %zu, inisde MM_MainGCThread::handleSTW how did we get here????? ##################\n", (uintptr_t)pthread_self());
	fflush(stdout);

	/* this thread effectively inherits exclusive access from the mutator thread -- set its state to indicate this */
	// TODO: Why are we asserting here? This is not working because on first concurrent PGC phase we
	// already call assumeExclusiveVMAccess(1) which already sets Exclusive VM access. This call assumes that
	// we did not have exclusive VM access before, which is not try anymore. So how can we assert both cases?
	env->assumeExclusiveVMAccess(1);

	_collector->mainThreadGarbageCollect(env, _allocDesc);

	uintptr_t exclusiveCount = env->relinquishExclusiveVMAccess();
	Assert_MM_true(1 == exclusiveCount);

	env->_cycleState = NULL;
	_incomingCycleState = NULL;
	_mainThreadState = STATE_WAITING;
	omrthread_monitor_notify(_collectorControlMutex);
}

bool
MM_MainGCThread::handleConcurrent(MM_EnvironmentBase *env)
{
	bool workDone = false;

	_mainThreadState = STATE_RUNNING_CONCURRENT;

	do {
		if (_acquireVMAccessDuringConcurrent) {
			omrthread_monitor_exit(_collectorControlMutex);
			env->acquireVMAccess();
		}
		if (_collector->isConcurrentWorkAvailable(env)) {
			MM_ConcurrentPhaseStatsBase *stats = _collector->getConcurrentPhaseStats();
			stats->clear();

			// TODO: This prints/reports concurrent start to stdout
			_collector->preConcurrentInitializeStatsAndReport(env, stats);
			if (!_acquireVMAccessDuringConcurrent) {
				omrthread_monitor_exit(_collectorControlMutex);
			}
			printf("TD#: %zu, inside handleConcurrent, about to call _collector->mainThreadConcurrentCollect -------------------\n", (uintptr_t)pthread_self());
			fflush(stdout);
			uintptr_t bytesConcurrentlyScanned = _collector->mainThreadConcurrentCollect(env);
			printf("TD#: %zu, inside handleConcurrent, RETURNED from _collector->mainThreadConcurrentCollect -------------------\n", (uintptr_t)pthread_self());
			fflush(stdout);

			if (!_acquireVMAccessDuringConcurrent) {
				omrthread_monitor_enter(_collectorControlMutex);
			}

			// TODO: This prints/reports concurrent end to stdout
			// 	 Should we move both report calls to the respective collectors? E.g. Scavenger and IncrementalGenerationalGC
			_collector->postConcurrentUpdateStatsAndReport(env, stats, bytesConcurrentlyScanned);
			workDone = true;
		}

		if (_acquireVMAccessDuringConcurrent) {
			env->releaseVMAccess();
			omrthread_monitor_enter(_collectorControlMutex);
		}
	} while (_concurrentResumable && _collector->isConcurrentWorkAvailable(env));
	// TODO: If we came from 2nd PGC STW phase, then we must somehow set STATE_GC_REQUESTED to force 3rd and last PGC STW phase
	// 	 The goal is to eventually let mutator threads run during 2nd PGC phase but for the first iteration of changes
	// 	 the 2ns PGC phase will be STW, therefore there's no need to let mutators run.
	if (STATE_RUNNING_CONCURRENT == _mainThreadState) {
		_mainThreadState = STATE_WAITING;
	}

	return workDone;
}

void
MM_MainGCThread::mainThreadEntryPoint()
{
	OMR_VMThread *omrVMThread = NULL;
	Assert_MM_true(NULL != _collectorControlMutex);
	Assert_MM_true(NULL == _mainGCThread);

	/* Attach the thread as a system daemon thread */	
	/* You need a VM thread so that the stack walker can work */
	omrVMThread = MM_EnvironmentBase::attachVMThread(_extensions->getOmrVM(), "Dedicated GC Main", MM_EnvironmentBase::ATTACH_GC_MAIN_THREAD);
	if (NULL == omrVMThread) {
		/* we failed to attach so notify the creating thread that we should fail to start up */
		omrthread_monitor_enter(_collectorControlMutex);
		_mainThreadState = STATE_ERROR;
		omrthread_monitor_notify(_collectorControlMutex);
		omrthread_exit(_collectorControlMutex);
	} else {
		/* thread attached successfully */
		MM_EnvironmentBase *env = MM_EnvironmentBase::getEnvironment(omrVMThread);

		/* attachVMThread could have allocated and execute a barrier (until point, this thread acted as a mutator thread.
		 * Flush GC chaches (like barrier buffers) before turning into the main thread */
		/* TODO: call plain env->initializeGCThread() once downstream projects are ready (subclass Env::init calls base Env::init)  */
		env->MM_EnvironmentBase::initializeGCThread();

		env->setThreadType(GC_MAIN_THREAD);

		/* Begin running the thread */
		omrthread_monitor_enter(_collectorControlMutex);
		
		_collector->preMainGCThreadInitialize(env);
		
		_mainThreadState = STATE_WAITING;
		_mainGCThread = omrthread_self();
		omrthread_monitor_notify(_collectorControlMutex);
		do {
			if (STATE_GC_REQUESTED == _mainThreadState) {
				if (_runAsImplicit) { 
					printf("TD#: %zu, inisde mainThreadEntryPoint(), about to call handleConcurrent() 0000000000000\n", (uintptr_t)pthread_self());
					fflush(stdout);
					handleConcurrent(env);
				} else {
					// TODO: All 3 phases of concurrent PGC will be dealt here <<<<<<<<<<<<<<<<<<<<
					printf("TD#: %zu, inisde mainThreadEntryPoint(), about to call handleSTW() 1111111111111\n", (uintptr_t)pthread_self());
					fflush(stdout);
					handleSTW(env);
				}
			}

			if (STATE_WAITING == _mainThreadState) {
				printf("TD#: %zu, inisde mainThreadEntryPoint(), about to call handleConcurrent() 22222222222222222222222 \n", (uintptr_t)pthread_self());
				fflush(stdout);
				if (_runAsImplicit || !handleConcurrent(env)) {
					printf("TD#: %zu, inisde mainThreadEntryPoint, going to sleep and _runAsImplicit: %d\n", (uintptr_t)pthread_self(), (int)_runAsImplicit);
					fflush(stdout);
					omrthread_monitor_wait(_collectorControlMutex); // <><><><><><><><><><><><>
					printf("TD#: %zu, inisde mainThreadEntryPoint, thread WOKE UP FROM SLEEP!!!!!\n", (uintptr_t)pthread_self());
					fflush(stdout);
				}
			}
		} while (STATE_TERMINATION_REQUESTED != _mainThreadState);
		/* notify the other side that we are active so that they can continue running */
		_mainThreadState = STATE_TERMINATED;
		_mainGCThread = NULL;
		omrthread_monitor_notify(_collectorControlMutex);
		MM_EnvironmentBase::detachVMThread(_extensions->getOmrVM(), omrVMThread, MM_EnvironmentBase::ATTACH_GC_MAIN_THREAD);
		omrthread_exit(_collectorControlMutex);
	}
}

bool
MM_MainGCThread::garbageCollect(MM_EnvironmentBase *env, MM_AllocateDescription *allocDescription)
{
	Assert_MM_mustHaveExclusiveVMAccess(env->getOmrVMThread());
	bool didAttemptCollect = false;
	printf("TD#: %zu, inisde MM_MainGCThread::garbageCollect() This is most likely the entry point of a mutator thread!!!!!\n", (uintptr_t)pthread_self());
	fflush(stdout);
	
	if (NULL != _collector) {
		/* the collector has started up so try to run */
		/* once the main thread has stored itself in the _mainGCThread, it should never need to collect - this would hang */
		Assert_MM_true(omrthread_self() != _mainGCThread);
		if (_runAsImplicit || (NULL == _mainGCThread)) {
			/* We might not have _mainGCThread in the startup phase or late in the shutdown phase.
			 * For example, there may be a native out-of-memory during startup or RAS may 
			 * trigger a GC after we've shutdown the main thread.
			 */
			Assert_MM_true(0 == env->getWorkerID());
			_collector->preMainGCThreadInitialize(env);
			printf("TD#: %zu, inisde MM_MainGCThread::garbageCollect() and about to call _collector->mainThreadGarbageCollect()......\n", (uintptr_t)pthread_self());
			fflush(stdout);
			_collector->mainThreadGarbageCollect(env, allocDescription);

			if (_runAsImplicit && _collector->isConcurrentWorkAvailable(env)) {
				omrthread_monitor_enter(_collectorControlMutex);

				if (STATE_WAITING == _mainThreadState) {
					_mainThreadState = STATE_GC_REQUESTED;
					printf("TD#: %zu, inisde MM_MainGCThread::garbageCollect, waking up other threads on _collectorControlMutex.........\n", (uintptr_t)pthread_self());
					fflush(stdout);
					omrthread_monitor_notify(_collectorControlMutex);
				}

				omrthread_monitor_exit(_collectorControlMutex);
			}
		} else {
			/* this is the general case, when the main thread is running internally */
			omrthread_monitor_enter(_collectorControlMutex);
			/* The variable assignments below are safe because we hold Xaccess.  Otherwise, it is possible (based on the wait/notify mechanism here)
			 * that another thread could come in under this mutex and stomp on the "parameters" while another thread is waiting.
			 */
			_allocDesc = allocDescription;
			_incomingCycleState = env->_cycleState;
			MainGCThreadState previousState = _mainThreadState;
			_mainThreadState = STATE_GC_REQUESTED;
			if (STATE_WAITING == previousState) {
				printf("TD#: %zu, inisde garbageCollect() about to notify on _collectorControlMutex!!\n", (uintptr_t)pthread_self());
				fflush(stdout);
				omrthread_monitor_notify(_collectorControlMutex);
			} else if (STATE_RUNNING_CONCURRENT == previousState) {
				printf("TD#: %zu, inisde garbageCollect() forcing concurrent finish\n", (uintptr_t)pthread_self());
				fflush(stdout);
				_collector->forceConcurrentFinish();
			} else {
				Assert_MM_unreachable();
			}
			
			/* The main thread will claim exclusive VM access. Artificially give it up in this thread so that tools like -Xcheck:vm continue to work. */
			uintptr_t savedExclusiveCount = env->relinquishExclusiveVMAccess();
			// TODO: This is the entry point of PGC, in which case this thread will wake up the sleeping explicit thread from  <><><><><><><><> line above
			while (STATE_GC_REQUESTED == _mainThreadState) {
				printf("TD#: %zu, inisde garbageCollect(), main thread going to take a nap zZzZzZzZzZzZzZzZzZZzZzZZzZzZZzZ... on _collectorControlMutex\n", (uintptr_t)pthread_self());
				fflush(stdout);
				omrthread_monitor_wait(_collectorControlMutex);
				printf("TD#: %zu, inisde garbageCollect(), main thread just woke up from the nap!!!!!!!! on _collectorControlMutex\n", (uintptr_t)pthread_self());
				fflush(stdout);
			}
			env->assumeExclusiveVMAccess(savedExclusiveCount);

			Assert_MM_true(NULL == _incomingCycleState);
			omrthread_monitor_exit(_collectorControlMutex);
		}
		
		didAttemptCollect = true;
	}  else {
		printf("TD#: %zu, inisde MM_MainGCThread::garbageCollect() and _collector is NULL!!!!!!!!!!!!!!!!!!!!!!!!!!!\n", (uintptr_t)pthread_self());
		fflush(stdout);
	}

	return didAttemptCollect;
}
