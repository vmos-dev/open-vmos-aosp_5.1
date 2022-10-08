/*
 * Copyright (C) 2007 The Android Open Source Project
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
/*
 * Elements of the WallTime class are a port of Bionic's localtime.c to Java. That code had the
 * following header:
 *
 * This file is in the public domain, so clarified as of
 * 1996-06-05 by Arthur David Olson.
 */
package libcore.util;

import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.TimeZone;
import libcore.io.BufferIterator;

/**
 * Our concrete TimeZone implementation, backed by zoneinfo data.
 *
 * @hide - used to implement TimeZone
 */
public final class ZoneInfo extends TimeZone {
    private static final long MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;
    private static final long MILLISECONDS_PER_400_YEARS =
            MILLISECONDS_PER_DAY * (400 * 365 + 100 - 3);

    private static final long UNIX_OFFSET = 62167219200000L;

    private static final int[] NORMAL = new int[] {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,
    };

    private static final int[] LEAP = new int[] {
        0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335,
    };

    private int mRawOffset;
    private final int mEarliestRawOffset;
    private final boolean mUseDst;
    private final int mDstSavings; // Implements TimeZone.getDSTSavings.

    private final int[] mTransitions;
    private final int[] mOffsets;
    private final byte[] mTypes;
    private final byte[] mIsDsts;

    public static ZoneInfo makeTimeZone(String id, BufferIterator it) {
        // Variable names beginning tzh_ correspond to those in "tzfile.h".

        // Check tzh_magic.
        if (it.readInt() != 0x545a6966) { // "TZif"
            return null;
        }

        // Skip the uninteresting part of the header.
        it.skip(28);

        // Read the sizes of the arrays we're about to read.
        int tzh_timecnt = it.readInt();
        int tzh_typecnt = it.readInt();

        it.skip(4); // Skip tzh_charcnt.

        int[] transitions = new int[tzh_timecnt];
        it.readIntArray(transitions, 0, transitions.length);

        byte[] type = new byte[tzh_timecnt];
        it.readByteArray(type, 0, type.length);

        int[] gmtOffsets = new int[tzh_typecnt];
        byte[] isDsts = new byte[tzh_typecnt];
        for (int i = 0; i < tzh_typecnt; ++i) {
            gmtOffsets[i] = it.readInt();
            isDsts[i] = it.readByte();
            // We skip the abbreviation index. This would let us provide historically-accurate
            // time zone abbreviations (such as "AHST", "YST", and "AKST" for standard time in
            // America/Anchorage in 1982, 1983, and 1984 respectively). ICU only knows the current
            // names, though, so even if we did use this data to provide the correct abbreviations
            // for en_US, we wouldn't be able to provide correct abbreviations for other locales,
            // nor would we be able to provide correct long forms (such as "Yukon Standard Time")
            // for any locale. (The RI doesn't do any better than us here either.)
            it.skip(1);
        }

        return new ZoneInfo(id, transitions, type, gmtOffsets, isDsts);
    }

    private ZoneInfo(String name, int[] transitions, byte[] types, int[] gmtOffsets, byte[] isDsts) {
        mTransitions = transitions;
        mTypes = types;
        mIsDsts = isDsts;
        setID(name);

        // Find the latest daylight and standard offsets (if any).
        int lastStd = 0;
        boolean haveStd = false;
        int lastDst = 0;
        boolean haveDst = false;
        for (int i = mTransitions.length - 1; (!haveStd || !haveDst) && i >= 0; --i) {
            int type = mTypes[i] & 0xff;
            if (!haveStd && mIsDsts[type] == 0) {
                haveStd = true;
                lastStd = i;
            }
            if (!haveDst && mIsDsts[type] != 0) {
                haveDst = true;
                lastDst = i;
            }
        }

        // Use the latest non-daylight offset (if any) as the raw offset.
        if (lastStd >= mTypes.length) {
            mRawOffset = gmtOffsets[0];
        } else {
            mRawOffset = gmtOffsets[mTypes[lastStd] & 0xff];
        }

        // Use the latest transition's pair of offsets to compute the DST savings.
        // This isn't generally useful, but it's exposed by TimeZone.getDSTSavings.
        if (lastDst >= mTypes.length) {
            mDstSavings = 0;
        } else {
            mDstSavings = Math.abs(gmtOffsets[mTypes[lastStd] & 0xff] - gmtOffsets[mTypes[lastDst] & 0xff]) * 1000;
        }

        // Cache the oldest known raw offset, in case we're asked about times that predate our
        // transition data.
        int firstStd = -1;
        for (int i = 0; i < mTransitions.length; ++i) {
            if (mIsDsts[mTypes[i] & 0xff] == 0) {
                firstStd = i;
                break;
            }
        }
        int earliestRawOffset = (firstStd != -1) ? gmtOffsets[mTypes[firstStd] & 0xff] : mRawOffset;

        // Rather than keep offsets from UTC, we use offsets from local time, so the raw offset
        // can be changed and automatically affect all the offsets.
        mOffsets = gmtOffsets;
        for (int i = 0; i < mOffsets.length; i++) {
            mOffsets[i] -= mRawOffset;
        }

        // Is this zone observing DST currently or in the future?
        // We don't care if they've historically used it: most places have at least once.
        // See http://code.google.com/p/android/issues/detail?id=877.
        // This test means that for somewhere like Morocco, which tried DST in 2009 but has
        // no future plans (and thus no future schedule info) will report "true" from
        // useDaylightTime at the start of 2009 but "false" at the end. This seems appropriate.
        boolean usesDst = false;
        int currentUnixTimeSeconds = (int) (System.currentTimeMillis() / 1000);
        int i = mTransitions.length - 1;
        while (i >= 0 && mTransitions[i] >= currentUnixTimeSeconds) {
            if (mIsDsts[mTypes[i]] > 0) {
                usesDst = true;
                break;
            }
            i--;
        }
        mUseDst = usesDst;

        // tzdata uses seconds, but Java uses milliseconds.
        mRawOffset *= 1000;
        mEarliestRawOffset = earliestRawOffset * 1000;
    }

