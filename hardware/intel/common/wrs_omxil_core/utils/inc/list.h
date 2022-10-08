/*
 * list.h, list
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

#ifndef __LIST_H
#define __LIST_H

#ifdef __cplusplus
extern "C" {
#endif

struct list {
    struct list *next;
    struct list *prev;

    void *data;
};

void __list_init(struct list *);

struct list *__list_alloc(void);
struct list *list_alloc(void *);

void __list_free(struct list *);
void list_free_all(struct list *);

struct list *__list_last(struct list *);
struct list *__list_first(struct list *);
struct list *__list_entry(struct list *, int);
int list_length(struct list *);

struct list *__list_add_before(struct list *, struct list *);
struct list *__list_add_after(struct list *, struct list *);
struct list *__list_add_head(struct list *, struct list *);
struct list *__list_add_tail(struct list *, struct list *);
struct list *list_add_head(struct list *, void *);
struct list *list_add_tail(struct list *, void *);

struct list *__list_remove(struct list *, struct list *);
struct list *__list_delete(struct list *, struct list *);
struct list *list_delete(struct list *, void *);
struct list *list_delete_all(struct list *, void *);

struct list *list_find(struct list *, void *);
struct list *list_find_reverse(struct list *, void *);

#define __list_next(entry) ((entry) ? (entry->next) : NULL)
#define __list_prev(entry) ((entry) ? (entry->prev) : NULL)

#define list_foreach(list, ptr)         \
    for (ptr = list;                    \
         ptr != NULL;                   \
         ptr = __list_next(ptr))

#define list_foreach_safe(list, ptr, nxt)                       \
    for (ptr = list, nxt = __list_next(ptr);                    \
         ptr != NULL;						\
         ptr = nxt, nxt = __list_next(ptr))

#define list_foreach_reverse(list, ptr)         \
    for (ptr = __list_last(list);               \
         ptr != NULL;                           \
         ptr = __list_prev(ptr))

#define list_foreach_reverse_safe(list, ptr, prv)               \
    for (ptr = __list_last(list), prv = __list_prev(ptr);      \
         ptr != NULL;						\
         ptr = prv, prv = __list_prev(ptr))

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __LIST_H */
