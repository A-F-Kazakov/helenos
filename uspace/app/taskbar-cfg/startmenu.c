/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup taskbar-cfg
 * @{
 */
/** @file Start menu configuration tab
 */

#include <gfx/coord.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <tbarcfg/tbarcfg.h>
#include <ui/control.h>
#include <ui/label.h>
#include <ui/list.h>
#include <ui/pbutton.h>
#include <ui/resource.h>
#include <ui/tab.h>
#include <ui/window.h>
#include "startmenu.h"
#include "taskbar-cfg.h"

static void startmenu_entry_selected(ui_list_entry_t *, void *);
static void startmenu_new_entry_clicked(ui_pbutton_t *, void *);
static void startmenu_delete_entry_clicked(ui_pbutton_t *, void *);

/** Entry list callbacks */
ui_list_cb_t startmenu_entry_list_cb = {
	.selected = startmenu_entry_selected
};

/** New entry button callbacks */
ui_pbutton_cb_t startmenu_new_entry_button_cb = {
	.clicked = startmenu_new_entry_clicked
};

/** Remove seat button callbacks */
ui_pbutton_cb_t startmenu_delete_entry_button_cb = {
	.clicked = startmenu_delete_entry_clicked
};

/** Create start menu configuration tab
 *
 * @param tbcfg Taskbar configuration dialog
 * @param rsmenu Place to store pointer to new start menu configuration tab
 * @return EOK on success or an error code
 */
errno_t startmenu_create(taskbar_cfg_t *tbcfg, startmenu_t **rsmenu)
{
	ui_resource_t *ui_res;
	startmenu_t *smenu;
	gfx_rect_t rect;
	errno_t rc;

	ui_res = ui_window_get_res(tbcfg->window);

	smenu = calloc(1, sizeof(startmenu_t));
	if (smenu == NULL) {
		printf("Out of memory.\n");
		return ENOMEM;
	}

	smenu->tbarcfg = tbcfg;

	/* 'Start Menu' tab */

	rc = ui_tab_create(tbcfg->tabset, "Start Menu", &smenu->tab);
	if (rc != EOK)
		goto error;

	rc = ui_fixed_create(&smenu->fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		goto error;
	}

	/* 'Start menu entries:' label */

	rc = ui_label_create(ui_res, "Start menu entries:",
	    &smenu->entries_label);
	if (rc != EOK) {
		printf("Error creating label.\n");
		goto error;
	}

	if (ui_resource_is_textmode(ui_res)) {
		rect.p0.x = 4;
		rect.p0.y = 4;
		rect.p1.x = 36;
		rect.p1.y = 5;
	} else {
		rect.p0.x = 20;
		rect.p0.y = 60;
		rect.p1.x = 360;
		rect.p1.y = 80;
	}

	ui_label_set_rect(smenu->entries_label, &rect);

	rc = ui_fixed_add(smenu->fixed, ui_label_ctl(smenu->entries_label));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	/* List of entries */

	rc = ui_list_create(tbcfg->window, false, &smenu->entries_list);
	if (rc != EOK) {
		printf("Error creating list.\n");
		goto error;
	}

	if (ui_resource_is_textmode(ui_res)) {
		rect.p0.x = 4;
		rect.p0.y = 5;
		rect.p1.x = 56;
		rect.p1.y = 10;
	} else {
		rect.p0.x = 20;
		rect.p0.y = 80;
		rect.p1.x = 360;
		rect.p1.y = 180;
	}

	ui_list_set_rect(smenu->entries_list, &rect);

	rc = ui_fixed_add(smenu->fixed, ui_list_ctl(smenu->entries_list));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	ui_list_set_cb(smenu->entries_list, &startmenu_entry_list_cb,
	    (void *)smenu);

	/* 'New Entry' button */

	rc = ui_pbutton_create(ui_res, "New...", &smenu->new_entry);
	if (rc != EOK) {
		printf("Error creating button.\n");
		goto error;
	}

	if (ui_resource_is_textmode(ui_res)) {
		rect.p0.x = 58;
		rect.p0.y = 5;
		rect.p1.x = 68;
		rect.p1.y = 6;
	} else {
		rect.p0.x = 370;
		rect.p0.y = 80;
		rect.p1.x = 450;
		rect.p1.y = 105;
	}

	ui_pbutton_set_rect(smenu->new_entry, &rect);

	rc = ui_fixed_add(smenu->fixed, ui_pbutton_ctl(smenu->new_entry));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	ui_pbutton_set_cb(smenu->new_entry, &startmenu_new_entry_button_cb,
	    (void *)smenu);

	/* 'Delete Entry' button */

	rc = ui_pbutton_create(ui_res, "Delete", &smenu->delete_entry);
	if (rc != EOK) {
		printf("Error creating button.\n");
		goto error;
	}

	if (ui_resource_is_textmode(ui_res)) {
		rect.p0.x = 58;
		rect.p0.y = 7;
		rect.p1.x = 68;
		rect.p1.y = 8;
	} else {
		rect.p0.x = 370;
		rect.p0.y = 110;
		rect.p1.x = 450;
		rect.p1.y = 135;
	}

	ui_pbutton_set_rect(smenu->delete_entry, &rect);

	rc = ui_fixed_add(smenu->fixed, ui_pbutton_ctl(smenu->delete_entry));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	ui_pbutton_set_cb(smenu->delete_entry,
	    &startmenu_delete_entry_button_cb, (void *)smenu);

	ui_tab_add(smenu->tab, ui_fixed_ctl(smenu->fixed));

	*rsmenu = smenu;
	return EOK;
error:
	if (smenu->delete_entry != NULL)
		ui_pbutton_destroy(smenu->delete_entry);
	if (smenu->new_entry != NULL)
		ui_pbutton_destroy(smenu->new_entry);
	if (smenu->entries_label != NULL)
		ui_label_destroy(smenu->entries_label);
	if (smenu->entries_list != NULL)
		ui_list_destroy(smenu->entries_list);
	if (smenu->fixed != NULL)
		ui_fixed_destroy(smenu->fixed);
	free(smenu);
	return rc;
}

/** Populate start menu tab with start menu configuration data
 *
 * @param smenu Start menu configuration tab
 * @param tbarcfg Taskbar configuration
 * @return EOK on success or an error code
 */
errno_t startmenu_populate(startmenu_t *smenu, tbarcfg_t *tbarcfg)
{
	smenu_entry_t *entry;
	startmenu_entry_t *smentry;
	const char *caption;
	const char *cmd;
	errno_t rc;

	entry = tbarcfg_smenu_first(tbarcfg);
	while (entry != NULL) {
		caption = smenu_entry_get_caption(entry);
		cmd = smenu_entry_get_cmd(entry);

		rc = startmenu_insert(smenu, caption, cmd, &smentry);
		if (rc != EOK)
			return rc;

		entry = tbarcfg_smenu_next(entry);
	}

	return EOK;
}

/** Destroy start menu configuration tab.
 *
 * @param smenu Start menu configuration tab
 */
void startmenu_destroy(startmenu_t *smenu)
{
	ui_list_entry_t *lentry;
	startmenu_entry_t *entry;

	lentry = ui_list_first(smenu->entries_list);
	while (lentry != NULL) {
		entry = (startmenu_entry_t *)ui_list_entry_get_arg(lentry);
		free(entry->caption);
		free(entry->cmd);
		free(entry);
		ui_list_entry_delete(lentry);
		lentry = ui_list_first(smenu->entries_list);
	}

	/* This will automatically destroy all controls in the tab */
	ui_tab_destroy(smenu->tab);
	free(smenu);
}

/** Insert new entry into entries list.
 *
 * @param smenu Start menu configuration tab
 * @param caption Entry caption
 * @param cmd Command to run
 * @param rentry Place to store pointer to new entry or NULL
 * @return EOK on success or an error code
 */
errno_t startmenu_insert(startmenu_t *smenu, const char *caption,
    const char *cmd, startmenu_entry_t **rentry)
{
	startmenu_entry_t *entry;
	ui_list_entry_attr_t attr;
	errno_t rc;

	entry = calloc(1, sizeof(startmenu_entry_t));
	if (entry == NULL)
		return ENOMEM;

	entry->startmenu = smenu;
	entry->caption = str_dup(caption);
	if (entry->caption == NULL) {
		free(entry);
		return ENOMEM;
	}

	entry->cmd = str_dup(cmd);
	if (entry->caption == NULL) {
		free(entry->caption);
		free(entry);
		return ENOMEM;
	}

	ui_list_entry_attr_init(&attr);
	attr.caption = caption;
	attr.arg = (void *)entry;
	rc = ui_list_entry_append(smenu->entries_list, &attr, &entry->lentry);
	if (rc != EOK) {
		free(entry->caption);
		free(entry->cmd);
		free(entry);
		return rc;
	}

	if (rentry != NULL)
		*rentry = entry;
	return EOK;
}

/** Entry in entry list is selected.
 *
 * @param lentry UI list entry
 * @param arg Argument (dcfg_seats_entry_t *)
 */
static void startmenu_entry_selected(ui_list_entry_t *lentry, void *arg)
{
	(void)lentry;
	(void)arg;
}

/** New Entry' button clicked.
 *
 * @param pbutton Push button
 * @param arg Argument (dcfg_seats_entry_t *)
 */
static void startmenu_new_entry_clicked(ui_pbutton_t *pbutton, void *arg)
{
	(void)pbutton;
	(void)arg;
}

/** "Delete Entry' button clicked.
 *
 * @param pbutton Push button
 * @param arg Argument (dcfg_seats_entry_t *)
 */
static void startmenu_delete_entry_clicked(ui_pbutton_t *pbutton, void *arg)
{
	(void)pbutton;
	(void)arg;
}

/** @}
 */