    @Override
    public int getOffset(int era, int year, int month, int day, int dayOfWeek, int millis) {
        // XXX This assumes Gregorian always; Calendar switches from
        // Julian to Gregorian in 1582.  What calendar system are the
        // arguments supposed to come from?

        long calc = (year / 400) * MILLISECONDS_PER_400_YEARS;
        year %= 400;

        calc += year * (365 * MILLISECONDS_PER_DAY);
        calc += ((year + 3) / 4) * MILLISECONDS_PER_DAY;

        if (year > 0) {
            calc -= ((year - 1) / 100) * MILLISECONDS_PER_DAY;
        }

        boolean isLeap = (year == 0 || (year % 4 == 0 && year % 100 != 0));
        int[] mlen = isLeap ? LEAP : NORMAL;

        calc += mlen[month] * MILLISECONDS_PER_DAY;
        calc += (day - 1) * MILLISECONDS_PER_DAY;
        calc += millis;

        calc -= mRawOffset;
        calc -= UNIX_OFFSET;

        return getOffset(calc);
    }

    @Override
    public int getOffset(long when) {
        int unix = (int) (when / 1000);
        int transition = Arrays.binarySearch(mTransitions, unix);
        if (transition < 0) {
            transition = ~transition - 1;
            if (transition < 0) {
                // Assume that all times before our first transition correspond to the
                // oldest-known non-daylight offset. The obvious alternative would be to
                // use the current raw offset, but that seems like a greater leap of faith.
                return mEarliestRawOffset;
            }
        }
        return mRawOffset + mOffsets[mTypes[transition] & 0xff] * 1000;
    }

    @Override public boolean inDaylightTime(Date time) {
        long when = time.getTime();
        int unix = (int) (when / 1000);
        int transition = Arrays.binarySearch(mTransitions, unix);
        if (transition < 0) {
            transition = ~transition - 1;
            if (transition < 0) {
                // Assume that all times before our first transition are non-daylight.
                // Transition data tends to start with a transition to daylight, so just
                // copying the first transition would assume the opposite.
                // http://code.google.com/p/android/issues/detail?id=14395
                return false;
            }
        }
        return mIsDsts[mTypes[transition] & 0xff] == 1;
    }

    @Override public int getRawOffset() {
        return mRawOffset;
    }

    @Override public void setRawOffset(int off) {
        mRawOffset = off;
    }

    @Override public int getDSTSavings() {
        return mUseDst ? mDstSavings: 0;
    }

    @Override public boolean useDaylightTime() {
        return mUseDst;
    }

    @Override public boolean hasSameRules(TimeZone timeZone) {
        if (!(timeZone instanceof ZoneInfo)) {
            return false;
        }
        ZoneInfo other = (ZoneInfo) timeZone;
        if (mUseDst != other.mUseDst) {
            return false;
        }
        if (!mUseDst) {
            return mRawOffset == other.mRawOffset;
        }
        return mRawOffset == other.mRawOffset
                // Arrays.equals returns true if both arrays are null
                && Arrays.equals(mOffsets, other.mOffsets)
                && Arrays.equals(mIsDsts, other.mIsDsts)
                && Arrays.equals(mTypes, other.mTypes)
                && Arrays.equals(mTransitions, other.mTransitions);
    }

