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

import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.graphics.drawable.Drawable;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v4.app.ListFragment;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ImageView;
import android.widget.TextView;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;

/**
 * A ListFragment which displays a list of applications currently on the device, with
 * filtering functionality.
 */
public class AppListFragment extends ListFragment {

    // Stores the full list of applications installed on the device
    private final List<ApplicationItem> mInstalledApps = new ArrayList<ApplicationItem>();

    // Stores the result of the last query
    private final  List<ApplicationItem> mFilteredApps = new ArrayList<ApplicationItem>();

    private PackageManager mPackageManager;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Retrieve all of the applications installed on this device
        mPackageManager = getActivity().getPackageManager();

        // Start an AsyncTask to load the application's installed on the device
        new LoadApplicationsTask().execute();
    }

    @Override
    public void onViewCreated(View view, Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        // Set the ListView's Adapter to display the filtered applications
        setListAdapter(new AppAdapter());

        // Set the text which displays when the ListView is empty
        setEmptyText(getString(R.string.applist_empty_text));

        // If the installed applications is not loaded yet, hide the list
        if (mInstalledApps.isEmpty()) {
            setListShown(false);
        }
    }

    /**
     * Set the query used to filter the installed application's names.
     * @param query Query to filter against.
     */
    public void setFilterQuery(String query) {
        // Fail-fast if the installed apps has not been loaded yet
        if (mInstalledApps.isEmpty()) {
            return;
        }

        // Clear current filtered apps and hide ListView
        mFilteredApps.clear();

        if (TextUtils.isEmpty(query)) {
            // If the query is empty, show all install apps
            mFilteredApps.addAll(mInstalledApps);
        } else {
            // Compile Regex Pattern which will match if the app name contains the query
            final Pattern p = Pattern.compile(".*" + query + ".*", Pattern.CASE_INSENSITIVE);

            // Iterate through the installed apps to see if their label matches the query
            for (ApplicationItem application : mInstalledApps) {
                // If the app label matches the query, add it to the filtered apps list
                if (p.matcher(application.label).matches()) {
                    mFilteredApps.add(application);
                }
            }
        }

        // Notify the adapter so that it updates the ListView's contents
        AppAdapter adapter = (AppAdapter) getListAdapter();
        adapter.notifyDataSetChanged();
    }

    private class AppAdapter extends BaseAdapter {
        private final LayoutInflater mInflater;

        AppAdapter() {
            mInflater = LayoutInflater.from(getActivity());
        }

        @Override
        public int getCount() {
            return mFilteredApps.size();
        }

        @Override
        public ApplicationItem getItem(int position) {
            return mFilteredApps.get(position);
        }

        @Override
        public long getItemId(int position) {
            return position;
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            if (convertView == null) {
                convertView = mInflater.inflate(R.layout.list_item, parent, false);
            }

            final ApplicationItem item = getItem(position);

            // Set the application's label on the TextView
            ((TextView) convertView.findViewById(android.R.id.text1))
                    .setText(item.label);

            // ImageView to display the application's icon
            ImageView imageView = (ImageView) convertView.findViewById(R.id.icon);

            // Check the item's drawable reference to see if it is available already
            Drawable icon = item.drawable != null ? item.drawable.get() : null;

            if (icon != null) {
                imageView.setImageDrawable(icon);
            } else {
                // Start a new AsyncTask which will retrieve the application icon and display it in
                // the ImageView
                new ApplicationIconTask(imageView).execute(item);
            }

            return convertView;
        }
    }

    /**
     * Our model object for each application item. Allows us to load the label async and store the
     * result for use later (filtering and displaying in the adapter).
     */
    private static class ApplicationItem {
        final ApplicationInfo applicationInfo;
        final CharSequence label;
        WeakReference<Drawable> drawable;

        ApplicationItem(PackageManager packageManager, ApplicationInfo applicationInfo) {
            this.applicationInfo = applicationInfo;

            // Load and store the app's label using the PackageManager
            this.label = applicationInfo.loadLabel(packageManager);
        }
    }

    /**
     * Simple AsyncTask which retrieves the installed applications from the {@link PackageManager}
     * and stores them locally.
     */
    private class LoadApplicationsTask extends AsyncTask<Void, Void, List<ApplicationItem>> {

        @Override
        protected List<ApplicationItem> doInBackground(Void... voids) {
            // Load all installed applications on the device
            List<ApplicationInfo> apps = mPackageManager.getInstalledApplications(0);
            // Create an ArrayList used to store the result
            ArrayList<ApplicationItem> loadedApps = new ArrayList<ApplicationItem>();

            // Iterate through the results and create an ApplicationItem instance for each one,
            // adding them to the returned List.
            for (ApplicationInfo info : apps) {
                loadedApps.add(new ApplicationItem(mPackageManager, info));
            }

            return loadedApps;
        }

        @Override
        protected void onPostExecute(List<ApplicationItem> loadedApps) {
            super.onPostExecute(loadedApps);

            // Add the results to the install apps list
            mInstalledApps.addAll(loadedApps);

            // Reset the query to update the ListView
            setFilterQuery(null);

            // Make sure the list is shown
            setListShown(true);
        }
    }

    /**
     * Simple AsyncTask which loads the given application's logo from the {@link PackageManager} and
     * displays it in the given {@link ImageView}
     */
    private class ApplicationIconTask extends AsyncTask<ApplicationItem, Void, Drawable> {
        private final ImageView mImageView;

        ApplicationIconTask(ImageView imageView) {
            mImageView = imageView;
        }

        @Override
        protected Drawable doInBackground(ApplicationItem... params) {
            Drawable icon = null;

            // Check to see that we've been provided one item
            if (params.length == 1) {
                ApplicationItem item = params[0];

                // Load the icon from the PackageManager
                icon = item.applicationInfo.loadIcon(mPackageManager);

                // Store the icon in a WeakReference so we do not cause a OutOfMemoryError.
                item.drawable = new WeakReference<Drawable>(icon);
            }

            return icon;
        }

        @Override
        protected void onPostExecute(Drawable icon) {
            super.onPostExecute(icon);
            // Display the icon in the ImageView
            mImageView.setImageDrawable(icon);
        }
    }

}
