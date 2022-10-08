package com.android.tests.basicjar;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

public class BasicJar {
    
    public static String getContent() {
        InputStream input = BasicJar.class.getResourceAsStream("/com/android/tests/basicjar/basicJar.txt");
        if (input == null) {
            return "FAILED TO FIND basicJar.txt";
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