    @Override public boolean equals(Object obj) {
        if (!(obj instanceof ZoneInfo)) {
            return false;
        }
        ZoneInfo other = (ZoneInfo) obj;
        return getID().equals(other.getID()) && hasSameRules(other);
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + getID().hashCode();
        result = prime * result + Arrays.hashCode(mOffsets);
        result = prime * result + Arrays.hashCode(mIsDsts);
        result = prime * result + mRawOffset;
        result = prime * result + Arrays.hashCode(mTransitions);
        result = prime * result + Arrays.hashCode(mTypes);
        result = prime * result + (mUseDst ? 1231 : 1237);
        return result;
    }

    @Override
    public String toString() {
        return getClass().getName() + "[id=\"" + getID() + "\"" +
            ",mRawOffset=" + mRawOffset +
            ",mEarliestRawOffset=" + mEarliestRawOffset +
            ",mUseDst=" + mUseDst +
            ",mDstSavings=" + mDstSavings +
            ",transitions=" + mTransitions.length +
            "]";
    }

    @Override
    public Object clone() {
        // Overridden for documentation. The default clone() behavior is exactly what we want.
        // Though mutable, the arrays of offset data are treated as immutable. Only ID and
        // mRawOffset are mutable in this class, and those are an immutable object and a primitive
        // respectively.
        return super.clone();
    }

    /**
     * A class that represents a "wall time". This class is modeled on the C tm struct and
     * is used to support android.text.format.Time behavior. Unlike the tm struct the year is
     * represented as the full year, not the years since 1900.
     *
     * <p>This class contains a rewrite of various native functions that android.text.format.Time
     * once relied on such as mktime_tz and localtime_tz. This replacement does not support leap
     * seconds but does try to preserve behavior around ambiguous date/times found in the BSD
     * version of mktime that was previously used.
     *
     * <p>The original native code used a 32-bit value for time_t on 32-bit Android, which
     * was the only variant of Android available at the time. To preserve old behavior this code
     * deliberately uses {@code int} rather than {@code long} for most things and performs
     * calculations in seconds. This creates deliberate truncation issues for date / times before
     * 1901 and after 2038. This is intentional but might be fixed in future if all the knock-ons
     * can be resolved: Application code may have come to rely on the range so previously values
     * like zero for year could indicate an invalid date but if we move to long the year zero would
     * be valid.
     *
     * <p>All offsets are considered to be safe for addition / subtraction / multiplication without
     * worrying about overflow. All absolute time arithmetic is checked for overflow / underflow.
     */
    public static class WallTime {

        // We use a GregorianCalendar (set to UTC) to handle all the date/time normalization logic
        // and to convert from a broken-down date/time to a millis value.
        // Unfortunately, it cannot represent an initial state with a zero day and would
        // automatically normalize it, so we must copy values into and out of it as needed.
        private final GregorianCalendar calendar;

        private int year;
        private int month;
        private int monthDay;
        private int hour;
        private int minute;
        private int second;
        private int weekDay;
        private int yearDay;
        private int isDst;
        private int gmtOffsetSeconds;

        public WallTime() {
            this.calendar = createGregorianCalendar();
            calendar.setTimeZone(TimeZone.getTimeZone("UTC"));
        }

        // LayoutLib replaces this method via bytecode manipulation, since the
        // minimum-cost constructor is not available on host machines.
        private static GregorianCalendar createGregorianCalendar() {
            return new GregorianCalendar(false);
        }

        /**
         * Sets the wall time to a point in time using the time zone information provided. This
         * is a replacement for the old native localtime_tz() function.
         *
         * <p>When going from an instant to a wall time it is always unambiguous because there
         * is only one offset rule acting at any given instant. We do not consider leap seconds.
         */
        public void localtime(int timeSeconds, ZoneInfo zoneInfo) {
            try {
                int offsetSeconds = zoneInfo.mRawOffset / 1000;

                // Find out the timezone DST state and adjustment.
                byte isDst;
                if (zoneInfo.mTransitions.length == 0) {
                    isDst = 0;
                } else {
                    // transitionIndex can be in the range -1..zoneInfo.mTransitions.length - 1
                    int transitionIndex = findTransitionIndex(zoneInfo, timeSeconds);
                    if (transitionIndex < 0) {
                        // -1 means timeSeconds is "before the first recorded transition". The first
                        // recorded transition is treated as a transition from non-DST and the raw
                        // offset.
                        isDst = 0;
                    } else {
                        byte transitionType = zoneInfo.mTypes[transitionIndex];
                        offsetSeconds += zoneInfo.mOffsets[transitionType];
                        isDst = zoneInfo.mIsDsts[transitionType];
                    }
                }

                // Perform arithmetic that might underflow before setting fields.
                int wallTimeSeconds = checkedAdd(timeSeconds, offsetSeconds);

                // Set fields.
                calendar.setTimeInMillis(wallTimeSeconds * 1000L);
                copyFieldsFromCalendar();
                this.isDst = isDst;
                this.gmtOffsetSeconds = offsetSeconds;
            } catch (CheckedArithmeticException e) {
                // Just stop, leaving fields untouched.
            }
        }

