/*
 * queue.c, queue
 *
 * Copyright (c) 2009-2010 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>

#include <queue.h>

inline void __queue_init(struct queue *queue)
{
	queue->head = NULL;
	queue->tail = NULL;
	queue->length = 0;
}

struct queue *queue_alloc(void)
{
	struct queue *queue;

	queue = malloc(sizeof(struct queue));
	if (queue)
		__queue_init(queue);

	return queue;
}

inline void __queue_free(struct queue *queue)
{
	free(queue);
}

void queue_free_all(struct queue *queue)
{
	struct list *list = queue->head;

	list_free_all(list);
	__queue_init(queue);
}

void __queue_push_head(struct queue *queue, struct list *entry)
{
	queue->head = __list_add_head(queue->head, entry);
	if (!queue->tail)
		queue->tail = queue->head;

	queue->length++;
}

int queue_push_head(struct queue *queue, void *data)
{
	struct list *entry = list_alloc(data);

	if (!entry)
		return -1;
	else
		queue->head = __list_add_head(queue->head, entry);

	if (!queue->tail)
		queue->tail = queue->head;

	queue->length++;
        return 0;
}

void __queue_push_tail(struct queue *queue, struct list *entry)
{
        queue->tail = list_add_tail(queue->tail, entry);
        if (queue->tail == NULL) {
                return;
        }
	if (queue->tail->next)
		queue->tail = queue->tail->next;
	else
		queue->head = queue->tail;

	queue->length++;
}

int queue_push_tail(struct queue *queue, void *data)
{
	struct list *entry = list_alloc(data);

	if (!entry)
		return -1;
	else
		queue->tail = __list_add_tail(queue->tail, entry);

	if (queue->tail->next)
		queue->tail = queue->tail->next;
	else
		queue->head = queue->tail;

	queue->length++;
        return 0;
}

struct list *__queue_pop_head(struct queue *queue)
{
	struct list *entry = queue->head;

	if (entry) {
		queue->head = __list_remove(queue->head, entry);
		if (!queue->head)
			queue->tail = NULL;

		queue->length--;
	}

	return entry;
}

void *queue_pop_head(struct queue *queue)
{
	struct list *entry;
	void *data = NULL;

	entry = __queue_pop_head(queue);
	if (entry) {
		data = entry->data;
		__list_free(entry);
	}

	return data;
}

struct list *__queue_pop_tail(struct queue *queue)
{
	struct list *entry = queue->tail;
	struct list *prev;

	if (entry) {
		prev = entry->prev;
		queue->head = __list_remove(queue->head, entry);
		queue->tail = prev;
		queue->length--;
	}

	return entry;
}

void *queue_pop_tail(struct queue *queue)
{
	struct list *entry;
	void *data = NULL;

	entry = __queue_pop_tail(queue);
	if (entry) {
		data = entry->data;
		__list_free(entry);
	}

	return data;
}

inline struct list *__queue_peek_head(struct queue *queue)
{
	return queue->head;
}

inline struct list *__queue_peek_tail(struct queue *queue)
{
	return queue->tail;
}

inline void *queue_peek_head(struct queue *queue)
{
	struct list *entry;
	void *data = NULL;

	entry = __queue_peek_head(queue);
	if (entry)
		data = entry->data;

	return data;
}

inline void *queue_peek_tail(struct queue *queue)
{
	struct list *entry;
	void *data = NULL;

	entry = __queue_peek_tail(queue);
	if (entry)
		data = entry->data;

	return data;
}

int queue_length(struct queue *queue)
{
	return queue->length;
}
