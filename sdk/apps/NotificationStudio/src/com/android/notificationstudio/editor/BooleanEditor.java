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

import android.view.View;
import android.view.ViewStub;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.Switch;

import com.android.notificationstudio.R;
import com.android.notificationstudio.editor.Editors.Editor;
import com.android.notificationstudio.model.EditableItem;

public class BooleanEditor implements Editor {

    public Runnable bindEditor(View v, final EditableItem item, final Runnable afterChange) {
        final ViewStub booleanEditorStub = (ViewStub) v.findViewById(R.id.boolean_editor_stub);
        booleanEditorStub.setLayoutResource(R.layout.boolean_editor);
        final Switch booleanEditor = (Switch) booleanEditorStub.inflate();
        Runnable updateSwitch = new Runnable() {
            public void run() {
                booleanEditor.setChecked(item.hasValue() && item.getValueBool());
            }};
        booleanEditor.setVisibility(View.VISIBLE);
        updateSwitch.run();
        booleanEditor.setOnCheckedChangeListener(new OnCheckedChangeListener(){
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                item.setValue(isChecked);
                afterChange.run();
            }});
        return updateSwitch;
    }

}