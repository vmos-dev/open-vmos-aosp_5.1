package com.android.tests.buildconfigtest.lib1;

import com.android.tests.javaprojecttest.lib1.BuildConfig;


public class Lib1 {

    public static String getContent() {
        String a = "Release";
        
        if (BuildConfig.DEBUG) {
            a = "DEBUG";
        }
        
        return a;
    }
}