        /**
         * Returns the time in seconds since beginning of the Unix epoch for the wall time using the
         * time zone information provided. This is a replacement for an old native mktime_tz() C
         * function.
         *
         * <p>When going from a wall time to an instant the answer can be ambiguous. A wall
         * time can map to zero, one or two instants given sane date/time transitions. Sane
         * in this case means that transitions occur less frequently than the offset
         * differences between them (which could cause all sorts of craziness like the
         * skipping out of transitions).
         *
         * <p>For example, this is not fully supported:
         * <ul>
         *     <li>t1 { time = 1, offset = 0 }
         *     <li>t2 { time = 2, offset = -1 }
         *     <li>t3 { time = 3, offset = -2 }
         * </ul>
         * A wall time in this case might map to t1, t2 or t3.
         *
         * <p>We do not handle leap seconds.
         * <p>We assume that no timezone offset transition has an absolute offset > 24 hours.
         * <p>We do not assume that adjacent transitions modify the DST state; adjustments can
         * occur for other reasons such as when a zone changes its raw offset.
         */
        public int mktime(ZoneInfo zoneInfo) {
            // Normalize isDst to -1, 0 or 1 to simplify isDst equality checks below.
            this.isDst = this.isDst > 0 ? this.isDst = 1 : this.isDst < 0 ? this.isDst = -1 : 0;

            copyFieldsToCalendar();
            final long longWallTimeSeconds = calendar.getTimeInMillis()  / 1000;
            if (Integer.MIN_VALUE > longWallTimeSeconds
                    || longWallTimeSeconds > Integer.MAX_VALUE) {
                // For compatibility with the old native 32-bit implementation we must treat
                // this as an error. Note: -1 could be confused with a real time.
                return -1;
            }

            try {
                final int wallTimeSeconds =  (int) longWallTimeSeconds;
                final int rawOffsetSeconds = zoneInfo.mRawOffset / 1000;
                final int rawTimeSeconds = checkedSubtract(wallTimeSeconds, rawOffsetSeconds);

                if (zoneInfo.mTransitions.length == 0) {
                    // There is no transition information. There is just a raw offset for all time.
                    if (this.isDst > 0) {
                        // Caller has asserted DST, but there is no DST information available.
                        return -1;
                    }
                    copyFieldsFromCalendar();
                    this.isDst = 0;
                    this.gmtOffsetSeconds = rawOffsetSeconds;
                    return rawTimeSeconds;
                }

                // We cannot know for sure what instant the wall time will map to. Unfortunately, in
                // order to know for sure we need the timezone information, but to get the timezone
                // information we need an instant. To resolve this we use the raw offset to find an
                // OffsetInterval; this will get us the OffsetInterval we need or very close.

                // The initialTransition can be between -1 and (zoneInfo.mTransitions - 1). -1
                // indicates the rawTime is before the first transition and is handled gracefully by
                // createOffsetInterval().
                final int initialTransitionIndex = findTransitionIndex(zoneInfo, rawTimeSeconds);

                if (isDst < 0) {
                    // This is treated as a special case to get it out of the way:
                    // When a caller has set isDst == -1 it means we can return the first match for
                    // the wall time we find. If the caller has specified a wall time that cannot
                    // exist this always returns -1.

                    Integer result = doWallTimeSearch(zoneInfo, initialTransitionIndex,
                            wallTimeSeconds, true /* mustMatchDst */);
                    return result == null ? -1 : result;
                }

                // If the wall time asserts a DST (isDst == 0 or 1) the search is performed twice:
                // 1) The first attempts to find a DST offset that matches isDst exactly.
                // 2) If it fails, isDst is assumed to be incorrect and adjustments are made to see
                // if a valid wall time can be created. The result can be somewhat arbitrary.

                Integer result = doWallTimeSearch(zoneInfo, initialTransitionIndex, wallTimeSeconds,
                        true /* mustMatchDst */);
                if (result == null) {
                    result = doWallTimeSearch(zoneInfo, initialTransitionIndex, wallTimeSeconds,
                            false /* mustMatchDst */);
                }
                if (result == null) {
                    result = -1;
                }
                return result;
            } catch (CheckedArithmeticException e) {
                return -1;
            }
        }

