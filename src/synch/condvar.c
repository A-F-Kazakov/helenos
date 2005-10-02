/*
 * Copyright (C) 2001-2004 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <synch/condvar.h>
#include <synch/mutex.h>
#include <synch/waitq.h>
#include <synch/synch.h>

/** Initialize condition variable
 *
 * Initialize condition variable.
 *
 * @param cv Condition variable.
 */
void condvar_initialize(condvar_t *cv)
{
	waitq_initialize(&cv->wq);
}

/** Signal the condition has become true
 *
 * Signal the condition has become true
 * to the first waiting thread by waking it up.
 *
 * @param Condition variable.
 */
void condvar_signal(condvar_t *cv)
{
	waitq_wakeup(&cv->wq, WAKEUP_FIRST);
}

/** Signal the condition has become true
 *
 * Signal the condition has become true
 * to all waiting threads by waking them up.
 *
 * @param Condition variable.
 */
void condvar_broadcast(condvar_t *cv)
{
	waitq_wakeup(&cv->wq, WAKEUP_ALL);
}

/** Wait for the condition becoming true
 *
 * Wait for the condition becoming true.
 *
 * @param Condition variable.
 */
int _condvar_wait_timeout(condvar_t *cv, mutex_t *mtx, __u32 usec, int trywait)
{
	int rc;

	mutex_unlock(mtx);
	rc = waitq_sleep_timeout(&cv->wq, usec, trywait);
	mutex_lock(mtx);
	return rc;
}
