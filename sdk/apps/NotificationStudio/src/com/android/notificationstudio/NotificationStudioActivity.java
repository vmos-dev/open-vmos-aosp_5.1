/*
 * Copyright 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.notificationstudio;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.app.Activity;
import android.app.Notification;
import android.app.NotificationManager;
import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnLayoutChangeListener;
import android.view.ViewGroup;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.FrameLayout.LayoutParams;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RemoteViews;
import android.widget.TextView;

import com.android.notificationstudio.action.ShareCodeAction;
import com.android.notificationstudio.action.ShareMockupAction;
import com.android.notificationstudio.editor.Editors;
import com.android.notificationstudio.generator.NotificationGenerator;
import com.android.notificationstudio.model.EditableItem;
import com.android.notificationstudio.model.EditableItemConstants;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class NotificationStudioActivity extends Activity implements EditableItemConstants{
    private static final String TAG = NotificationStudioActivity.class.getSimpleName();
    private static final int PREVIEW_NOTIFICATION = 1;
    private static final int REFRESH_DELAY = 50;
    private static final ExecutorService BACKGROUND = Executors.newSingleThreadExecutor();

    private boolean mRefreshPending;

    private final Handler mHandler = new Handler();
    private final Runnable mRefreshNotificationInner = new Runnable() {
        public void run() {
            refreshNotificationInner();
        }};

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getWindow().setBackgroundDrawableResource(android.R.color.black);
        setContentView(R.layout.studio);
        initPreviewScroller();

        EditableItem.initIfNecessary(this);

        initEditors();
    }

    private void initPreviewScroller() {

        MaxHeightScrollView preview = (MaxHeightScrollView) findViewById(R.id.preview_scroller);
        if (preview == null)
            return;
        final int margin = ((ViewGroup.MarginLayoutParams) preview.getLayoutParams()).bottomMargin;
        preview.addOnLayoutChangeListener(new OnLayoutChangeListener(){
            public void onLayoutChange(View v, int left, int top, int right, int bottom,
                    int oldLeft, int oldTop, int oldRight, int oldBottom) {
                // animate preview height changes
                if (oldBottom != bottom) {
                    final View e = findViewById(R.id.editors);
                    final int y = bottom + margin;
                    e.animate()
                        .translationY(y - oldBottom)
                        .setListener(new AnimatorListenerAdapter() {
                            public void onAnimationEnd(Animator animation) {
                                FrameLayout.LayoutParams lp = (LayoutParams) e.getLayoutParams();
                                lp.topMargin = y;
                                e.setTranslationY(0);
                                e.setLayoutParams(lp);
                            }
                        });
                }
            }});

        // limit the max height for preview, leave room for editors + soft keyboard
        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        float actualHeight = dm.heightPixels / dm.ydpi;
        float pct = actualHeight < 3.5 ? .32f :
                    actualHeight < 4 ? .35f :
                    .38f;
        preview.setMaxHeight((int)(dm.heightPixels * pct));
    }

    private void initEditors() {
        LinearLayout items = (LinearLayout) findViewById(R.id.items);
        items.removeAllViews();
        String currentCategory = null;
        for (EditableItem item : EditableItem.values()) {
            String itemCategory = item.getCategory(this);
            if (!itemCategory.equals(currentCategory)) {
                View dividerView = getLayoutInflater().inflate(R.layout.divider, null);
                ((TextView) dividerView.findViewById(R.id.divider_text)).setText(itemCategory);
                items.addView(dividerView);
                currentCategory = itemCategory;
            }
            View editorView = Editors.newEditor(this, items, item);
            if (editorView != null)
                items.addView(editorView);
        }
        refreshNotification();
    }

    @Override
    protected void onRestoreInstanceState(Bundle savedInstanceState) {
       // we'll take care of restoring state
    }

    public void refreshNotification() {
        mRefreshPending = true;
        mHandler.postDelayed(mRefreshNotificationInner, REFRESH_DELAY);
    }

    private void refreshNotificationInner() {
        if (!mRefreshPending) {
            return;
        }
        final Notification notification = NotificationGenerator.build(this);
        ViewGroup oneU = (ViewGroup) findViewById(R.id.oneU);
        ViewGroup fourU = (ViewGroup) findViewById(R.id.fourU);
        View oneUView = refreshRemoteViews(oneU, notification.contentView);
        if (Build.VERSION.SDK_INT >= 16)
            refreshRemoteViews(fourU, notification.bigContentView);
        else if (Build.VERSION.SDK_INT >= 11) {
            ImageView largeIcon = (ImageView) findViewById(R.id.large_icon);
            largeIcon.setVisibility(notification.largeIcon == null ? View.GONE : View.VISIBLE);
            if (notification.largeIcon != null)
                largeIcon.setImageBitmap(notification.largeIcon);
        } else if (oneUView != null) {
            oneUView.setBackgroundColor(getResources().getColor(R.color.gb_background));
            oneUView.setMinimumHeight(100);
        }
        mRefreshPending = false;

        // this can take a while, run on a background thread
        BACKGROUND.submit(new Runnable() {
            public void run() {
                NotificationManager mgr =
                        (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
                try {
                    mgr.notify(PREVIEW_NOTIFICATION, notification);
                } catch (Throwable t) {
                    Log.w(TAG, "Error displaying notification", t);
                }
            }});
    }

    private View refreshRemoteViews(ViewGroup parent, RemoteViews remoteViews) {
        parent.removeAllViews();
        if (remoteViews != null) {
            parent.setVisibility(View.VISIBLE);
            try {
                View v = remoteViews.apply(this, parent);
                parent.addView(v);
                return v;
            } catch (Exception e) {
                TextView exceptionView = new TextView(this);
                exceptionView.setText(e.getClass().getSimpleName() + ": " + e.getMessage());
                parent.addView(exceptionView);
                return exceptionView;
            }
        } else {
            parent.setVisibility(View.GONE);
            return null;
        }
    }

    // action bar setup
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.action_bar, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.action_share_code:
                ShareCodeAction.launch(this, item.getTitle());
                return true;
            case R.id.action_share_mockup:
                ShareMockupAction.launch(this, item.getTitle());
                return true;
        }
        return false;
    }

    // hides the soft keyboard more aggressively when leaving text editors
    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        View v = getCurrentFocus();
        boolean ret = super.dispatchTouchEvent(event);

        if (v instanceof EditText) {
            View currentFocus = getCurrentFocus();
            int screenCoords[] = new int[2];
            currentFocus.getLocationOnScreen(screenCoords);
            float x = event.getRawX() + currentFocus.getLeft() - screenCoords[0];
            float y = event.getRawY() + currentFocus.getTop() - screenCoords[1];

            if (event.getAction() == MotionEvent.ACTION_UP
                    && (x < currentFocus.getLeft() ||
                        x >= currentFocus.getRight() ||
                        y < currentFocus.getTop() ||
                        y > currentFocus.getBottom())) {
                InputMethodManager imm =
                    (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                imm.hideSoftInputFromWindow(getWindow().getCurrentFocus().getWindowToken(), 0);
            }
        }
        return ret;
    }

}
