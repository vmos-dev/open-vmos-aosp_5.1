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

package com.android.notificationstudio.editor;

import android.content.res.Resources;
import android.view.MotionEvent;
import android.view.SoundEffectConstants;
import android.view.View;
import android.view.View.OnTouchListener;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.HorizontalScrollView;
import android.widget.ImageView;
import android.widget.ImageView.ScaleType;
import android.widget.LinearLayout;

import com.android.notificationstudio.R;
import com.android.notificationstudio.editor.Editors.Editor;
import com.android.notificationstudio.model.EditableItem;

public class IconEditor implements Editor {

    public Runnable bindEditor(View v, final EditableItem item, final Runnable afterChange) {
        final LinearLayout iconEditor = (LinearLayout) v.findViewById(R.id.icon_editor_layout);
        final HorizontalScrollView scroller =
                (HorizontalScrollView) v.findViewById(R.id.icon_editor_scroller);
        scroller.setVisibility(View.VISIBLE);

        final Object[] displayValues = getAvailableValuesForDisplay(item);
        final Runnable updateSelection = new Runnable() {
            public void run() {
                for (int i=0;i<iconEditor.getChildCount();i++) {
                    View imageViewHolder = iconEditor.getChildAt(i);
                    Object iconResId = imageViewHolder.getTag();
                    boolean selected = item.hasValue() && item.getValue().equals(iconResId) ||
                            !item.hasValue() && iconResId == null;
                    imageViewHolder.setSelected(selected);
                    if (selected) {
                        int x = imageViewHolder.getLeft();
                        if (x < scroller.getScrollX() ||
                            x > scroller.getScrollX() + scroller.getWidth()) {
                            scroller.scrollTo(imageViewHolder.getLeft(), 0);
                        }
                    }
                }
            }};

        int iconSize = getIconSize(v.getResources());
        int outerMargin = v.getResources().getDimensionPixelSize(R.dimen.editor_icon_outer_margin);
        int innerMargin = v.getResources().getDimensionPixelSize(R.dimen.editor_icon_inner_margin);

        for (final Object iconResId : displayValues) {
            final FrameLayout imageViewHolder = new FrameLayout(v.getContext());
            imageViewHolder.setTag(iconResId);
            final ImageView imageView = new ImageView(v.getContext());
            imageView.setScaleType(ScaleType.CENTER);
            imageView.setOnTouchListener(new OnTouchListener(){
                public boolean onTouch(View v, MotionEvent event) {
                    if (event.getAction() == MotionEvent.ACTION_UP) {
                        v.playSoundEffect(SoundEffectConstants.CLICK);
                        item.setValue(iconResId);
                        updateSelection.run();
                        afterChange.run();
                    }
                    return true;
                }});

            imageViewHolder.setBackgroundResource(R.drawable.icon_bg);
            if (iconResId != null)
                setImage(imageView, iconResId);

            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(iconSize, iconSize);
            lp.bottomMargin = lp.topMargin = lp.leftMargin = lp.rightMargin = outerMargin;
            imageViewHolder.setLayoutParams(lp);

            FrameLayout.LayoutParams flp = new FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT);
            flp.bottomMargin = flp.topMargin = flp.leftMargin = flp.rightMargin = innerMargin;
            imageView.setLayoutParams(flp);

            imageViewHolder.addView(imageView);
            iconEditor.addView(imageViewHolder);
        }
        updateSelection.run();
        return updateSelection;
    }

    protected int getIconSize(Resources res) {
        return res.getDimensionPixelSize(R.dimen.editor_icon_size_small);
    }

    protected void setImage(ImageView imageView, Object value) {
        imageView.setImageResource((Integer) value);
    }

    private Object[] getAvailableValuesForDisplay(EditableItem item) {
        Object[] avail = item.getAvailableValues();
        Object[] rt = new Object[avail.length + 1];
        System.arraycopy(avail, 0, rt, 1, avail.length);
        return rt;
    }

}