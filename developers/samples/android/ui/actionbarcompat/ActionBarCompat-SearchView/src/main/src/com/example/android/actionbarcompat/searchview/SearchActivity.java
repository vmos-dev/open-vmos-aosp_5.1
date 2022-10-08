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

package com.example.android.actionbarcompat.searchview;

import android.os.Bundle;
import android.support.v4.view.MenuItemCompat;
import android.support.v7.app.ActionBarActivity;
import android.support.v7.widget.SearchView;
import android.view.Menu;
import android.view.MenuItem;

/**
 * This sample shows you how to use {@link SearchView} to provide a search function in a single
 * Activity, when utilizing ActionBarCompat.
 *
 * This Activity extends from {@link ActionBarActivity}, which provides all of the function
 * necessary to display a compatible Action Bar on devices running Android v2.1+.
 */
public class SearchActivity extends ActionBarActivity {

    private AppListFragment mAppListFragment;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Retrieve the AppListFragment from the layout
        mAppListFragment = (AppListFragment) getSupportFragmentManager()
                .findFragmentById(R.id.frag_app_list);
    }

    // BEGIN_INCLUDE(create_menu)
    /**
     * Use this method to instantiate your menu, inflating our {@code main} menu resource.
     */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate our menu from the resources by using the menu inflater.
        getMenuInflater().inflate(R.menu.main, menu);

        return super.onCreateOptionsMenu(menu);
    }
    // END_INCLUDE(create_menu)


    /**
     * This is called just before the menu is about to be displayed. You should
     * use this method to modify your menu or its items. In this example, we
     * retrieve the SearchView.
     */
    @Override
    public boolean onPrepareOptionsMenu(Menu menu) {
        // BEGIN_INCLUDE(retrieve_view)
        // First we retrieve the searchable menu item
        MenuItem searchItem = menu.findItem(R.id.menu_search);

        // Now we get the SearchView from the item
        final SearchView searchView = (SearchView) MenuItemCompat.getActionView(searchItem);
        // END_INCLUDE(retrieve_view)

        // Set the hint text which displays when the SearchView is empty
        searchView.setQueryHint(getString(R.string.search_hint));

        // BEGIN_INCLUDE(action_expand_listener)
        // We now set OnActionExpandListener on the menu item so we can reset SearchView's query
        // when it is collapsed.
        MenuItemCompat.setOnActionExpandListener(searchItem,
                new MenuItemCompat.OnActionExpandListener() {
                    @Override
                    public boolean onMenuItemActionExpand(MenuItem menuItem) {
                        // Return true to allow the action view to expand
                        return true;
                    }

                    @Override
                    public boolean onMenuItemActionCollapse(MenuItem menuItem) {
                        // When the action view is collapsed, reset the query
                        searchView.setQuery(null, true);

                        // Return true to allow the action view to collapse
                        return true;
                    }
                });
        // END_INCLUDE(action_expand_listener)

        // BEGIN_INCLUDE(query_text_listener)
        // We now set a OnQueryTextListener so we are notified when the query has changed
        searchView.setOnQueryTextListener(new SearchView.OnQueryTextListener() {
            @Override
            public boolean onQueryTextSubmit(String s) {
                // Propagate query to our AppListFragment
                mAppListFragment.setFilterQuery(s);

                // Return true to signify that we have handled the query submit
                return true;
            }

            @Override
            public boolean onQueryTextChange(String s) {
                // Propagate query to our AppListFragment
                mAppListFragment.setFilterQuery(s);

                // Return true to signify that we have handled the query change
                return true;
            }
        });
        // END_INCLUDE(query_text_listener)

        return super.onPrepareOptionsMenu(menu);
    }
}
