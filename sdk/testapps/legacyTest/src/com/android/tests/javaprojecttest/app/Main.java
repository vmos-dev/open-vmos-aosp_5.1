package com.android.tests.javaprojecttest.app;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

import com.android.tests.basicjar.BasicJar;

public class Main extends Activity {
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        TextView tv = (TextView) findViewById(R.id.basicJar);
        tv.setText("basicJar: " + BasicJar.getContent());
    }
}