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

import android.text.Editable;
import android.text.InputType;
import android.text.TextWatcher;
import android.view.View;
import android.widget.EditText;

import com.android.notificationstudio.R;
import com.android.notificationstudio.editor.Editors.Editor;
import com.android.notificationstudio.model.EditableItem;

public class TextEditor implements Editor {

    public Runnable bindEditor(View v, final EditableItem item, final Runnable afterChange) {
        final EditText textEditor = (EditText) v.findViewById(R.id.text_editor);
        textEditor.setVisibility(View.VISIBLE);
        textEditor.setInputType(getInputType());
        Runnable updateEditText = new Runnable() {
            public void run() {
                textEditor.setText(item.getValue() == null ? "" : item.getValue().toString());
            }};
        if (item.hasValue())
            updateEditText.run();
        textEditor.addTextChangedListener(new TextWatcher() {
            public void afterTextChanged(Editable s) {
                Object newVal = parseValue(s.length() == 0 ? null : s.toString());
                if (equal(newVal, item.getValue()))
                    return;
                item.setValue(newVal);
                afterChange.run();
            }

            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                // noop
            }

            public void onTextChanged(CharSequence s, int start, int before, int count) {
                // noop
            }
        });
        return updateEditText;
    }

    protected int getInputType() {
        return InputType.TYPE_CLASS_TEXT;
    }

    protected Object parseValue(String str) {
        return str;
    }

    private static boolean equal(Object a,  Object b) {
        return a == b || (a != null && a.equals(b));
    }

}