        /**
         * Attempt to apply DST adjustments to {@code oldWallTimeSeconds} to create a wall time in
         * {@code targetInterval}.
         *
         * <p>This is used when a caller has made an assertion about standard time / DST that cannot
         * be matched to any offset interval that exists. We must therefore assume that the isDst
         * assertion is incorrect and the invalid wall time is the result of some modification the
         * caller made to a valid wall time that pushed them outside of the offset interval they
         * were in. We must correct for any DST change that should have been applied when they did
         * so.
         *
         * <p>Unfortunately, we have no information about what adjustment they made and so cannot
         * know which offset interval they were previously in. For example, they may have added a
         * second or a year to a valid time to arrive at what they have.
         *
         * <p>We try all offset types that are not the same as the isDst the caller asserted. For
         * each possible offset we work out the offset difference between that and
         * {@code targetInterval}, apply it, and see if we are still in {@code targetInterval}. If
         * we are, then we have found an adjustment.
         */
        private Integer tryOffsetAdjustments(ZoneInfo zoneInfo, int oldWallTimeSeconds,
                OffsetInterval targetInterval, int transitionIndex, int isDstToFind)
                throws CheckedArithmeticException {

            int[] offsetsToTry = getOffsetsOfType(zoneInfo, transitionIndex, isDstToFind);
            for (int j = 0; j < offsetsToTry.length; j++) {
                int rawOffsetSeconds = zoneInfo.mRawOffset / 1000;
                int jOffsetSeconds = rawOffsetSeconds + offsetsToTry[j];
                int targetIntervalOffsetSeconds = targetInterval.getTotalOffsetSeconds();
                int adjustmentSeconds = targetIntervalOffsetSeconds - jOffsetSeconds;
                int adjustedWallTimeSeconds = checkedAdd(oldWallTimeSeconds, adjustmentSeconds);
                if (targetInterval.containsWallTime(adjustedWallTimeSeconds)) {
                    // Perform any arithmetic that might overflow.
                    int returnValue = checkedSubtract(adjustedWallTimeSeconds,
                            targetIntervalOffsetSeconds);

                    // Modify field state and return the result.
                    calendar.setTimeInMillis(adjustedWallTimeSeconds * 1000L);
                    copyFieldsFromCalendar();
                    this.isDst = targetInterval.getIsDst();
                    this.gmtOffsetSeconds = targetIntervalOffsetSeconds;
                    return returnValue;
                }
            }
            return null;
        }

        /**
         * Return an array of offsets that have the requested {@code isDst} value.
         * The {@code startIndex} is used as a starting point so transitions nearest
         * to that index are returned first.
         */
        private static int[] getOffsetsOfType(ZoneInfo zoneInfo, int startIndex, int isDst) {
            // +1 to account for the synthetic transition we invent before the first recorded one.
            int[] offsets = new int[zoneInfo.mOffsets.length + 1];
            boolean[] seen = new boolean[zoneInfo.mOffsets.length];
            int numFound = 0;

            int delta = 0;
            boolean clampTop = false;
            boolean clampBottom = false;
            do {
                // delta = { 1, -1, 2, -2, 3, -3...}
                delta *= -1;
                if (delta >= 0) {
                    delta++;
                }

                int transitionIndex = startIndex + delta;
                if (delta < 0 && transitionIndex < -1) {
                    clampBottom = true;
                    continue;
                } else if (delta > 0 && transitionIndex >=  zoneInfo.mTypes.length) {
                    clampTop = true;
                    continue;
                }

                if (transitionIndex == -1) {
                    if (isDst == 0) {
                        // Synthesize a non-DST transition before the first transition we have
                        // data for.
                        offsets[numFound++] = 0; // offset of 0 from raw offset
                    }
                    continue;
                }
                byte type = zoneInfo.mTypes[transitionIndex];
                if (!seen[type]) {
                    if (zoneInfo.mIsDsts[type] == isDst) {
                        offsets[numFound++] = zoneInfo.mOffsets[type];
                    }
                    seen[type] = true;
                }
            } while (!(clampTop && clampBottom));

            int[] toReturn = new int[numFound];
            System.arraycopy(offsets, 0, toReturn, 0, numFound);
            return toReturn;
        }

