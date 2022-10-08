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

package com.android.notificationstudio.model;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

import com.android.notificationstudio.R;

public enum EditableItem implements EditableItemConstants {

    PRESET(R.string.preset, TYPE_DROP_DOWN, CATEGORY_MAIN,
            PRESET_BASIC, PRESET_EMAIL, PRESET_PHOTO, PRESET_CUSTOM),
    SMALL_ICON(R.string.small_icon, TYPE_RESOURCE_ID, CATEGORY_MAIN,
            SMALL_ICONS),
    CONTENT_TITLE(R.string.content_title, TYPE_TEXT, CATEGORY_MAIN),
    CONTENT_TEXT(R.string.content_text, TYPE_TEXT, CATEGORY_MAIN),
    SUB_TEXT(R.string.sub_text, TYPE_TEXT, CATEGORY_MAIN),
    LARGE_ICON(R.string.large_icon, TYPE_BITMAP, CATEGORY_MAIN),
    CONTENT_INFO(R.string.content_info, TYPE_TEXT, CATEGORY_MAIN),
    NUMBER(R.string.number, TYPE_INT, CATEGORY_MAIN),
    WHEN(R.string.when, TYPE_DATETIME, CATEGORY_MAIN),
    PROGRESS(R.string.progress, TYPE_BOOLEAN, CATEGORY_MAIN),
    USES_CHRON(R.string.uses_chron, TYPE_BOOLEAN, CATEGORY_MAIN),
    STYLE(R.string.style, TYPE_DROP_DOWN, CATEGORY_STYLE,
            STYLE_NONE, STYLE_BIG_PICTURE, STYLE_BIG_TEXT, STYLE_INBOX),
    PICTURE(R.string.picture, TYPE_BITMAP, CATEGORY_STYLE),
    BIG_TEXT(R.string.big_text, TYPE_TEXT, CATEGORY_STYLE),
    LINES(R.string.lines, TYPE_TEXT_LINES, CATEGORY_STYLE),
    BIG_CONTENT_TITLE(R.string.big_content_title, TYPE_TEXT, CATEGORY_STYLE),
    SUMMARY_TEXT(R.string.summary_text, TYPE_TEXT, CATEGORY_STYLE),
    ACTION1_ICON(R.string.icon, TYPE_RESOURCE_ID, CATEGORY_ACTION1,
            ACTION_ICONS),
    ACTION1_TEXT(R.string.text, TYPE_TEXT, CATEGORY_ACTION1),
    ACTION2_ICON(R.string.icon, TYPE_RESOURCE_ID, CATEGORY_ACTION2,
            ACTION_ICONS),
    ACTION2_TEXT(R.string.text, TYPE_TEXT, CATEGORY_ACTION2),
    ACTION3_ICON(R.string.icon, TYPE_RESOURCE_ID, CATEGORY_ACTION3,
            ACTION_ICONS),
    ACTION3_TEXT(R.string.text, TYPE_TEXT, CATEGORY_ACTION3),
    ;

    private final int mCaptionId;
    private final int mType;
    private final int mCategoryId;

    private Object[] mAvailableValues;
    private Object mValue;
    private boolean mVisible = true;
    private Runnable mVisibilityListener;

    private EditableItem(int captionId, int type, int categoryId, Object... availableValues) {
        mCaptionId = captionId;
        mType = type;
        mCategoryId = categoryId;
        mAvailableValues = availableValues;
    }

    // init
    public static void initIfNecessary(Context context) {
        if (PRESET.hasValue())
            return;
        loadBitmaps(context, LARGE_ICON, LARGE_ICONS);
        loadBitmaps(context, PICTURE, PICTURES);
        PRESET.setValue(PRESET_BASIC);
    }

    private static void loadBitmaps(Context context, EditableItem item, int[] bitmapResIds) {
        Object[] largeIconBitmaps = new Object[bitmapResIds.length];
        Resources res = context.getResources();
        for (int i = 0; i < bitmapResIds.length; i++)
            largeIconBitmaps[i] = BitmapFactory.decodeResource(res, bitmapResIds[i]);
        item.setAvailableValues(largeIconBitmaps);
    }

