/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.android.common.util;

/**
 * Helper class for creating pools of objects. Creating new objects is an
 * expensive operation, which can lead to significant performance overhead if
 * new objects of the same type are allocated and destroyed during run time.
 * These performance issues can be mitigated by reusing unused objects and
 * reinitializing them, rather than destroying and removing them from memory.
 * <p>
 * The object pool pattern provided by the {@link Pool} interface facilitates
 * the reuse of objects by keeping unused ('released') objects in memory and
 * making them available for use. This can provide a significant performance
 * improvement, as objects are only created once and returned to the Pool when
 * no longer required, rather than destroyed and reallocated. Object
 * {@link Pools} keep track of these unused objects. An object pool provides two
 * basic methods for access:
 * <ul>
 * <li><b>{@link Pool#acquire()}:</b> Returns an used object if one is
 * available.</li>
 * <li><b> {@link Pool#release(Object)}:</b> Adds the given object to the pool,
 * ready to be reallocated in acquire().</li>
 * </ul>
 * <p>
 * This class contains the interface defining a {@link Pool}, an implementation
 * based on a fixed length array ({@link SimplePool}) and a synchronized pool
 * for use with concurrency ({@link SynchronizedPool}).
 * <p>
 * A {@link SimplePool} can be used like this:
 *
 * <pre>
 * public class MyPooledClass {
 *
 *     private static final SynchronizedPool<MyPooledClass> sPool =
 *             new SynchronizedPool<MyPooledClass>(10);
 *
 *     public static MyPooledClass obtain() {
 *         MyPooledClass instance = sPool.acquire();
 *         return (instance != null) ? instance : new MyPooledClass();
 *     }
 *
 *     public void recycle() {
 *          // Clear state if needed.
 *          sPool.release(this);
 *     }
 *
 *     . . .
 * }
 * </pre>
 */
public final class Pools {

    /**
     * Interface for managing a pool of objects.
     *
     * @param T The pooled type.
     */
    public static interface Pool<T> {

        /**
         * Retrieves an object from the pool. Returns null if the pool is empty
         * and no object is available.
         *
         * @return An instance from the pool if available, null otherwise.
         */
        public T acquire();

        /**
         * Releases an instance to the pool. This marks the object as reusable
         * and makes it available through a call to {@link #acquire()}. An
         * object should not be modified or accessed once it has been released.
         *
         * @param instance The instance to release.
         * @return True if the instance was put in the pool.
         * @throws IllegalStateException If the instance is already in the pool.
         */
        public boolean release(T instance);
    }

    private Pools() {
        /* do nothing - hiding constructor */
    }

    /**
     * Simple (non-synchronized) pool of objects. This class provides a simple,
     * fixed sized pool of objects.
     *
     * @param T The pooled type.
     */
    public static class SimplePool<T> implements Pool<T> {
        private final Object[] mPool;

        private int mPoolSize;

        /**
         * Creates a new instance. The parameter defines the maximum number of
         * objects that can be held in this pool.
         *
         * @param maxPoolSize The max pool size.
         * @throws IllegalArgumentException If the max pool size is less than
         *             zero.
         */
        public SimplePool(int maxPoolSize) {
            if (maxPoolSize <= 0) {
                throw new IllegalArgumentException("The max pool size must be > 0");
            }
            mPool = new Object[maxPoolSize];
        }

        /**
         * Returns an object from the pool or null if the pool is empty.
         *
         * @return An object from the pool or null if no object is available.
         */
        @Override
        @SuppressWarnings("unchecked")
        public T acquire() {
            if (mPoolSize > 0) {
                final int lastPooledIndex = mPoolSize - 1;
                T instance = (T) mPool[lastPooledIndex];
                mPool[lastPooledIndex] = null;
                mPoolSize--;
                return instance;
            }
            return null;
        }

        /**
         * Adds an object to the pool. If the pool is already full (its
         * allocated size has been exceeded), the object is not added and false
         * is returned. A linear check is performed to ensure that the object is
         * not already held in the pool.
         *
         * @param instance The element to release.
         * @return True if the object was added to the pool.
         * @throws IllegalStateException If the object already exists in the
         *             pool.
         */
        @Override
        public boolean release(T instance) {
            if (isInPool(instance)) {
                throw new IllegalStateException("Already in the pool!");
            }
            if (mPoolSize < mPool.length) {
                mPool[mPoolSize] = instance;
                mPoolSize++;
                return true;
            }
            return false;
        }

        /**
         * Checks if the object already exists in the pool.
         * @param instance The element to look for.
         * @return True if the object exists in the pool.
         */
        private boolean isInPool(T instance) {
            for (int i = 0; i < mPoolSize; i++) {
                if (mPool[i] == instance) {
                    return true;
                }
            }
            return false;
        }
    }

    /**
     * Synchronized pool of objects. Based on the implementation of a fixed size
     * pool in {@link SimplePool}, this class provides synchronized concurrent
     * access to the pool.
     *
     * @param T The pooled type.
     */
    public static class SynchronizedPool<T> extends SimplePool<T> {
        private final Object mLock = new Object();

        /**
         * Creates a new instance.
         *
         * @param maxPoolSize The max pool size.
         * @throws IllegalArgumentException If the max pool size is less than
         *             zero.
         */
        public SynchronizedPool(int maxPoolSize) {
            super(maxPoolSize);
        }

        /**
         * Returns an object from the pool or null if the pool is empty.
         * <p>
         * Access to the pool is synchronized.
         *
         * @return An object from the pool or null if no object is available.
         */
        @Override
        public T acquire() {
            synchronized (mLock) {
                return super.acquire();
            }
        }

        /**
         * Adds an object to the pool. If the pool is already full (its
         * allocated size has been exceeded), the object is not added and false
         * is returned. A linear check is performed to ensure that the object is
         * not already held in the pool.
         * <p>
         * Access to the pool is synchronized.
         *
         * @param element The element to be released
         * @return True if the object was added to the pool.
         * @throws IllegalStateException If the object already exists in the
         *             pool.
         */
        @Override
        public boolean release(T element) {
            synchronized (mLock) {
                return super.release(element);
            }
        }
    }
}
