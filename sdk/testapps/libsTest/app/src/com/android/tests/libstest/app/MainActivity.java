package com.android.tests.libstest.app;

import android.app.Activity;
import android.os.Bundle;

import com.android.tests.libstest.lib1.Lib1;
import com.android.tests.libstest.lib2.Lib2;

public class MainActivity extends Activity {
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        App.handleTextView(this);
        Lib1.handleTextView(this);
        Lib2.handleTextView(this);
    }
}