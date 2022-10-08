package com.android.tests.buildconfigtest;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

import com.android.tests.buildconfigtest.lib1.Lib1;

public class Main extends Activity
{
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        if (BuildConfig.DEBUG) {
            TextView tv = (TextView) findViewById(R.id.app);
            tv.setText("App: DEBUG");
        }

        TextView tv = (TextView) findViewById(R.id.lib1);
        tv.setText("Lib: " + Lib1.getContent());
    }
}
