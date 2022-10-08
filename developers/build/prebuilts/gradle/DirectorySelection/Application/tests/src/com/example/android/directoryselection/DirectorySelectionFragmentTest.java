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

/**
 * Tests for {@link com.example.android.directoryselection.DirectorySelectionFragment}.
 */
public class DirectorySelectionFragmentTest
        extends ActivityInstrumentationTestCase2<DirectorySelectionActivity> {

    private DirectorySelectionActivity mTestActivity;
    private DirectorySelectionFragment mTestFragment;

    public DirectorySelectionFragmentTest() {
        super(DirectorySelectionActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        // Starts the activity under test using the default Intent with:
        // action = {@link Intent#ACTION_MAIN}
        // flags = {@link Intent#FLAG_ACTIVITY_NEW_TASK}
        // All other fields are null or empty.
        mTestActivity = getActivity();
        mTestFragment = (DirectorySelectionFragment)
                mTestActivity.getSupportFragmentManager().getFragments().get(0);
    }

    public void testPreconditions() {
        assertNotNull(mTestFragment.mCurrentDirectoryTextView);
        assertNotNull(mTestFragment.mCreateDirectoryButton);
        assertNotNull(mTestFragment.mRecyclerView);
        assertNotNull(mTestFragment.mAdapter);
        assertNotNull(mTestFragment.mLayoutManager);
    }
}
