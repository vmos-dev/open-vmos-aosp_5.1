/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package libcore.java.util;

import dalvik.annotation.AndroidOnly;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;
import junit.framework.TestCase;

public class OldTimeZoneTest extends TestCase {

    static class Mock_TimeZone extends TimeZone {
        @Override
        public int getOffset(int era, int year, int month, int day, int dayOfWeek, int milliseconds) {
            return 0;
        }

        @Override
        public int getRawOffset() {
            return 0;
        }

        @Override
        public boolean inDaylightTime(Date date) {
            return false;
        }

        @Override
        public void setRawOffset(int offsetMillis) {

        }

        @Override
        public boolean useDaylightTime() {
            return false;
        }
    }

    public void test_constructor() {
        assertNotNull(new Mock_TimeZone());
    }

    public void test_clone() {
        TimeZone tz1 = TimeZone.getDefault();
        TimeZone tz2 = (TimeZone)tz1.clone();

        assertTrue(tz1.equals(tz2));
    }

    public void test_getAvailableIDs() {
        String[] str = TimeZone.getAvailableIDs();
        assertNotNull(str);
        assertTrue(str.length != 0);
        for(int i = 0; i < str.length; i++) {
            assertNotNull(TimeZone.getTimeZone(str[i]));
        }
    }

    public void test_getAvailableIDsI() {
        String[] str = TimeZone.getAvailableIDs(0);
        assertNotNull(str);
        assertTrue(str.length != 0);
        for(int i = 0; i < str.length; i++) {
            assertNotNull(TimeZone.getTimeZone(str[i]));
        }
    }

    public void test_getDisplayName() {
        Locale.setDefault(Locale.US);
        TimeZone tz = TimeZone.getTimeZone("GMT-6");
        assertEquals("GMT-06:00", tz.getDisplayName());
        tz = TimeZone.getTimeZone("America/Los_Angeles");
        assertEquals("Pacific Standard Time", tz.getDisplayName());
    }

    public void test_getDisplayNameLjava_util_Locale() {
        TimeZone tz = TimeZone.getTimeZone("America/Los_Angeles");
        assertEquals("Pacific Standard Time", tz.getDisplayName(Locale.US));
        assertEquals("heure normale du Pacifique nord-américain", tz.getDisplayName(Locale.FRANCE));
    }

    public void test_getDisplayNameZI() {
        Locale.setDefault(Locale.US);
        TimeZone tz = TimeZone.getTimeZone("America/Los_Angeles");
        assertEquals("PST",                   tz.getDisplayName(false, TimeZone.SHORT));
        assertEquals("Pacific Daylight Time", tz.getDisplayName(true, TimeZone.LONG));
        assertEquals("Pacific Standard Time", tz.getDisplayName(false, TimeZone.LONG));
    }

    @AndroidOnly("fail on RI. See comment below")
    public void test_getDisplayNameZILjava_util_Locale() {
        TimeZone tz = TimeZone.getTimeZone("America/Los_Angeles");
        assertEquals("Pacific Daylight Time", tz.getDisplayName(true,  TimeZone.LONG, Locale.US));
        assertEquals("Pacific Standard Time", tz.getDisplayName(false, TimeZone.LONG, Locale.UK));
        assertEquals("heure avanc\u00e9e du Pacifique",
                tz.getDisplayName(true,  TimeZone.LONG, Locale.FRANCE));
        assertEquals("heure normale du Pacifique nord-américain",
                tz.getDisplayName(false, TimeZone.LONG, Locale.FRANCE));

        assertEquals("PDT", tz.getDisplayName(true, TimeZone.SHORT, Locale.US));
        assertEquals("PST", tz.getDisplayName(false, TimeZone.SHORT, Locale.US));
        // RI fails on following lines. RI always returns short time zone name for
        // "America/Los_Angeles" as "PST", Android only returns a string if ICU has a translation.
        // There is no short time zone name for America/Los_Angeles in French or British English in
        // ICU data so an offset is returned instead.
        assertEquals("GMT-08:00", tz.getDisplayName(false, TimeZone.SHORT, Locale.FRANCE));
        assertEquals("GMT-07:00", tz.getDisplayName(true, TimeZone.SHORT, Locale.FRANCE));
        assertEquals("GMT-08:00", tz.getDisplayName(false, TimeZone.SHORT, Locale.UK));
        assertEquals("GMT-07:00", tz.getDisplayName(true, TimeZone.SHORT, Locale.UK));

        // The RI behavior mentioned above does not appear to be because "PST" is a legacy
        // three-character timezone supported by the RI: it happens for "Asia/Tehran"/"IRST" too
        // (IRST is not a legacy code). The RI may just use a different dataset that has "PST" /
        // "IRST" as valid translations (even for scripts like Chinese).
        TimeZone iranTz = TimeZone.getTimeZone("Asia/Tehran");
        assertEquals("Iran Summer Time", iranTz.getDisplayName(true, TimeZone.LONG, Locale.UK));
        assertEquals("Iran Daylight Time", iranTz.getDisplayName(true, TimeZone.LONG, Locale.US));
        assertEquals("Iran Standard Time", iranTz.getDisplayName(false, TimeZone.LONG, Locale.UK));
        assertEquals("Iran Standard Time", iranTz.getDisplayName(false, TimeZone.LONG, Locale.US));
        assertEquals("GMT+03:30", iranTz.getDisplayName(false, TimeZone.SHORT, Locale.UK));
        assertEquals("GMT+04:30", iranTz.getDisplayName(true, TimeZone.SHORT, Locale.UK));
    }

    public void test_getID() {
        TimeZone tz = TimeZone.getTimeZone("GMT-6");
        assertEquals("GMT-06:00", tz.getID());
        tz = TimeZone.getTimeZone("America/Denver");
        assertEquals("America/Denver", tz.getID());
    }

    public void test_hasSameRulesLjava_util_TimeZone() {
        TimeZone tz1 = TimeZone.getTimeZone("America/Denver");
        TimeZone tz2 = TimeZone.getTimeZone("America/Phoenix");
        assertEquals(tz1.getDisplayName(false, 0), tz2.getDisplayName(false, 0));
        // Arizona doesn't observe DST. See http://phoenix.about.com/cs/weather/qt/timezone.htm
        assertFalse(tz1.hasSameRules(tz2));
        assertFalse(tz1.hasSameRules(null));
        tz1 = TimeZone.getTimeZone("America/New_York");
        tz2 = TimeZone.getTimeZone("US/Eastern");
        assertEquals(tz1.getDisplayName(), tz2.getDisplayName());
        assertFalse(tz1.getID().equals(tz2.getID()));
        assertTrue(tz2.hasSameRules(tz1));
        assertTrue(tz1.hasSameRules(tz1));
    }

    public void test_setIDLjava_lang_String() {
        TimeZone tz = TimeZone.getTimeZone("GMT-6");
        assertEquals("GMT-06:00", tz.getID());
        tz.setID("New ID for GMT-6");
        assertEquals("New ID for GMT-6", tz.getID());
    }
}
