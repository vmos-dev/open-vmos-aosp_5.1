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

public class IntegerTest extends junit.framework.TestCase {

  public void testSystemProperties() {
    Properties originalProperties = System.getProperties();
    try {
      Properties testProperties = new Properties();
      testProperties.put("testIncInt", "notInt");
      System.setProperties(testProperties);
      assertNull("returned incorrect default Integer",
        Integer.getInteger("testIncInt"));
      assertEquals(new Integer(4), Integer.getInteger("testIncInt", 4));
      assertEquals(new Integer(4),
        Integer.getInteger("testIncInt", new Integer(4)));
    } finally {
      System.setProperties(originalProperties);
    }
  }

  public void testCompare() throws Exception {
    final int min = Integer.MIN_VALUE;
    final int zero = 0;
    final int max = Integer.MAX_VALUE;
    assertTrue(Integer.compare(max,  max)  == 0);
    assertTrue(Integer.compare(min,  min)  == 0);
    assertTrue(Integer.compare(zero, zero) == 0);
    assertTrue(Integer.compare(max,  zero) > 0);
    assertTrue(Integer.compare(max,  min)  > 0);
    assertTrue(Integer.compare(zero, max)  < 0);
    assertTrue(Integer.compare(zero, min)  > 0);
    assertTrue(Integer.compare(min,  zero) < 0);
    assertTrue(Integer.compare(min,  max)  < 0);
  }

  public void testParseInt() throws Exception {
    assertEquals(0, Integer.parseInt("+0", 10));
    assertEquals(473, Integer.parseInt("+473", 10));
    assertEquals(255, Integer.parseInt("+FF", 16));
    assertEquals(102, Integer.parseInt("+1100110", 2));
    assertEquals(2147483647, Integer.parseInt("+2147483647", 10));
    assertEquals(411787, Integer.parseInt("Kona", 27));
    assertEquals(411787, Integer.parseInt("+Kona", 27));
    assertEquals(-145, Integer.parseInt("-145", 10));

    try {
      Integer.parseInt("--1", 10); // multiple sign chars
      fail();
    } catch (NumberFormatException expected) {}

    try {
      Integer.parseInt("++1", 10); // multiple sign chars
      fail();
    } catch (NumberFormatException expected) {}

    try {
      Integer.parseInt("Kona", 10); // base too small
      fail();
    } catch (NumberFormatException expected) {}
  }

  public void testDecodeInt() throws Exception {
    assertEquals(0, Integer.decode("+0").intValue());
    assertEquals(473, Integer.decode("+473").intValue());
    assertEquals(255, Integer.decode("+0xFF").intValue());
    assertEquals(16, Integer.decode("+020").intValue());
    assertEquals(2147483647, Integer.decode("+2147483647").intValue());
    assertEquals(-73, Integer.decode("-73").intValue());
    assertEquals(-255, Integer.decode("-0xFF").intValue());
    assertEquals(255, Integer.decode("+#FF").intValue());
    assertEquals(-255, Integer.decode("-#FF").intValue());

    try {
      Integer.decode("--1"); // multiple sign chars
      fail();
    } catch (NumberFormatException expected) {}

    try {
      Integer.decode("++1"); // multiple sign chars
      fail();
    } catch (NumberFormatException expected) {}

    try {
      Integer.decode("-+1"); // multiple sign chars
      fail();
    } catch (NumberFormatException expected) {}

    try {
      Integer.decode("Kona"); // invalid number
      fail();
    } catch (NumberFormatException expected) {}
  }

  public void testParsePositiveInt() throws Exception {
    assertEquals(0, Integer.parsePositiveInt("0", 10));
    assertEquals(473, Integer.parsePositiveInt("473", 10));
    assertEquals(255, Integer.parsePositiveInt("FF", 16));

    try {
      Integer.parsePositiveInt("-1", 10);
      fail();
    } catch (NumberFormatException e) {}

    try {
      Integer.parsePositiveInt("+1", 10);
      fail();
    } catch (NumberFormatException e) {}

    try {
      Integer.parsePositiveInt("+0", 16);
      fail();
    } catch (NumberFormatException e) {}
  }

}