        /**
         * Find a time <em>in seconds</em> the same or close to {@code wallTimeSeconds} that
         * satisfies {@code mustMatchDst}. The search begins around the timezone offset transition
         * with {@code initialTransitionIndex}.
         *
         * <p>If {@code mustMatchDst} is {@code true} the method can only return times that
         * use timezone offsets that satisfy the {@code this.isDst} requirements.
         * If {@code this.isDst == -1} it means that any offset can be used.
         *
         * <p>If {@code mustMatchDst} is {@code false} any offset that covers the
         * currently set time is acceptable. That is: if {@code this.isDst} == -1, any offset
         * transition can be used, if it is 0 or 1 the offset used must match {@code this.isDst}.
         *
         * <p>Note: This method both uses and can modify field state. It returns the matching time
         * in seconds if a match has been found and modifies fields, or it returns {@code null} and
         * leaves the field state unmodified.
         */
        private Integer doWallTimeSearch(ZoneInfo zoneInfo, int initialTransitionIndex,
                int wallTimeSeconds, boolean mustMatchDst) throws CheckedArithmeticException {

            // The loop below starts at the initialTransitionIndex and radiates out from that point
            // up to 24 hours in either direction by applying transitionIndexDelta to inspect
            // adjacent transitions (0, -1, +1, -2, +2). 24 hours is used because we assume that no
            // total offset from UTC is ever > 24 hours. clampTop and clampBottom are used to
            // indicate whether the search has either searched > 24 hours or exhausted the
            // transition data in that direction. The search stops when a match is found or if
            // clampTop and clampBottom are both true.
            // The match logic employed is determined by the mustMatchDst parameter.
            final int MAX_SEARCH_SECONDS = 24 * 60 * 60;
            boolean clampTop = false, clampBottom = false;
            int loop = 0;
            do {
                // transitionIndexDelta = { 0, -1, 1, -2, 2,..}
                int transitionIndexDelta = (loop + 1) / 2;
                if (loop % 2 == 1) {
                    transitionIndexDelta *= -1;
                }
                loop++;

                // Only do any work in this iteration if we need to.
                if (transitionIndexDelta > 0 && clampTop
                        || transitionIndexDelta < 0 && clampBottom) {
                    continue;
                }

                // Obtain the OffsetInterval to use.
                int currentTransitionIndex = initialTransitionIndex + transitionIndexDelta;
                OffsetInterval offsetInterval =
                        OffsetInterval.create(zoneInfo, currentTransitionIndex);
                if (offsetInterval == null) {
                    // No transition exists with the index we tried: Stop searching in the
                    // current direction.
                    clampTop |= (transitionIndexDelta > 0);
                    clampBottom |= (transitionIndexDelta < 0);
                    continue;
                }

                // Match the wallTimeSeconds against the OffsetInterval.
                if (mustMatchDst) {
                    // Work out if the interval contains the wall time the caller specified and
                    // matches their isDst value.
                    if (offsetInterval.containsWallTime(wallTimeSeconds)) {
                        if (this.isDst == -1 || offsetInterval.getIsDst() == this.isDst) {
                            // This always returns the first OffsetInterval it finds that matches
                            // the wall time and isDst requirements. If this.isDst == -1 this means
                            // the result might be a DST or a non-DST answer for wall times that can
                            // exist in two OffsetIntervals.
                            int totalOffsetSeconds = offsetInterval.getTotalOffsetSeconds();
                            int returnValue = checkedSubtract(wallTimeSeconds,
                                    totalOffsetSeconds);

                            copyFieldsFromCalendar();
                            this.isDst = offsetInterval.getIsDst();
                            this.gmtOffsetSeconds = totalOffsetSeconds;
                            return returnValue;
                        }
                    }
                } else {
                    // To retain similar behavior to the old native implementation: if the caller is
                    // asserting the same isDst value as the OffsetInterval we are looking at we do
                    // not try to find an adjustment from another OffsetInterval of the same isDst
                    // type. If you remove this you get different results in situations like a
                    // DST -> DST transition or STD -> STD transition that results in an interval of
                    // "skipped" wall time. For example: if 01:30 (DST) is invalid and between two
                    // DST intervals, and the caller has passed isDst == 1, this results in a -1
                    // being returned.
                    if (isDst != offsetInterval.getIsDst()) {
                        final int isDstToFind = isDst;
                        Integer returnValue = tryOffsetAdjustments(zoneInfo, wallTimeSeconds,
                                offsetInterval, currentTransitionIndex, isDstToFind);
                        if (returnValue != null) {
                            return returnValue;
                        }
                    }
                }

                // See if we can avoid another loop in the current direction.
                if (transitionIndexDelta > 0) {
                    // If we are searching forward and the OffsetInterval we have ends
                    // > MAX_SEARCH_SECONDS after the wall time, we don't need to look any further
                    // forward.
                    boolean endSearch = offsetInterval.getEndWallTimeSeconds() - wallTimeSeconds
                            > MAX_SEARCH_SECONDS;
                    if (endSearch) {
                        clampTop = true;
                    }
                } else if (transitionIndexDelta < 0) {
                    boolean endSearch = wallTimeSeconds - offsetInterval.getStartWallTimeSeconds()
                            >= MAX_SEARCH_SECONDS;
                    if (endSearch) {
                        // If we are searching backward and the OffsetInterval starts
                        // > MAX_SEARCH_SECONDS before the wall time, we don't need to look any
                        // further backwards.
                        clampBottom = true;
                    }
                }
            } while (!(clampTop && clampBottom));
            return null;
        }

