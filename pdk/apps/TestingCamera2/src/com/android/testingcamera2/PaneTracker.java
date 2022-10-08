/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.testingcamera2;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import android.os.Handler;
import android.os.Looper;

/**
 * Tracks currently-available panes of various kinds, for other panes to find targets/etc.
 *
 */
public class PaneTracker {

    private Set<ControlPane> mActivePanes = new HashSet<ControlPane>();
    private Set<PaneSetChangedListener<?>> mActiveListeners = new HashSet<PaneSetChangedListener<?>>();

    /**
     * Various events panes might alert other panes about
     */
    public enum PaneEvent {
        NEW_CAMERA_SELECTED,
        CAMERA_CONFIGURED
    }

    public PaneTracker() {
    }

    public void addPane(ControlPane pane) {
        boolean added = mActivePanes.add(pane);
        if (!added) return;
        for (PaneSetChangedListener<?> listener : mActiveListeners) {
            if (listener.getFilterType().isInstance(pane)) {
                listener.onPaneAdded(pane);
            }
        }
    }

    public void removePane(ControlPane pane) {
        boolean existed = mActivePanes.remove(pane);
        if (!existed) return;
        for (PaneSetChangedListener<?> listener : mActiveListeners) {
            if (listener.getFilterType().isInstance(pane)) {
                listener.onPaneRemoved(pane);
            }
        }
    }

    public <T extends ControlPane> List<T> getPanes(Class<T> paneClass) {
        List<T> filteredPanes = new ArrayList<T>();
        for (ControlPane pane : mActivePanes ) {
            if (paneClass.isInstance(pane)) {
                filteredPanes.add(paneClass.cast(pane));
            }
        }
        return filteredPanes;
    }

    public void notifyOtherPanes(final ControlPane sourcePane, final PaneEvent event) {
        Handler mainHandler = new Handler(Looper.getMainLooper());
        mainHandler.post(new Runnable() {
            public void run() {
                for (ControlPane pane: mActivePanes ) {
                    pane.notifyPaneEvent(sourcePane, event);
                }
            }
        });
    }

    /**
     * Notify all panes that the UI orientation has changed
     *
     * @param orientation one of the Surface.ROTATION_* constants
     */
    public void notifyOrientationChange(int orientation) {
        for (ControlPane pane: mActivePanes ) {
            pane.onOrientationChange(orientation);
        }
    }

    public void addPaneListener(PaneSetChangedListener<?> listener) {
        mActiveListeners.add(listener);
    }

    public void removePaneListener(PaneSetChangedListener<?> listener) {
        mActiveListeners.remove(listener);
    }

    /**
     * Interface for clients to listen to additions and removals of panes
     * of specific types.
     */
    public static abstract class PaneSetChangedListener<T extends ControlPane> {
        private Class<T> mFilterType;

        public PaneSetChangedListener(Class<T> filterType) {
            mFilterType = filterType;
        }

        public Class<T> getFilterType() {
            return mFilterType;
        }

        public abstract void onPaneAdded(ControlPane pane);
        public abstract void onPaneRemoved(ControlPane pane);
    }

}
