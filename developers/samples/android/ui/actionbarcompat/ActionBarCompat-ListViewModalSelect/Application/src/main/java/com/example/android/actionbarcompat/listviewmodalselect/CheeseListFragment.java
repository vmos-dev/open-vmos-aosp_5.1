/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.example.android.actionbarcompat.listviewmodalselect;

import android.os.Bundle;
import android.support.v4.app.ListFragment;
import android.support.v7.app.ActionBarActivity;
import android.support.v7.view.ActionMode;
import android.util.SparseBooleanArray;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ArrayAdapter;
import android.widget.BaseAdapter;
import android.widget.ListView;

import com.example.android.common.actionbarcompat.MultiSelectionUtil;
import com.example.android.common.dummydata.Cheeses;

import java.util.ArrayList;
import java.util.Iterator;

/**
 * This ListFragment displays a list of cheeses, allowing the user to select multiple items
 * in a modal selection mode. In this example, the user can remove multiple items via the displayed
 * action mode.
 */
public class CheeseListFragment extends ListFragment {

    // ArrayList containing the cheese name Strings
    private ArrayList<String> mItems;

    // The Controller which provides CHOICE_MODE_MULTIPLE_MODAL-like functionality
    private MultiSelectionUtil.Controller mMultiSelectController;

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        // The items which will be displayed
        mItems = Cheeses.asList();

        // Set the ListAdapter so that the ListView displays the items
        setListAdapter(new ArrayAdapter<String>(getActivity(), R.layout.simple_selectable_list_item,
                        android.R.id.text1, mItems));

        // BEGIN_INCLUDE(attach_controller)
        // Attach a MultiSelectionUtil.Controller to the ListView, giving it an instance of
        // ModalChoiceListener (see below)
        mMultiSelectController = MultiSelectionUtil
                .attachMultiSelectionController(getListView(), (ActionBarActivity) getActivity(),
                        new ModalChoiceListener());

        // Allow the Controller to restore itself
        mMultiSelectController.restoreInstanceState(savedInstanceState);
        // END_INCLUDE(attach_controller)
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        // BEGIN_INCLUDE(save_instance)
        // Allow the Controller to save it's instance state so that any checked items are
        // stored
        if (mMultiSelectController != null) {
            mMultiSelectController.saveInstanceState(outState);
        }
        // END_INCLUDE(save_instance)
    }

    /**
     * The listener which is provided to MultiSelectionUtil to handle any multi-selection actions.
     * It is responsible for providing the menu to display on the action mode, and handling any
     * action item clicks.
     */
    private class ModalChoiceListener implements MultiSelectionUtil.MultiChoiceModeListener {

        @Override
        public void onItemCheckedStateChanged(ActionMode mode, int position, long id,
                boolean checked) {
        }

        @Override
        public boolean onCreateActionMode(ActionMode actionMode, Menu menu) {
            // Inflate the menu resource (res/menu/context_cheeses.xml) which will be displayed
            // in the action mode
            actionMode.getMenuInflater().inflate(R.menu.context_cheeses, menu);
            return true;
        }

        @Override
        public boolean onPrepareActionMode(ActionMode actionMode, Menu menu) {
            return true;
        }

        @Override
        public boolean onActionItemClicked(ActionMode actionMode, MenuItem menuItem) {
            // We switch on the menu item's id, so we know which is clicked
            switch (menuItem.getItemId()) {

                // Our item with the menu_remove ID is used to remove the item(s) from the list.
                // Here we retrieve which positions are currently checked, then iterate through the
                // list and remove the checked items. Once finished we notify the adapter to update
                // the ListView contents and finish the action mode.
                case R.id.menu_remove:
                    // Retrieve which positions are currently checked
                    final ListView listView = getListView();
                    SparseBooleanArray checkedPositions = listView.getCheckedItemPositions();

                    // Check to see if there are any checked items
                    if (checkedPositions.size() == 0) {
                        return false;
                    }

                    // Iterate through the items and remove any which are checked
                    final Iterator<String> iterator = mItems.iterator();
                    int i = 0;
                    while (iterator.hasNext()) {
                        // Call next() so that the iterator index moves
                        iterator.next();

                        // Remove the item if it is checked (and increment position)
                        if (checkedPositions.get(i++)) {
                            iterator.remove();
                        }
                    }

                    // Clear the ListView's checked items
                    listView.clearChoices();

                    // Finally, notify the adapter so that it updates the ListView
                    ((BaseAdapter) getListAdapter()).notifyDataSetChanged();

                    // As we're removing all of checked items, we'll close the action mode
                    actionMode.finish();

                    // We return true to signify that we have handled the action item click
                    return true;
            }

            return false;
        }

        @Override
        public void onDestroyActionMode(ActionMode actionMode) {
        }
    }
}