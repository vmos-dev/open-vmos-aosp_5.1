package com.android.tests.javaprojecttest.lib1;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

public class Lib1 {

    public static String getContent() {
        InputStream input = Lib1.class.getResourceAsStream("Lib1.txt");
        if (input == null) {
            return "FAILED TO FIND Lib1.txt";
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
