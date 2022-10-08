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

import android.os.Build;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.android.notificationstudio.NotificationStudioActivity;
import com.android.notificationstudio.R;
import com.android.notificationstudio.model.EditableItem;
import com.android.notificationstudio.model.EditableItemConstants;

import java.util.HashMap;
import java.util.Map;

public class Editors implements EditableItemConstants {

    public interface Editor {
        Runnable bindEditor(View v, EditableItem item, Runnable afterChange);
    }

    private static final Map<Integer, Editor> EDITORS = editors();
    private static Runnable sUpdatePreset;

    private static Map<Integer, Editor> editors() {
        Map<Integer, Editor> editors = new HashMap<Integer, Editor>();
        editors.put(TYPE_RESOURCE_ID, new IconEditor());
        editors.put(TYPE_TEXT, new TextEditor());
        editors.put(TYPE_INT, new IntEditor());
        if (Build.VERSION.SDK_INT >= 14)  // switch 14, progress 14, uses chron 16
            editors.put(TYPE_BOOLEAN, new BooleanEditor());
        editors.put(TYPE_DROP_DOWN, new DropDownEditor());
        editors.put(TYPE_BITMAP, new BitmapEditor());
        if (Build.VERSION.SDK_INT >= 11) // fragments 11, when 11
            editors.put(TYPE_DATETIME, new DateTimeEditor());
        editors.put(TYPE_TEXT_LINES, new LinesEditor());
        return editors;
    }

    public static View newEditor(final NotificationStudioActivity activity,
            final ViewGroup parent, final EditableItem item) {
        final View editorView = activity.getLayoutInflater().inflate(R.layout.editable_item, null);
        ((TextView) editorView.findViewById(R.id.caption)).setText(item.getCaption(activity));

        // bind visibility
        editorView.setVisibility(item.isVisible() ? View.VISIBLE : View.GONE);
        item.setVisibilityListener(new Runnable(){
            public void run() {
                editorView.setVisibility(item.isVisible() ? View.VISIBLE : View.GONE);
            }});

        // bind type-specific behavior
        Editor editor = EDITORS.get(item.getType());
        if (editor == null)
            return null;
        Runnable updater = editor.bindEditor(editorView, item, new Runnable() {
            public void run() {
                if (item.equals(EditableItem.PRESET)) {
                    updateEditors(parent);
                } else {
                    EditableItem.PRESET.setValue(PRESET_CUSTOM);
                    sUpdatePreset.run();
                }
                activity.refreshNotification();
            }});

        // store the updater as the view tag
        editorView.setTag(updater);
        if (item.equals(EditableItem.PRESET))
            sUpdatePreset = updater;

        return editorView;
    }

    private static void updateEditors(ViewGroup parent) {
        for (int i = 0; i < parent.getChildCount(); i++) {
            Object childTag = parent.getChildAt(i).getTag();
            if (childTag instanceof Runnable) {
                ((Runnable) childTag).run();
            }
        }
    }

}
