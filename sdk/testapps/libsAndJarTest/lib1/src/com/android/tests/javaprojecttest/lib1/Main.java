package com.android.tests.javaprojecttest.lib1;

import android.app.Activity;
import android.os.Bundle;

import com.android.tests.javaprojecttest.lib2.Lib2;

public class Main extends Activity
{
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        //Access some class from Lib2 to make sure we can access them.
        String foo = Lib2.getContent();
    }
}
