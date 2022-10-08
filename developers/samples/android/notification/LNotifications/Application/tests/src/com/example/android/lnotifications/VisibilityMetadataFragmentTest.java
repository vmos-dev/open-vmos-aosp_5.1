package com.example.android.lnotifications;

import android.app.Notification;
import android.test.ActivityInstrumentationTestCase2;

/**
 * Unit tests for {@link VisibilityMetadataFragment}.
 */
public class VisibilityMetadataFragmentTest extends
        ActivityInstrumentationTestCase2<LNotificationActivity> {

    private LNotificationActivity mActivity;
    private VisibilityMetadataFragment mFragment;

    public VisibilityMetadataFragmentTest() {
        super(LNotificationActivity.class);
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = getActivity();
        // The second tab should be {@link VisibilityMetadataFragment}, that is tested in the
        // {@link LNotificationActivityTest}.
        mActivity.getActionBar().setSelectedNavigationItem(1);
        getInstrumentation().waitForIdleSync();
        mFragment = (VisibilityMetadataFragment) mActivity.getFragmentManager()
                .findFragmentById(R.id.container);
    }

    public void testPreconditions() {
        assertNotNull(mActivity);
        assertNotNull(mFragment);
        assertNotNull(mActivity.findViewById(R.id.visibility_metadata_notification_description));
        assertNotNull(mActivity.findViewById(R.id.visibility_radio_group));
        assertNotNull(mActivity.findViewById(R.id.visibility_private_radio_button));
        assertNotNull(mActivity.findViewById(R.id.visibility_secret_radio_button));
        assertNotNull(mActivity.findViewById(R.id.visibility_public_radio_button));
        assertNotNull(mActivity.findViewById(R.id.show_notification_button));
    }

    public void testCreateNotification_publicVisibility() {
        Notification notification = mFragment.createNotification(VisibilityMetadataFragment
                .NotificationVisibility.PUBLIC);

        assertEquals(Notification.VISIBILITY_PUBLIC, notification.visibility);
        assertEquals(R.drawable.ic_public_notification, notification.icon);
    }

    public void testCreateNotification_privateVisibility() {
        Notification notification = mFragment.createNotification(VisibilityMetadataFragment
                .NotificationVisibility.PRIVATE);

        assertEquals(Notification.VISIBILITY_PRIVATE, notification.visibility);
        assertEquals(R.drawable.ic_private_notification, notification.icon);
    }

    public void testCreateNotification_secretVisibility() {
        Notification notification = mFragment.createNotification(VisibilityMetadataFragment
                .NotificationVisibility.SECRET);

        assertEquals(Notification.VISIBILITY_SECRET, notification.visibility);
        assertEquals(R.drawable.ic_secret_notification, notification.icon);
    }
}

