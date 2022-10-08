package com.android.tests.basicjar3;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

public class BasicJar3 {
    
    public static String getContent() {
        InputStream input = BasicJar3.class.getResourceAsStream("/com/android/tests/basicjar3/basicJar3.txt");
        if (input == null) {
            return "FAILED TO FIND basicJar3.txt";
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