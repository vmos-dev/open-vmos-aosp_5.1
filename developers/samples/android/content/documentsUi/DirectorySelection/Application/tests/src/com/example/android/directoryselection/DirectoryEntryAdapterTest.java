/*
* Copyright 2014 The Android Open Source Project
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
package com.example.android.directoryselection;

import android.test.ActivityInstrumentationTestCase2;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests for {@link DirectorySelectionFragment}.
 */
public class DirectoryEntryAdapterTest
        extends ActivityInstrumentationTestCase2<DirectorySelectionActivity> {

    private static final String FILE1 = "file1";
    private static final String MIME_TYPE1 = "text/appliaction";
    private static final String DIRECTORY1 = "directory1";

    private DirectorySelectionActivity mTestActivity;
    private DirectorySelectionFragment mTestFragment;
    private DirectoryEntryAdapter mAdapter;
    private List<DirectoryEntry> mDirectoryEntries;

    public DirectoryEntryAdapterTest() {
        super(DirectorySelectionActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mTestActivity = getActivity();
        mTestFragment = (DirectorySelectionFragment)
                mTestActivity.getSupportFragmentManager().getFragments().get(0);
        mDirectoryEntries = new ArrayList<>();

        DirectoryEntry file = new DirectoryEntry();
        file.fileName = FILE1;
        file.mimeType = MIME_TYPE1;
        mDirectoryEntries.add(file);

        DirectoryEntry directory = new DirectoryEntry();
        directory.fileName = DIRECTORY1;
        directory.mimeType = DirectoryEntryAdapter.DIRECTORY_MIME_TYPE;
        mDirectoryEntries.add(directory);
    }

    public void testGetItemCount() {
        mTestFragment.mAdapter.setDirectoryEntries(mDirectoryEntries);

        assertEquals(2, mTestFragment.mAdapter.getItemCount());
    }
}
