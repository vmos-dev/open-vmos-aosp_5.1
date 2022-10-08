/*
 * Copyright (C) 2013 The Android Open Source Project
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

package libcore.util;

import java.io.File;
import java.io.FileOutputStream;
import java.io.RandomAccessFile;
import java.util.TimeZone;

public class ZoneInfoDBTest extends junit.framework.TestCase {

  // The base tzdata file, always present on a device.
  private static final String TZDATA_IN_ROOT =
      System.getenv("ANDROID_ROOT") + "/usr/share/zoneinfo/tzdata";

  // An empty override file should fall back to the default file.
  public void testEmptyOverrideFile() throws Exception {
    ZoneInfoDB.TzData data = new ZoneInfoDB.TzData(TZDATA_IN_ROOT);
    ZoneInfoDB.TzData dataWithEmptyOverride =
        new ZoneInfoDB.TzData(makeEmptyFile(), TZDATA_IN_ROOT);
    assertEquals(data.getVersion(), dataWithEmptyOverride.getVersion());
    assertEquals(data.getAvailableIDs().length, dataWithEmptyOverride.getAvailableIDs().length);
  }

  // A corrupt override file should fall back to the default file.
  public void testCorruptOverrideFile() throws Exception {
    ZoneInfoDB.TzData data = new ZoneInfoDB.TzData(TZDATA_IN_ROOT);
    ZoneInfoDB.TzData dataWithCorruptOverride =
        new ZoneInfoDB.TzData(makeCorruptFile(), TZDATA_IN_ROOT);
    assertEquals(data.getVersion(), dataWithCorruptOverride.getVersion());
    assertEquals(data.getAvailableIDs().length, dataWithCorruptOverride.getAvailableIDs().length);
  }

  // Given no tzdata files we can use, we should fall back to built-in "GMT".
  public void testNoGoodFile() throws Exception {
    ZoneInfoDB.TzData data = new ZoneInfoDB.TzData(makeEmptyFile());
    assertEquals("missing", data.getVersion());
    assertEquals(1, data.getAvailableIDs().length);
    assertEquals("GMT", data.getAvailableIDs()[0]);
  }

  // Given a valid override file, we should find ourselves using that.
  public void testGoodOverrideFile() throws Exception {
    RandomAccessFile in = new RandomAccessFile(TZDATA_IN_ROOT, "r");
    byte[] content = new byte[(int) in.length()];
    in.readFully(content);
    // Bump the version number to one long past where humans will be extinct.
    content[6] = '9';
    content[7] = '9';
    content[8] = '9';
    content[9] = '9';
    content[10] = 'z';
    in.close();

    ZoneInfoDB.TzData data = new ZoneInfoDB.TzData(TZDATA_IN_ROOT);
    String goodFile = makeTemporaryFile(content);
    try {
      ZoneInfoDB.TzData dataWithOverride = new ZoneInfoDB.TzData(goodFile, TZDATA_IN_ROOT);
      assertEquals("9999z", dataWithOverride.getVersion());
      assertEquals(data.getAvailableIDs().length, dataWithOverride.getAvailableIDs().length);
    } finally {
      new File(goodFile).delete();
    }
  }

  // Confirms any caching that exists correctly handles TimeZone mutability.
  public void testMakeTimeZone_timeZoneMutability() throws Exception {
    ZoneInfoDB.TzData data = new ZoneInfoDB.TzData(TZDATA_IN_ROOT);
    String tzId = "Europe/London";
    ZoneInfo first = data.makeTimeZone(tzId);
    ZoneInfo second = data.makeTimeZone(tzId);
    assertNotSame(first, second);

    assertTrue(first.hasSameRules(second));

    first.setID("Not Europe/London");

    assertFalse(first.getID().equals(second.getID()));

    first.setRawOffset(3600);
    assertFalse(first.getRawOffset() == second.getRawOffset());
  }

  public void testMakeTimeZone_notFound() throws Exception {
    ZoneInfoDB.TzData data = new ZoneInfoDB.TzData(TZDATA_IN_ROOT);
    assertNull(data.makeTimeZone("THIS_TZ_DOES_NOT_EXIST"));
  }

  private static String makeCorruptFile() throws Exception {
    return makeTemporaryFile("invalid content".getBytes());
  }

  private static String makeEmptyFile() throws Exception {
    return makeTemporaryFile(new byte[0]);
  }

  private static String makeTemporaryFile(byte[] content) throws Exception {
    File f = File.createTempFile("temp-", ".txt");
    FileOutputStream fos = new FileOutputStream(f);
    fos.write(content);
    fos.close();
    return f.getPath();
  }
}
