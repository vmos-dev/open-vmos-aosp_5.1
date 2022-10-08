package com.example.android.lnotifications;

import android.app.Notification;
import android.test.ActivityInstrumentationTestCase2;

/**
 * Unit tests for {@link HeadsUpNotificationFragment}.
 */
public class HeadsUpNotificationFragmentTest extends
        ActivityInstrumentationTestCase2<LNotificationActivity> {

    private LNotificationActivity mActivity;
    private HeadsUpNotificationFragment mFragment;

    public HeadsUpNotificationFragmentTest() {
        super(LNotificationActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
        // The first tab should be {@link HeadsUpNotificationFragment}, that is tested in the
        // {@link LNotificationActivityTest}.
        mActivity.getActionBar().setSelectedNavigationItem(0);
        getInstrumentation().waitForIdleSync();
        mFragment = (HeadsUpNotificationFragment) mActivity.getFragmentManager()
                .findFragmentById(R.id.container);
    }

    public void testPreconditions() {
        assertNotNull(mActivity);
        assertNotNull(mFragment);
        assertNotNull(mActivity.findViewById(R.id.heads_up_notification_description));
        assertNotNull(mActivity.findViewById(R.id.show_notification_button));
        assertNotNull(mActivity.findViewById(R.id.use_heads_up_checkbox));
    }

    public void testCreateNotification_verifyFullScreenIntentIsNotNull() {
        Notification notification = mFragment.createNotification(true);
        assertNotNull(notification.fullScreenIntent);
    }

    public void testCreateNotification_verifyFullScreenIntentIsNull() {
        Notification notification = mFragment.createNotification(false);
        assertNull(notification.fullScreenIntent);
    }

    // If Mockito can be used, mock the NotificationManager and tests Notifications are actually
    // created.
}
