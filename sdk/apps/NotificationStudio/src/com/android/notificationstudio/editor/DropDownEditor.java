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

import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

import com.android.notificationstudio.R;
import com.android.notificationstudio.editor.Editors.Editor;
import com.android.notificationstudio.model.EditableItem;

public class DropDownEditor implements Editor {

    public Runnable bindEditor(View v, final EditableItem item, final Runnable afterChange) {
        final Spinner dropDownEditor = (Spinner) v.findViewById(R.id.drop_down_editor);
        dropDownEditor.setVisibility(View.VISIBLE);
        Integer[] values = item.getAvailableValuesInteger();
        final float textSize = v.getResources().getDimensionPixelSize(R.dimen.editor_text_size);
        final int p = v.getResources().getDimensionPixelSize(R.dimen.editor_drop_down_padding);
        final int p2 = p * 2;
        final ArrayAdapter<Integer> adapter =
            new ArrayAdapter<Integer>(v.getContext(), android.R.layout.simple_spinner_item, values) {

            @Override
            public View getDropDownView(int position, View convertView, ViewGroup parent) {
                TextView v = (TextView) super.getDropDownView(position, convertView, parent);
                v.setTextSize(TypedValue.COMPLEX_UNIT_PX, textSize);
                v.setPadding(p2, p2, p2, p2);
                v.setText(v.getResources().getString(getItem(position)));
                return v;
            }

            @Override
            public View getView(int position, View convertView, ViewGroup parent) {
                TextView v = (TextView) super.getView(position, convertView, parent);
                v.setTextSize(TypedValue.COMPLEX_UNIT_PX, textSize);
                v.setPadding(p, p, p, p);
                v.setText(v.getResources().getString(getItem(position)));
                return v;
            }
        };
        dropDownEditor.setAdapter(adapter);
        Runnable updateSelection = new Runnable() {
            public void run() {
                dropDownEditor.setSelection(adapter.getPosition(item.getValueInt()));
            }};
        if (item.hasValue())
            updateSelection.run();
        dropDownEditor.setOnItemSelectedListener(new OnItemSelectedListener(){
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                Object oldValue = item.getValue();
                Object newValue = adapter.getItem(position);
                if (newValue.equals(oldValue))
                    return;
                item.setValue(newValue);
                afterChange.run();
            }
            public void onNothingSelected(AdapterView<?> parent) {
                // noop
            }});
        return updateSelection;
    }

}