        public void setYear(int year) {
            this.year = year;
        }

        public void setMonth(int month) {
            this.month = month;
        }

        public void setMonthDay(int monthDay) {
            this.monthDay = monthDay;
        }

        public void setHour(int hour) {
            this.hour = hour;
        }

        public void setMinute(int minute) {
            this.minute = minute;
        }

        public void setSecond(int second) {
            this.second = second;
        }

        public void setWeekDay(int weekDay) {
            this.weekDay = weekDay;
        }

        public void setYearDay(int yearDay) {
            this.yearDay = yearDay;
        }

        public void setIsDst(int isDst) {
            this.isDst = isDst;
        }

        public void setGmtOffset(int gmtoff) {
            this.gmtOffsetSeconds = gmtoff;
        }

        public int getYear() {
            return year;
        }

        public int getMonth() {
            return month;
        }

        public int getMonthDay() {
            return monthDay;
        }

        public int getHour() {
            return hour;
        }

        public int getMinute() {
            return minute;
        }

        public int getSecond() {
            return second;
        }

        public int getWeekDay() {
            return weekDay;
        }

        public int getYearDay() {
            return yearDay;
        }

        public int getGmtOffset() {
            return gmtOffsetSeconds;
        }

        public int getIsDst() {
            return isDst;
        }

        private void copyFieldsToCalendar() {
            calendar.set(Calendar.YEAR, year);
            calendar.set(Calendar.MONTH, month);
            calendar.set(Calendar.DAY_OF_MONTH, monthDay);
            calendar.set(Calendar.HOUR_OF_DAY, hour);
            calendar.set(Calendar.MINUTE, minute);
            calendar.set(Calendar.SECOND, second);
        }

        private void copyFieldsFromCalendar() {
            year = calendar.get(Calendar.YEAR);
            month = calendar.get(Calendar.MONTH);
            monthDay = calendar.get(Calendar.DAY_OF_MONTH);
            hour = calendar.get(Calendar.HOUR_OF_DAY);
            minute = calendar.get(Calendar.MINUTE);
            second =  calendar.get(Calendar.SECOND);

            // Calendar uses Sunday == 1. Android Time uses Sunday = 0.
            weekDay = calendar.get(Calendar.DAY_OF_WEEK) - 1;
            // Calendar enumerates from 1, Android Time enumerates from 0.
            yearDay = calendar.get(Calendar.DAY_OF_YEAR) - 1;
        }

        /**
         * Find the transition in the {@code timezone} in effect at {@code timeSeconds}.
         *
         * <p>Returns an index in the range -1..timeZone.mTransitions.length - 1. -1 is used to
         * indicate the time is before the first transition. Other values are an index into
         * timeZone.mTransitions.
         */
        private static int findTransitionIndex(ZoneInfo timeZone, int timeSeconds) {
            int matchingRawTransition = Arrays.binarySearch(timeZone.mTransitions, timeSeconds);
            if (matchingRawTransition < 0) {
                matchingRawTransition = ~matchingRawTransition - 1;
            }
            return matchingRawTransition;
        }
    }

