/*
 * Copyright (c) 2011 Jiri Svoboda
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

/** @addtogroup kbd_ctl
 * @ingroup input
 * @{
 */
/**
 * @file
 * @brief Keyboard device connector controller driver.
 */

#include <async.h>
#include <bool.h>
#include <errno.h>
#include <fcntl.h>
#include <gsp.h>
#include <io/console.h>
#include <io/keycode.h>
#include <ipc/kbdev.h>
#include <kbd.h>
#include <kbd_ctl.h>
#include <kbd_port.h>
#include <stdlib.h>
#include <vfs/vfs_sess.h>


static int kbdev_ctl_init(kbd_dev_t *);
static void kbdev_ctl_set_ind(kbd_dev_t *, unsigned);

static void kbdev_callback_conn(ipc_callid_t, ipc_call_t *);

kbd_ctl_ops_t kbdev_ctl = {
	.parse_scancode = NULL,
	.init = kbdev_ctl_init,
	.set_ind = kbdev_ctl_set_ind
};

/** Kbdev softstate */
typedef struct {
	/** Link to generic keyboard device */
	kbd_dev_t *kbd_dev;

	/** Session with kbdev device */
	async_sess_t *sess;

	/** File descriptor of open kbdev device */
	int fd;
} kbdev_t;

/** XXX Need to pass data from async_connect_to_me() to connection handler */
static kbdev_t *unprotected_kbdev;

static kbdev_t *kbdev_new(kbd_dev_t *kdev)
{
	kbdev_t *kbdev;

	kbdev = calloc(1, sizeof(kbdev_t));
	if (kbdev == NULL)
		return NULL;

	kbdev->kbd_dev = kdev;
	kbdev->fd = -1;

	return kbdev;
}

static void kbdev_destroy(kbdev_t *kbdev)
{
	if (kbdev->sess != NULL)
		async_hangup(kbdev->sess);
	if (kbdev->fd >= 0)
		close(kbdev->fd);
	free(kbdev);
}

static int kbdev_ctl_init(kbd_dev_t *kdev)
{
	const char *pathname;
	async_sess_t *sess;
	async_exch_t *exch;
	kbdev_t *kbdev;
	int fd;
	int rc;

	pathname = kdev->dev_path;

	fd = open(pathname, O_RDWR);
	if (fd < 0) {
		return -1;
	}

	sess = fd_session(EXCHANGE_SERIALIZE, fd);
	if (sess == NULL) {
		printf(NAME ": Failed starting session with '%s'\n", pathname);
		close(fd);
		return -1;
	}

	kbdev = kbdev_new(kdev);
	if (kbdev == NULL) {
		printf(NAME ": Failed allocating device structure for '%s'.\n",
		    pathname);
		return -1;
	}

	kbdev->fd = fd;
	kbdev->sess = sess;

	exch = async_exchange_begin(sess);
	if (exch == NULL) {
		printf(NAME ": Failed starting exchange with '%s'.\n", pathname);
		kbdev_destroy(kbdev);
		return -1;
	}

	/*
	 * XXX We need to pass kbdev to the connection handler. Since the
	 * framework does not support this, use a global variable.
	 * This needs to be fixed ASAP.
	 */
	unprotected_kbdev = kbdev;

	rc = async_connect_to_me(exch, 0, 0, 0, kbdev_callback_conn);
	if (rc != EOK) {
		printf(NAME ": Failed creating callback connection from '%s'.\n",
		    pathname);
		async_exchange_end(exch);
		kbdev_destroy(kbdev);
		return -1;
	}

	async_exchange_end(exch);

	kdev->ctl_private = (void *) kbdev;
	return 0;
}

static void kbdev_ctl_set_ind(kbd_dev_t *kdev, unsigned mods)
{
	async_sess_t *sess;
	async_exch_t *exch;

	sess = ((kbdev_t *) kdev->ctl_private)->sess;

	exch = async_exchange_begin(sess);
	if (!exch)
		return;

	async_msg_1(exch, KBDEV_SET_IND, mods);
	async_exchange_end(exch);
}

static void kbdev_callback_conn(ipc_callid_t iid, ipc_call_t *icall)
{
	kbdev_t *kbdev;
	int retval;
	int type, key;

	kbdev = unprotected_kbdev;

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid;

		callid = async_get_call(&call);
		if (!IPC_GET_IMETHOD(call)) {
			/* XXX Handle hangup */
			return;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case KBDEV_EVENT:
			/* Got event from keyboard device */
			retval = 0;
			type = IPC_GET_ARG1(call);
			key = IPC_GET_ARG2(call);
			kbd_push_ev(kbdev->kbd_dev, type, key);
			break;
		default:
			retval = ENOTSUP;
			break;
		}

		async_answer_0(callid, retval);
	}
}

/**
 * @}
 */