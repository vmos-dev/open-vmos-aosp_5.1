package com.example.android.lnotifications;

import android.app.Activity;
import android.app.Notification;
import android.content.Intent;
import android.net.Uri;
import android.test.ActivityInstrumentationTestCase2;
import android.util.Log;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;

/**
 * Unit tests for {@link OtherMetadataFragment}.
 */
public class OtherMetadataFragmentTest extends
        ActivityInstrumentationTestCase2<LNotificationActivity> {

    private LNotificationActivity mActivity;
    private OtherMetadataFragment mFragment;

    public OtherMetadataFragmentTest() {
        super(LNotificationActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
        // The third tab should be {@link OtherMetadataFragment}, that is tested in the
        // {@link LNotificationActivityTest}.
        mActivity.getActionBar().setSelectedNavigationItem(2);
        getInstrumentation().waitForIdleSync();
        mFragment = (OtherMetadataFragment) mActivity.getFragmentManager()
                .findFragmentById(R.id.container);
    }

    public void testPreconditions() {
        assertNotNull(mActivity);
        assertNotNull(mFragment);
        assertNull(mFragment.mContactUri);
        assertNotNull(mActivity.findViewById(R.id.attach_person));
        assertNotNull(mActivity.findViewById(R.id.category_spinner));
        assertNotNull(mActivity.findViewById(R.id.priority_spinner));
        assertNotNull(mActivity.findViewById(R.id.show_notification_button));
    }

    public void testCreateNotification_verifyPriorityAndCategoryWithoutPerson() {
        Notification notification = mFragment.createNotification(OtherMetadataFragment.Priority
                .HIGH, OtherMetadataFragment.Category.CALL, null);
        assertEquals(Notification.PRIORITY_HIGH, notification.priority);
        assertEquals(Notification.CATEGORY_CALL, notification.category);
    }

    public void testCreateNotification_verifyPriorityAndCategoryWithPerson() {
        String tel = "81 (90) 555-1212";
        Uri dummyContactUri = Uri.fromParts("tel", tel, null);
        Notification notification = mFragment.createNotification(OtherMetadataFragment.Priority
                .DEFAULT, OtherMetadataFragment.Category.MESSAGE, dummyContactUri);
        assertEquals(Notification.PRIORITY_DEFAULT, notification.priority);
        assertEquals(Notification.CATEGORY_MESSAGE, notification.category);

        String[] peopleArray = notification.extras.getStringArray(Notification.EXTRA_PEOPLE);
        assertNotNull(peopleArray);
        assertEquals(1, peopleArray.length);
    }

    public void testActionPickResultUpdatesContactInstanceField() {
        getInstrumentation().runOnMainSync(new Runnable() {
            @Override
            public void run() {
                String tel = "81 (90) 555-1212";
                Uri dummyContactUri = Uri.fromParts("tel", tel, null);
                Intent intent = new Intent(Intent.ACTION_PICK);
                intent.setData(dummyContactUri);
                mFragment.onActivityResult(mFragment.REQUEST_CODE_PICK_CONTACT,
                        Activity.RESULT_OK, intent);
            }
        });
        getInstrumentation().waitForIdleSync();
        assertNotNull(mFragment.mContactUri);
    }
}