    /**
     * A wall-time representation of a timezone offset interval.
     *
     * <p>Wall-time means "as it would appear locally in the timezone in which it applies".
     * For example in 2007:
     * PST was a -8:00 offset that ran until Mar 11, 2:00 AM.
     * PDT was a -7:00 offset and ran from Mar 11, 3:00 AM to Nov 4, 2:00 AM.
     * PST was a -8:00 offset and ran from Nov 4, 1:00 AM.
     * Crucially this means that there was a "gap" after PST when PDT started, and an overlap when
     * PDT ended and PST began.
     *
     * <p>For convenience all wall-time values are represented as the number of seconds since the
     * beginning of the Unix epoch <em>in UTC</em>. To convert from a wall-time to the actual time
     * in the offset it is necessary to <em>subtract</em> the {@code totalOffsetSeconds}.
     * For example: If the offset in PST is -07:00 hours, then:
     * timeInPstSeconds = wallTimeUtcSeconds - offsetSeconds
     * i.e. 13:00 UTC - (-07:00) = 20:00 UTC = 13:00 PST
     */
    static class OffsetInterval {

        private final int startWallTimeSeconds;
        private final int endWallTimeSeconds;
        private final int isDst;
        private final int totalOffsetSeconds;

        /**
         * Creates an {@link OffsetInterval}.
         *
         * <p>If {@code transitionIndex} is -1, the transition is synthesized to be a non-DST offset
         * that runs from the beginning of time until the first transition in {@code timeZone} and
         * has an offset of {@code timezone.mRawOffset}. If {@code transitionIndex} is the last
         * transition that transition is considered to run until the end of representable time.
         * Otherwise, the information is extracted from {@code timeZone.mTransitions},
         * {@code timeZone.mOffsets} an {@code timeZone.mIsDsts}.
         */
        public static OffsetInterval create(ZoneInfo timeZone, int transitionIndex)
                throws CheckedArithmeticException {

            if (transitionIndex < -1 || transitionIndex >= timeZone.mTransitions.length) {
                return null;
            }

            int rawOffsetSeconds = timeZone.mRawOffset / 1000;
            if (transitionIndex == -1) {
                int endWallTimeSeconds = checkedAdd(timeZone.mTransitions[0], rawOffsetSeconds);
                return new OffsetInterval(Integer.MIN_VALUE, endWallTimeSeconds, 0 /* isDst */,
                        rawOffsetSeconds);
            }

            byte type = timeZone.mTypes[transitionIndex];
            int totalOffsetSeconds = timeZone.mOffsets[type] + rawOffsetSeconds;
            int endWallTimeSeconds;
            if (transitionIndex == timeZone.mTransitions.length - 1) {
                // If this is the last transition, make up the end time.
                endWallTimeSeconds = Integer.MAX_VALUE;
            } else {
                endWallTimeSeconds = checkedAdd(timeZone.mTransitions[transitionIndex + 1],
                        totalOffsetSeconds);
            }
            int isDst = timeZone.mIsDsts[type];
            int startWallTimeSeconds =
                    checkedAdd(timeZone.mTransitions[transitionIndex], totalOffsetSeconds);
            return new OffsetInterval(
                    startWallTimeSeconds, endWallTimeSeconds, isDst, totalOffsetSeconds);
        }

        private OffsetInterval(int startWallTimeSeconds, int endWallTimeSeconds, int isDst,
                int totalOffsetSeconds) {
            this.startWallTimeSeconds = startWallTimeSeconds;
            this.endWallTimeSeconds = endWallTimeSeconds;
            this.isDst = isDst;
            this.totalOffsetSeconds = totalOffsetSeconds;
        }

        public boolean containsWallTime(long wallTimeSeconds) {
            return wallTimeSeconds >= startWallTimeSeconds && wallTimeSeconds < endWallTimeSeconds;
        }

        public int getIsDst() {
            return isDst;
        }

        public int getTotalOffsetSeconds() {
            return totalOffsetSeconds;
        }

        public long getEndWallTimeSeconds() {
            return endWallTimeSeconds;
        }

        public long getStartWallTimeSeconds() {
            return startWallTimeSeconds;
        }
    }

    /**
     * An exception used to indicate an arithmetic overflow or underflow.
     */
    private static class CheckedArithmeticException extends Exception {
    }

    /**
     * Calculate (a + b).
     *
     * @throws CheckedArithmeticException if overflow or underflow occurs
     */
    private static int checkedAdd(int a, int b) throws CheckedArithmeticException {
        // Adapted from Guava IntMath.checkedAdd();
        long result = (long) a + b;
        if (result != (int) result) {
            throw new CheckedArithmeticException();
        }
        return (int) result;
    }

    /**
     * Calculate (a - b).
     *
     * @throws CheckedArithmeticException if overflow or underflow occurs
     */
    private static int checkedSubtract(int a, int b) throws CheckedArithmeticException {
        // Adapted from Guava IntMath.checkedSubtract();
        long result = (long) a - b;
        if (result != (int) result) {
            throw new CheckedArithmeticException();
        }
        return (int) result;
    }
}
