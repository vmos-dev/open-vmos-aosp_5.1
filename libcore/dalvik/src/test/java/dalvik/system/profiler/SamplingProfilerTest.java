/*
 * Copyright (C) 2010 The Android Open Source Project
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

package dalvik.system.profiler;

import dalvik.system.profiler.AsciiHprofWriter;
import dalvik.system.profiler.BinaryHprofReader;
import dalvik.system.profiler.BinaryHprofWriter;
import dalvik.system.profiler.HprofData.Sample;
import dalvik.system.profiler.HprofData.StackTrace;
import dalvik.system.profiler.HprofData.ThreadEvent;
import dalvik.system.profiler.HprofData;
import dalvik.system.profiler.SamplingProfiler.ThreadSet;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.math.BigInteger;
import java.security.KeyPairGenerator;
import java.security.SecureRandom;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import javax.crypto.spec.DHParameterSpec;
import junit.framework.TestCase;

public class SamplingProfilerTest extends TestCase {

    /**
     * Run the SamplingProfiler to gather some data on an actual
     * computation, then assert that it looks correct with test_HprofData.
     */
    public void test_SamplingProfiler() throws Exception {
        ThreadSet threadSet = SamplingProfiler.newArrayThreadSet(Thread.currentThread());
        SamplingProfiler profiler = new SamplingProfiler(12, threadSet);
        profiler.start(100);
        toBeMeasured();
        profiler.stop();
        profiler.shutdown();
        test_HprofData(profiler.getHprofData(), true);
    }

    private static final String P_STR =
            "9494fec095f3b85ee286542b3836fc81a5dd0a0349b4c239dd38744d488cf8e3"
            + "1db8bcb7d33b41abb9e5a33cca9144b1cef332c94bf0573bf047a3aca98cdf3b";
    private static final String G_STR =
            "98ab7c5c431479d8645e33aa09758e0907c78747798d0968576f9877421a9089"
            + "756f7876e76590b76765645c987976d764dd4564698a87585e64554984bb4445"
            + "76e5764786f875b4456c";

    private static final byte[] P = new BigInteger(P_STR,16).toByteArray();
    private static final byte[] G = new BigInteger(G_STR,16).toByteArray();

    private static void toBeMeasured () throws Exception {
        long start = System.currentTimeMillis();
        for (int i = 0; i < 10000; i++) {
            BigInteger p = new BigInteger(P);
            BigInteger g = new BigInteger(G);
            KeyPairGenerator gen = KeyPairGenerator.getInstance("DH");
            gen.initialize(new DHParameterSpec(p, g), new SecureRandom());
        }
        long end = System.currentTimeMillis();
    }

    public void test_HprofData_null() throws Exception {
        try {
            new HprofData(null);
            fail();
        } catch (NullPointerException expected) {
        }
    }

    public void test_HprofData_empty() throws Exception {
        Map<StackTrace, int[]> stackTraces = new HashMap<StackTrace, int[]>();
        HprofData hprofData = new HprofData(stackTraces);
        test_HprofData(hprofData, true);
    }

    public void test_HprofData_timeMillis() throws Exception {
        Map<StackTrace, int[]> stackTraces = new HashMap<StackTrace, int[]>();
        HprofData hprofData = new HprofData(stackTraces);
        long now = System.currentTimeMillis();
        hprofData.setStartMillis(now);
        assertEquals(now, hprofData.getStartMillis());
        test_HprofData(hprofData, true);
    }

    public void test_HprofData_addThreadEvent_null() throws Exception {
        Map<StackTrace, int[]> stackTraces = new HashMap<StackTrace, int[]>();
        HprofData hprofData = new HprofData(stackTraces);
        try {
            hprofData.addThreadEvent(null);
            fail();
        } catch (NullPointerException expected) {
        }
        test_HprofData(hprofData, true);
    }

    public void test_HprofData_addThreadEvent() throws Exception {
        Map<StackTrace, int[]> stackTraces = new HashMap<StackTrace, int[]>();
        HprofData hprofData = new HprofData(stackTraces);

        // should have nothing in the thread history to start
        assertEquals(0, hprofData.getThreadHistory().size());

        // add thread 1
        final int threadId = 1;
        final int objectId = 2;
        ThreadEvent start1 = ThreadEvent.start(objectId, threadId,
                                               "thread-name", "thread-group", "parent-group");
        hprofData.addThreadEvent(start1);
        assertEquals(Arrays.asList(start1), hprofData.getThreadHistory());
        test_HprofData(hprofData, true);

        // remove thread 2, which should not exist (but that's okay on the RI)
        ThreadEvent end2 = ThreadEvent.end(threadId+1);
        hprofData.addThreadEvent(end2);
        assertEquals(Arrays.asList(start1, end2), hprofData.getThreadHistory());
        test_HprofData(hprofData, false); // non-strict from here down because of this RI data

        // remove thread 1, which should exist
        ThreadEvent end1 = ThreadEvent.end(threadId);
        hprofData.addThreadEvent(end1);
        assertEquals(Arrays.asList(start1, end2, end1), hprofData.getThreadHistory());
        test_HprofData(hprofData, false);

        // remove thread 1 again, which should not exist (its not okay to have end followed by end)
        try {
            hprofData.addThreadEvent(ThreadEvent.end(threadId));
            fail();
        } catch (IllegalArgumentException expected) {
        }
        assertEquals(Arrays.asList(start1, end2, end1), hprofData.getThreadHistory());
        test_HprofData(hprofData, false);
    }

    public void test_HprofData_addStackTrace() throws Exception {
        Map<StackTrace, int[]> stackTraces = new HashMap<StackTrace, int[]>();
        HprofData hprofData = new HprofData(stackTraces);

        // should have no samples to start
        assertEquals(0, hprofData.getSamples().size());

        // attempt to add a stack for a non-existent thread, should fail
        final int stackTraceId = 1;
        final int threadId = 2;
        final int objectId = 3;
        final int sampleCount = 4;
        StackTraceElement[] stackFrames = new Throwable().getStackTrace();
        final int[] countCell = new int[] { 4 };
        StackTrace stackTrace = new StackTrace(stackTraceId, threadId, stackFrames);
        try {
            hprofData.addStackTrace(stackTrace, countCell);
            fail();
        } catch (IllegalArgumentException expected) {
        }

        // add the thread and add the event
        ThreadEvent start = ThreadEvent.start(objectId, threadId,
                                              "thread-name", "thread-group", "parent-group");
        hprofData.addThreadEvent(start);
        hprofData.addStackTrace(stackTrace, countCell);
        Set<Sample> samples = hprofData.getSamples();
        assertNotNull(samples);
        assertNotSame(samples, hprofData.getSamples());
        assertEquals(1, samples.size());
        Sample sample = samples.iterator().next();
        assertNotNull(sample);
        assertEquals(stackTrace, sample.stackTrace);
        assertEquals(sampleCount, sample.count);
        test_HprofData(hprofData, true);

        // confirm we can mutate the sample count, but that its not
        // visible in the current sample, but it will be visible in a
        // new one.
        countCell[0] += 42;
        assertEquals(sampleCount, sample.count);
        Sample sample2 = hprofData.getSamples().iterator().next();
        assertEquals(sampleCount + 42, sample2.count);
        test_HprofData(hprofData, true);

        // try to reuse the stackTraceId, should fail
        try {
            hprofData.addStackTrace(stackTrace, countCell);
            fail();
        } catch (IllegalArgumentException expected) {
        }
        assertEquals(1, hprofData.getSamples().size());
        test_HprofData(hprofData, true);

    }

    private void test_HprofData(HprofData hprofData, boolean strict) throws Exception {
        assertHprofData(hprofData, strict);
        test_HprofData_ascii(hprofData);
        test_HprofData_binary(hprofData, strict);
    }

    /**
     * Assert general properities of HprofData hold true.
     */
    private void assertHprofData(HprofData hprofData, boolean strict) throws Exception {
        List<ThreadEvent> threadHistory = hprofData.getThreadHistory();
        assertNotNull(threadHistory);
        Set<Integer> threadsSeen = new HashSet<Integer>();
        Set<Integer> threadsActive = new HashSet<Integer>();
        for (ThreadEvent event : threadHistory) {
            assertNotNull(event);
            assertNotNull(event.type);
            switch (event.type) {
                case START:
                    assertNotNull(event.threadName);
                    assertTrue(threadsActive.add(event.threadId));
                    assertTrue(threadsSeen.add(event.threadId));
                    break;
                case END:
                    assertEquals(-1, event.objectId);
                    assertNull(event.threadName);
                    assertNull(event.groupName);
                    assertNull(event.parentGroupName);
                    if (strict) {
                        assertTrue(threadsActive.remove(event.threadId));
                    }
                    break;
            }
        }

        Set<Sample> samples = hprofData.getSamples();
        assertNotNull(samples);
        for (Sample sample : samples) {
            assertNotNull(sample);
            assertTrue(sample.count > 0);
            assertNotNull(sample.stackTrace);
            assertTrue(sample.stackTrace.stackTraceId != -1);
            assertTrue(threadsSeen.contains(sample.stackTrace.getThreadId()));
            assertNotNull(sample.stackTrace.getStackFrames());
        }
    }

    /**
     * Convert to HprofData to ASCII to see if it triggers any exceptions
     */
    private void test_HprofData_ascii(HprofData hprofData) throws Exception {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        AsciiHprofWriter.write(hprofData, out);
        assertFalse(out.toByteArray().length == 0);
    }

    /**
     * Convert to HprofData to binary and then reparse as to
     * HprofData. Make sure the accessible data is equivalent.
     */
    private void test_HprofData_binary(HprofData hprofData, boolean strict) throws Exception {

        ByteArrayOutputStream out = new ByteArrayOutputStream();
        BinaryHprofWriter.write(hprofData, out);
        out.close();

        byte[] bytes = out.toByteArray();
        assertFalse(bytes.length == 0);
        if (false) {
            File file = new File("/sdcard/java.hprof");
            OutputStream debug = new FileOutputStream(file);
            debug.write(bytes);
            debug.close();
            System.out.println("Wrote binary hprof data to " + file);
        }

        InputStream in = new ByteArrayInputStream(bytes);
        BinaryHprofReader reader = new BinaryHprofReader(in);
        assertTrue(reader.getStrict());
        reader.read();
        in.close();
        assertEquals("JAVA PROFILE 1.0.2", reader.getVersion());
        assertNotNull(reader.getHprofData());

        HprofData parsed = reader.getHprofData();
        assertHprofData(hprofData, strict);

        assertEquals(Long.toHexString(hprofData.getStartMillis()),
                     Long.toHexString(parsed.getStartMillis()));
        assertEquals(Long.toHexString(hprofData.getFlags()),
                     Long.toHexString(parsed.getFlags()));
        assertEquals(Long.toHexString(hprofData.getDepth()),
                     Long.toHexString(parsed.getDepth()));
        assertEquals(hprofData.getThreadHistory(),
                     parsed.getThreadHistory());
        assertEquals(hprofData.getSamples(),
                     parsed.getSamples());
    }
}
