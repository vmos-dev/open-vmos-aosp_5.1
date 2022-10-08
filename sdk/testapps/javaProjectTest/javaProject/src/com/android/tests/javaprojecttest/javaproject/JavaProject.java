package com.android.tests.javaprojecttest.javaproject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

public class JavaProject {

    public static String getContent() {
        InputStream input = JavaProject.class.getResourceAsStream("/com/android/tests/javaprojecttest/javaproject/JavaProject.txt");
        if (input == null) {
            return "FAILED TO FIND JavaProject.txt";
        }

        BufferedReader reader = null;
        try {
            reader = new BufferedReader(new InputStreamReader(input, "UTF-8"));

            return reader.readLine();
        } catch (IOException e) {
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException e) {
                }
            }
        }
        
        return "FAILED TO READ CONTENT";
    }
}
