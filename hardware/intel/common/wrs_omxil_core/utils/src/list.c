/*
 * list.c, list
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

#include <list.h>

void __list_init(struct list *entry)
{
    if (entry) {
        entry->prev = NULL;
        entry->next = NULL;
        entry->data = NULL;
    }
}

struct list *__list_alloc(void)
{
    struct list *new;

    new = malloc(sizeof(struct list));
    __list_init(new);

    return new;
}

struct list *list_alloc(void *data)
{
    struct list *new;

    new = __list_alloc();
    if (new)
        new->data = data;

    return new;
}

void __list_free(struct list *entry)
{
    free(entry);
}

void list_free_all(struct list *list)
{
    struct list *ptr, *tmp;

    list_foreach_safe(list, ptr, tmp) {
        __list_free(ptr);
    }
}

struct list *__list_last(struct list *list)
{
    if (list)
        while (list->next)
            list = list->next;

    return list;
}

struct list *__list_first(struct list *list)
{
    if (list)
        while (list->prev)
            list = list->prev;

    return list;
}

struct list *__list_entry(struct list *list, int index)
{
    struct list *entry;
    int i = 0;

    list_foreach(list, entry) {
        if (i == index)
            break;
        i++;
    }

    return entry;
}

int list_length(struct list *list)
{
    int length = 0;

    while (list) {
        list = list->next;
        length++;
    }

    return length;
}

struct list *__list_add_before(struct list *entry, struct list *new)
{
    struct list *prev;

    if (entry) {
        prev = entry->prev;
        if (prev)
            prev->next = new;
        new->prev = prev;
        new->next = entry;
        entry->prev = new;
    }

    return new;
}

struct list *__list_add_after(struct list *entry, struct list *new)
{
    struct list *next;

    if (entry) {
        next = entry->next;
        if (next)
            next->prev = new;
        new->next = next;
        new->prev = entry;
        entry->next = new;

        return entry;
    }

    return new;
}

struct list *__list_add_head(struct list *list, struct list *new)
{
    struct list *first;

    if (list) {
        first = __list_first(list);
        __list_add_before(first, new);
    }

    return new;
}

struct list *__list_add_tail(struct list *list, struct list *new)
{
    struct list *last;

    if (list) {
        last = __list_last(list);
        __list_add_after(last, new);

        return list;
    }
    else
        return new;
}

struct list *list_add_head(struct list *list, void *data)
{
    struct list *new;

    new = list_alloc(data);
    if (!new)
        return NULL;

    return __list_add_head(list, new);
}

struct list *list_add_tail(struct list *list, void *data)
{
    struct list *new;

    new = list_alloc(data);
    if (!new)
        return NULL;

    return __list_add_tail(list, new);
}

struct list *__list_remove(struct list *list, struct list *entry)
{
    struct list *prev, *next;

    if (entry) {
        prev = entry->prev;
        next = entry->next;

        if (prev)
            prev->next = next;
        else
            list = next;
        if (next)
            next->prev = prev;

        entry->prev = NULL;
        entry->next = NULL;
    }

    return list;
}

struct list *__list_delete(struct list *list,
                           struct list *entry)
{
    list = __list_remove(list, entry);
    __list_free(entry);

    return list;
}

struct list *list_delete(struct list *list, void *data)
{
    struct list *ptr, *tmp;

    list_foreach_safe(list, ptr, tmp) {
        if (ptr->data == data) {
            list = __list_delete(list, ptr);
            break;
        }
    }

    return list;
}

struct list *list_delete_all(struct list *list, void *data)
{
    struct list *ptr, *tmp;

    list_foreach_safe(list, ptr, tmp) {
        if (ptr->data == data)
            list = __list_delete(list, ptr);
    }

    return list;
}

struct list *list_find(struct list *list, void *data)
{
    struct list *ptr;

    list_foreach(list, ptr) {
        if (ptr->data == data)
            break;
    }

    return ptr;
}

struct list *list_find_reverse(struct list *list, void *data)
{
    struct list *ptr;

    list_foreach_reverse(list, ptr) {
        if (ptr->data == data)
            break;
    }

    return ptr;
}