    // visibility
    public boolean isVisible() {
        return mVisible;
    }

    public void setVisible(boolean visible) {
        if (mVisible == visible)
            return;
        mVisible = visible;
        if (mVisibilityListener != null)
            mVisibilityListener.run();
    }

    public void setVisibilityListener(Runnable listener) {
        mVisibilityListener = listener;
    }

    // value

    public boolean hasValue() {
        return mValue != null;
    }

    public void setValue(Object value) {
        if (mValue == value)
            return;
        mValue = value;
        if (this == STYLE)
            applyStyle();
        if (this == PRESET && !PRESET_CUSTOM.equals(value))
            applyPreset();
    }

    private void applyStyle() {
        PICTURE.setVisible(STYLE_BIG_PICTURE.equals(mValue));
        BIG_TEXT.setVisible(STYLE_BIG_TEXT.equals(mValue));
        LINES.setVisible(STYLE_INBOX.equals(mValue));
        BIG_CONTENT_TITLE.setVisible(!STYLE_NONE.equals(mValue));
        SUMMARY_TEXT.setVisible(!STYLE_NONE.equals(mValue));
    }

    private void applyPreset() {
        for (EditableItem item : values())
            if (item != PRESET)
                item.setValue(null);
        STYLE.setValue(STYLE_NONE);
        if (PRESET_BASIC.equals(mValue)) {
            SMALL_ICON.setValue(android.R.drawable.stat_notify_chat);
            CONTENT_TITLE.setValue("Basic title");
            CONTENT_TEXT.setValue("Basic text");
        } else if (PRESET_EMAIL.equals(mValue)) {
            SMALL_ICON.setValue(R.drawable.ic_notification_multiple_mail_holo_dark);
            LARGE_ICON.setValue(LARGE_ICON.getAvailableValues()[3]);
            CONTENT_TITLE.setValue("3 new messages");
            CONTENT_TEXT.setValue("Alice, Bob, Chuck");
            STYLE.setValue(STYLE_INBOX);
            LINES.setValue("Alice: Re: Something\n" +
                           "Bob: Did you get the memo?\n" +
                           "Chuck: Limited time offer!");
        } else if (PRESET_PHOTO.equals(mValue)) {
            SMALL_ICON.setValue(android.R.drawable.ic_menu_camera);
            LARGE_ICON.setValue(LARGE_ICON.getAvailableValues()[2]);
            CONTENT_TITLE.setValue("Sunset on the rocks");
            CONTENT_TEXT.setValue("800x534 | 405.1K");
            SUMMARY_TEXT.setValue(CONTENT_TEXT.getValueString());
            STYLE.setValue(STYLE_BIG_PICTURE);
            PICTURE.setValue(PICTURE.getAvailableValues()[0]);
            ACTION1_ICON.setValue(android.R.drawable.ic_menu_share);
            ACTION1_TEXT.setValue("Share");
        }
    }

    public Object getValue() {
        return mValue;
    }

    public String getValueString() {
        return (String) mValue;
    }

    public int getValueInt() {
        return (Integer) mValue;
    }

    public long getValueLong() {
        return (Long) mValue;
    }

    public boolean getValueBool() {
        return (Boolean) mValue;
    }

    public Bitmap getValueBitmap() {
        return (Bitmap) mValue;
    }

    // available values

    public Object[] getAvailableValues() {
        return mAvailableValues;
    }

    public Integer[] getAvailableValuesInteger() {
        Integer[] integers = new Integer[mAvailableValues.length];
        System.arraycopy(mAvailableValues, 0, integers, 0, integers.length);
        return integers;
    }

    public <T> void setAvailableValues(T... values) {
        mAvailableValues = values;
    }

    public String getCaption(Context context) {
        return context.getString(mCaptionId);
    }

    public String getCategory(Context context) {
        return context.getString(mCategoryId);
    }

    public int getType() {
        return mType;
    }

}
