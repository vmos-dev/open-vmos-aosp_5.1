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

import android.content.Context;
import android.util.Pair;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;

import java.util.ArrayList;
import java.util.List;

/**
 * A specialized adapter containing an array of checkboxes.
 *
 * <p>
 * This adapter contains an array of pairs, where each pair represents the name and checked
 * state of a given item.
 * </p>
 */
public class CheckableListAdapter extends ArrayAdapter<CheckableListAdapter.CheckableItem> {
    private Context mContext;

    public CheckableListAdapter(Context context, int resource, List<CheckableItem> objects) {
        super(context, resource, objects);
        mContext = context;
    }

    public static class CheckableItem {
        public String name;
        public boolean isChecked;

        public CheckableItem(String name, boolean isChecked) {
            this.name = name;
            this.isChecked = isChecked;
        }
    }

    @Override
    public View getView(final int position, View convertView, ViewGroup parent) {
        CheckBox row = (CheckBox) convertView;
        if (row == null) {
            LayoutInflater inflater =
                    (LayoutInflater)mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            row = (CheckBox) inflater.inflate(R.layout.checkable_list_item, parent, false);
        }
        CheckableItem item = getItem(position);
        row.setChecked(item.isChecked);
        row.setText(item.name);

        row.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                CheckableItem item = getItem(position);
                item.isChecked = b;
            }
        });
        return row;
    }

    /**
     * Returns a list containing the indexes of the currently checked items.
     *
     * @return a {@link java.util.List} of indices.
     */
    public List<Integer> getCheckedPositions() {
        ArrayList<Integer> checkedPositions = new ArrayList<Integer>();
        int size = getCount();
        for (int i = 0; i < size; i++) {
            CheckableItem item = getItem(i);
            if (item.isChecked) {
                checkedPositions.add(i);
            }
        }

        return checkedPositions;
    }

    /**
     * Update the items in this list.  Checked state will be preserved for items that are
     * still included in the list.
     *
     * @param elems a list of strings that represents the names of the items to be included.
     */
    public void updateItems(String[] elems) {
        ArrayList<CheckableItem> newList = new ArrayList<CheckableItem>();
        for (String e : elems) {
            CheckableItem item = new CheckableItem(e, false);
            newList.add(item);
            boolean newItem = true;
            int size = getCount();
            for (int i = 0; i < size; i++) {
                CheckableItem current = getItem(i);
                if (current.name.equals(e) && current.isChecked) {
                    item.isChecked = true;
                    newItem = false;
                }
            }
            if (newItem) {
                item.isChecked = newItem;
            }
        }
        clear();
        addAll(newList);
        notifyDataSetChanged();
    }

}
