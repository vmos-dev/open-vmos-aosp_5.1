package com.google.android.DemoKit;

public class DemoKitTablet extends BaseActivity {
	private OutputController mOutputController;

	protected void hideControls() {
		super.hideControls();
		mOutputController = null;
	}

	protected void showControls() {
		super.showControls();
		mOutputController = new OutputController(this, true);
		mOutputController.accessoryAttached();
	}
}
