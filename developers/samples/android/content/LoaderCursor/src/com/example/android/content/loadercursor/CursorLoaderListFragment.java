/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.example.android.content.loadercursor;

import android.app.ListFragment;
import android.app.LoaderManager;
import android.content.CursorLoader;
import android.content.Loader;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.ContactsContract.Contacts;
import android.text.TextUtils;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.ListView;
import android.widget.SearchView;
import android.widget.SearchView.OnCloseListener;
import android.widget.SearchView.OnQueryTextListener;
import android.widget.SimpleCursorAdapter;
import android.widget.Toast;

/**
 * A {@link ListFragment} that shows the use of a {@link LoaderManager} to
 * display a list of contacts accessed through a {@link Cursor}.
 */
public class CursorLoaderListFragment extends ListFragment implements
        LoaderManager.LoaderCallbacks<Cursor> {

    // This is the Adapter being used to display the list's data.
    SimpleCursorAdapter mAdapter;

    // The SearchView for doing filtering.
    SearchView mSearchView;

    // If non-null, this is the current filter the user has provided.
    String mCurFilter;

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        // Give some text to display if there is no data. In a real
        // application this would come from a resource.
        setEmptyText("No phone numbers");

        // We have a menu item to show in action bar.
        setHasOptionsMenu(true);

        /*
         * Create an empty adapter we will use to display the loaded data. The
         * simple_list_item_2 layout contains two rows on top of each other
         * (text1 and text2) that will show the contact's name and status.
         */
        mAdapter = new SimpleCursorAdapter(getActivity(),
                android.R.layout.simple_list_item_2, null,
                new String[] {
                        Contacts.DISPLAY_NAME, Contacts.CONTACT_STATUS
                },
                new int[] {
                        android.R.id.text1, android.R.id.text2
                }, 0);
        setListAdapter(mAdapter);

        // Start out with a progress indicator.
        setListShown(false);

        // BEGIN_INCLUDE(getloader)
        // Prepare the loader. Either re-connect with an existing one,
        // or start a new one.
        getLoaderManager().initLoader(0, null, this);
        // END_INCLUDE(getloader)
    }

    @Override
    public void onCreateOptionsMenu(Menu menu, MenuInflater inflater) {

        inflater.inflate(R.menu.main, menu);

        // Get the search item and update its action view
        MenuItem item = menu.findItem(R.id.action_search);
        mSearchView = (SearchView) item.getActionView();
        mSearchView.setOnQueryTextListener(queryListener);
        mSearchView.setOnCloseListener(closeListener);
        mSearchView.setIconifiedByDefault(true);
    }

    /**
     * The {@link OnCloseListener} called when the SearchView is closed. Resets
     * the query field.
     */
    private SearchView.OnCloseListener closeListener = new SearchView.OnCloseListener() {

        @Override
        public boolean onClose() {
            // Restore the SearchView if a query was entered
            if (!TextUtils.isEmpty(mSearchView.getQuery())) {
                mSearchView.setQuery(null, true);
            }
            return true;
        }
    };

    /**
     * The {@link OnQueryTextListener} that is called when text in the
     * {@link SearchView} is changed. Updates the query filter and triggers a
     * reload of the {@link LoaderManager} to update the displayed list.
     */
    private OnQueryTextListener queryListener = new OnQueryTextListener() {

        /**
         * Called when the action bar search text has changed. Update the search
         * filter, and restart the loader to do a new query with this filter.
         *
         * @param newText the new content of the query text field
         * @return true, the action has been handled.
         */
        public boolean onQueryTextChange(String newText) {

            String newFilter = !TextUtils.isEmpty(newText) ? newText : null;

            // Don't do anything if the filter hasn't actually changed.
            // Prevents restarting the loader when restoring state.
            if (mCurFilter == null && newFilter == null) {
                return true;
            }
            if (mCurFilter != null && mCurFilter.equals(newFilter)) {
                return true;
            }

            // Restart the Loader.
            // #onCreateLoader uses the value of mCurFilter as a filter when
            // creating the query for the Loader.
            mCurFilter = newFilter;
            getLoaderManager().restartLoader(0, null, CursorLoaderListFragment.this);

            return true;
        }

        @Override
        public boolean onQueryTextSubmit(String query) {
            // Don't care about this.
            return true;
        }
    };

    /**
     * An item has been clicked in the {@link ListView}. Display a toast with
     * the tapped item's id.
     */
    @Override
    public void onListItemClick(ListView l, View v, int position, long id) {
        Toast.makeText(getActivity(), "Item clicked: " + id, Toast.LENGTH_LONG).show();
    }

    // These are the Contacts rows that we will retrieve.
    static final String[] CONTACTS_SUMMARY_PROJECTION = new String[] {
            Contacts._ID,
            Contacts.DISPLAY_NAME,
            Contacts.CONTACT_STATUS,
            Contacts.LOOKUP_KEY,
    };

    // BEGIN_INCLUDE(oncreateloader)
    public Loader<Cursor> onCreateLoader(int id, Bundle args) {
        // This is called when a new Loader needs to be created. This
        // sample only has one Loader, so we don't care about the ID.
        // First, pick the base URI to use depending on whether we are
        // currently filtering.
        Uri baseUri;
        if (mCurFilter != null) {
            baseUri = Uri.withAppendedPath(Contacts.CONTENT_FILTER_URI,
                    Uri.encode(mCurFilter));
        } else {
            baseUri = Contacts.CONTENT_URI;
        }

        // Now create and return a CursorLoader that will take care of
        // creating a Cursor for the data being displayed.
        String select = "((" + Contacts.DISPLAY_NAME + " NOTNULL) AND ("
                + Contacts.HAS_PHONE_NUMBER + "=1) AND ("
                + Contacts.DISPLAY_NAME + " != '' ))";
        String order = Contacts.DISPLAY_NAME + " COLLATE LOCALIZED ASC";

        return new CursorLoader(getActivity(), baseUri,
                CONTACTS_SUMMARY_PROJECTION, select, null, order);
    }

    // END_INCLUDE(oncreateloader)

    // BEGIN_INCLUDE(onloadfinished)
    public void onLoadFinished(Loader<Cursor> loader, Cursor data) {
        // Swap the new cursor in. (The framework will take care of closing the
        // old cursor once we return.)
        mAdapter.swapCursor(data);

        // The list should now be shown.
        if (isResumed()) {
            setListShown(true);
        } else {
            setListShownNoAnimation(true);
        }
    }

    // END_INCLUDE(onloadfinished)

    // BEGIN_INCLUDE(onloaderreset)
    public void onLoaderReset(Loader<Cursor> loader) {
        // This is called when the last Cursor provided to onLoadFinished()
        // above is about to be closed. We need to make sure we are no
        // longer using it.
        mAdapter.swapCursor(null);
    }
    // END_INCLUDE(onloaderreset)

}
