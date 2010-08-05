/*
 * sis33acq.c
 * Acquisition helpers for sis33 devices
 *
 * Copyright (c) 2009-2010 Emilio G. Cota <cota@braap.org>
 * Released under the GPL v2. (and only v2, not any later version)
 */
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sis33acq.h"

static int get_endian(void)
{
	int i = 1;
	char *p = (char *)&i;

	if (p[0] == 1)
		return LITTLE_ENDIAN;
	else
		return BIG_ENDIAN;
}

static inline uint32_t be_to_cpu(uint32_t hwval)
{
	if (get_endian() == LITTLE_ENDIAN)
		return myswap32(hwval);
	else
		return hwval;
}

static void acq_swap(void *buf, size_t size)
{
	uint32_t *data = buf;
	int i;

	for (i = 0; i < size / sizeof(uint32_t); i++)
		data[i] = be_to_cpu(data[i]);
}

static void sis33acq_list_swap(struct sis33_acq_list *list, int elems)
{
	struct sis33_acq *acq;
	int i;

	for (i = 0; i < elems; i++) {
		acq = &list->acqs[i];
		if (acq->be && get_endian() == LITTLE_ENDIAN) {
			acq_swap(acq->data, acq->size);
			acq->be = 0;
		}
	}
}

static int sis33acq_reorder(struct sis33_acq *acq)
{
	uint8_t *buf;
	size_t len1;

	buf = malloc(acq->size);
	if (buf == NULL)
		return -1;
	len1 = acq->size - acq->first_samp * sizeof(uint16_t);
	memcpy(buf, &acq->data[acq->first_samp], len1);
	memcpy(buf + len1, acq->data, acq->size - len1);
	memcpy(acq->data, buf, acq->size);
	acq->first_samp = 0;
	free(buf);
	return 0;
}

static int sis33acq_list_reorder(struct sis33_acq_list *list, int elems)
{
	struct sis33_acq *acq;
	int i;

	for (i = 0; i < elems; i++) {
		acq = &list->acqs[i];
		if (acq->first_samp && sis33acq_reorder(acq))
				return -1;
	}
	return 0;
}

int sis33acq_list_normalize(struct sis33_acq_list *list, int elems)
{
	sis33acq_list_swap(list, elems);
	return sis33acq_list_reorder(list, elems);
}

struct sis33_acq *sis33acq_zalloc(unsigned int nr_events, unsigned int ev_length)
{
	struct sis33_acq *acqs;
	struct sis33_acq *acq;
	int i;

	acqs = calloc(nr_events, sizeof(struct sis33_acq));
	if (acqs == NULL)
		return NULL;
	for (i = 0; i < nr_events; i++) {
		acq = &acqs[i];
		acq->data = calloc(ev_length, sizeof(uint16_t));
		if (acq->data == NULL)
			goto error;
		acq->size = ev_length * sizeof(uint16_t);
	}
	return acqs;
 error:
	for (i--; i >= 0; i--) {
		acq = &acqs[i];
		free(acq->data);
	}
	free(acqs);
	return NULL;
}

void sis33acq_free(struct sis33_acq *acqs, unsigned int n_acqs)
{
	struct sis33_acq *acq;
	int i;

	if (acqs == NULL)
		return;
	for (i = 0; i < n_acqs; i++) {
		acq = &acqs[i];
		if (acq->data)
			free(acq->data);
	}
	free(acqs);
}
