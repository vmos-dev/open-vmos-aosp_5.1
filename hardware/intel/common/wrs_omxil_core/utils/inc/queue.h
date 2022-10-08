/*
 * queue.h, queue
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

#ifndef __QUEUE_H
#define __QUEUE_H

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

struct queue {
	struct list *head;
	struct list *tail;
	int  length;
};

void __queue_init(struct queue *queue);
struct queue *queue_alloc(void);
/* FIXME */
inline void __queue_free(struct queue *queue);
/* FIXME */
void queue_free_all(struct queue *queue);

void __queue_push_head(struct queue *queue, struct list *entry);
int queue_push_head(struct queue *queue, void *data);
void __queue_push_tail(struct queue *queue, struct list *entry);
int queue_push_tail(struct queue *queue, void *data);

struct list *__queue_pop_head(struct queue *queue);
void *queue_pop_head(struct queue *queue);
struct list *__queue_pop_tail(struct queue *queue);
void *queue_pop_tail(struct queue *queue);

inline struct list *__queue_peek_head(struct queue *queue);
inline struct list *__queue_peek_tail(struct queue *queue);
inline void *queue_peek_head(struct queue *queue);
inline void *queue_peek_tail(struct queue *queue);

int queue_length(struct queue *queue);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __QUEUE_H */
