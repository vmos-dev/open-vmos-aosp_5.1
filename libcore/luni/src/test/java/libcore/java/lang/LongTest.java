/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package libcore.java.lang;

import java.util.Properties;

public class LongTest extends junit.framework.TestCase {

    public void testSystemProperties() {
        Properties originalProperties = System.getProperties();
        try {
            Properties testProperties = new Properties();
            testProperties.put("testIncLong", "string");
            System.setProperties(testProperties);
            assertNull(Long.getLong("testIncLong"));
            assertEquals(new Long(4), Long.getLong("testIncLong", 4L));
            assertEquals(new Long(4), Long.getLong("testIncLong", new Long(4)));
        } finally {
            System.setProperties(originalProperties);
        }
    }

    public void testCompare() throws Exception {
        final long min = Long.MIN_VALUE;
        final long zero = 0L;
        final long max = Long.MAX_VALUE;
        assertTrue(Long.compare(max,  max)  == 0);
        assertTrue(Long.compare(min,  min)  == 0);
        assertTrue(Long.compare(zero, zero) == 0);
        assertTrue(Long.compare(max,  zero) > 0);
        assertTrue(Long.compare(max,  min)  > 0);
        assertTrue(Long.compare(zero, max)  < 0);
        assertTrue(Long.compare(zero, min)  > 0);
        assertTrue(Long.compare(min,  zero) < 0);
        assertTrue(Long.compare(min,  max)  < 0);
    }

    public void testSignum() throws Exception {
        assertEquals(0, Long.signum(0));
        assertEquals(1, Long.signum(1));
        assertEquals(-1, Long.signum(-1));
        assertEquals(1, Long.signum(Long.MAX_VALUE));
        assertEquals(-1, Long.signum(Long.MIN_VALUE));
    }

    public void testParsePositiveLong() throws Exception {
        assertEquals(0, Long.parsePositiveLong("0", 10));
        assertEquals(473, Long.parsePositiveLong("473", 10));
        assertEquals(255, Long.parsePositiveLong("FF", 16));

        try {
            Long.parsePositiveLong("-1", 10);
            fail();
        } catch (NumberFormatException e) {}

        try {
            Long.parsePositiveLong("+1", 10);
            fail();
        } catch (NumberFormatException e) {}

        try {
            Long.parsePositiveLong("+0", 16);
            fail();
        } catch (NumberFormatException e) {}
    }

    public void testParseLong() throws Exception {
        assertEquals(0, Long.parseLong("+0", 10));
        assertEquals(473, Long.parseLong("+473", 10));
        assertEquals(255, Long.parseLong("+FF", 16));
        assertEquals(102, Long.parseLong("+1100110", 2));
        assertEquals(Long.MAX_VALUE, Long.parseLong("+" + Long.MAX_VALUE, 10));
        assertEquals(411787, Long.parseLong("Kona", 27));
        assertEquals(411787, Long.parseLong("+Kona", 27));
        assertEquals(-145, Long.parseLong("-145", 10));

        try {
            Long.parseLong("--1", 10); // multiple sign chars
            fail();
        } catch (NumberFormatException expected) {}

        try {
            Long.parseLong("++1", 10); // multiple sign chars
            fail();
        } catch (NumberFormatException expected) {}

        try {
            Long.parseLong("Kona", 10); // base to small
            fail();
        } catch (NumberFormatException expected) {}
    }

    public void testDecodeLong() throws Exception {
        assertEquals(0, Long.decode("+0").longValue());
        assertEquals(473, Long.decode("+473").longValue());
        assertEquals(255, Long.decode("+0xFF").longValue());
        assertEquals(16, Long.decode("+020").longValue());
        assertEquals(Long.MAX_VALUE, Long.decode("+" + Long.MAX_VALUE).longValue());
        assertEquals(-73, Long.decode("-73").longValue());
        assertEquals(-255, Long.decode("-0xFF").longValue());
        assertEquals(255, Long.decode("+#FF").longValue());
        assertEquals(-255, Long.decode("-#FF").longValue());

        try {
            Long.decode("--1"); // multiple sign chars
            fail();
        } catch (NumberFormatException expected) {}

        try {
            Long.decode("++1"); // multiple sign chars
            fail();
        } catch (NumberFormatException expected) {}

        try {
            Long.decode("+-1"); // multiple sign chars
            fail();
        } catch (NumberFormatException expected) {}

        try {
            Long.decode("Kona"); // invalid number
            fail();
        } catch (NumberFormatException expected) {}
    }

}
