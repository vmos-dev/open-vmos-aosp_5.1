/*
 * Copyright (C) 2013 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package libcore.java.util;


import junit.framework.TestCase;
import java.util.Calendar;
import java.util.GregorianCalendar;

public class GregorianCalendarTest extends TestCase {

    // https://code.google.com/p/android/issues/detail?id=61993
    public void test_computeFields_dayOfWeekAndWeekOfYearSet() {
        Calendar greg = GregorianCalendar.getInstance();

        // Setting WEEK_OF_YEAR and DAY_OF_WEEK with an intervening
        // call to computeFields will work.
        greg.set(Calendar.WEEK_OF_YEAR, 1);
        assertEquals(1, greg.get(Calendar.WEEK_OF_YEAR));
        greg.set(Calendar.DAY_OF_WEEK, Calendar.MONDAY);
        assertEquals(1, greg.get(Calendar.WEEK_OF_YEAR));

        // Setting WEEK_OF_YEAR after DAY_OF_WEEK with no intervening
        // call to computeFields will work.
        greg = GregorianCalendar.getInstance();
        greg.set(Calendar.DAY_OF_WEEK, Calendar.MONDAY);
        greg.set(Calendar.WEEK_OF_YEAR, 1);
        assertEquals(1, greg.get(Calendar.WEEK_OF_YEAR));
        assertEquals(Calendar.MONDAY, greg.get(Calendar.DAY_OF_WEEK));

        // Setting DAY_OF_WEEK *after* WEEK_OF_YEAR with no intervening computeFields
        // will make WEEK_OF_YEAR have no effect. This is a limitation of the API.
        // Combinations are chosen based *only* on the value of the last field set,
        // which in this case is DAY_OF_WEEK.
        greg = GregorianCalendar.getInstance();
        int weekOfYear = greg.get(Calendar.WEEK_OF_YEAR);
        greg.set(Calendar.WEEK_OF_YEAR, 1);
        greg.set(Calendar.DAY_OF_WEEK, Calendar.MONDAY);
        // Unchanged WEEK_OF_YEAR.
        assertEquals(weekOfYear, greg.get(Calendar.WEEK_OF_YEAR));
    }
}
