/*
 * Copyright (C) 2009 The Android Open Source Project
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

package libcore.icu;

import java.util.Locale;

public class NativePluralRulesTest extends junit.framework.TestCase {
    public void testNegatives() throws Exception {
        // icu4c's behavior changed, but we prefer to preserve compatibility.
        NativePluralRules en_US = NativePluralRules.forLocale(new Locale("en", "US"));
        assertEquals(NativePluralRules.OTHER, en_US.quantityForInt(2));
        assertEquals(NativePluralRules.ONE, en_US.quantityForInt(1));
        assertEquals(NativePluralRules.OTHER, en_US.quantityForInt(0));
        assertEquals(NativePluralRules.OTHER, en_US.quantityForInt(-1));
        assertEquals(NativePluralRules.OTHER, en_US.quantityForInt(-2));

        NativePluralRules ar = NativePluralRules.forLocale(new Locale("ar"));
        assertEquals(NativePluralRules.ZERO, ar.quantityForInt(0));
        assertEquals(NativePluralRules.OTHER, ar.quantityForInt(-1)); // Not ONE.
        assertEquals(NativePluralRules.OTHER, ar.quantityForInt(-2)); // Not TWO.
        assertEquals(NativePluralRules.OTHER, ar.quantityForInt(-3)); // Not FEW.
        assertEquals(NativePluralRules.OTHER, ar.quantityForInt(-11)); // Not MANY.
        assertEquals(NativePluralRules.OTHER, ar.quantityForInt(-100));
    }

    public void testEnglish() throws Exception {
        NativePluralRules npr = NativePluralRules.forLocale(new Locale("en", "US"));
        assertEquals(NativePluralRules.OTHER, npr.quantityForInt(0));
        assertEquals(NativePluralRules.ONE, npr.quantityForInt(1));
        assertEquals(NativePluralRules.OTHER, npr.quantityForInt(2));
    }

    public void testCzech() throws Exception {
        NativePluralRules npr = NativePluralRules.forLocale(new Locale("cs", "CZ"));
        assertEquals(NativePluralRules.OTHER, npr.quantityForInt(0));
        assertEquals(NativePluralRules.ONE, npr.quantityForInt(1));
        assertEquals(NativePluralRules.FEW, npr.quantityForInt(2));
        assertEquals(NativePluralRules.FEW, npr.quantityForInt(3));
        assertEquals(NativePluralRules.FEW, npr.quantityForInt(4));
        assertEquals(NativePluralRules.OTHER, npr.quantityForInt(5));
    }

    public void testArabic() throws Exception {
        NativePluralRules npr = NativePluralRules.forLocale(new Locale("ar"));
        assertEquals(NativePluralRules.ZERO, npr.quantityForInt(0));
        assertEquals(NativePluralRules.ONE, npr.quantityForInt(1));
        assertEquals(NativePluralRules.TWO, npr.quantityForInt(2));
        for (int i = 3; i <= 10; ++i) {
            assertEquals(NativePluralRules.FEW, npr.quantityForInt(i));
        }
        assertEquals(NativePluralRules.MANY, npr.quantityForInt(11));
        assertEquals(NativePluralRules.MANY, npr.quantityForInt(99));
        assertEquals(NativePluralRules.OTHER, npr.quantityForInt(100));
        assertEquals(NativePluralRules.OTHER, npr.quantityForInt(101));
        assertEquals(NativePluralRules.OTHER, npr.quantityForInt(102));
        assertEquals(NativePluralRules.FEW, npr.quantityForInt(103));
        assertEquals(NativePluralRules.MANY, npr.quantityForInt(111));
    }

    public void testHebrew() throws Exception {
        // java.util.Locale will translate "he" to the deprecated "iw".
        NativePluralRules he = NativePluralRules.forLocale(new Locale("he"));
        assertEquals(NativePluralRules.ONE, he.quantityForInt(1));
        assertEquals(NativePluralRules.TWO, he.quantityForInt(2));
        assertEquals(NativePluralRules.OTHER, he.quantityForInt(3));
        assertEquals(NativePluralRules.OTHER, he.quantityForInt(10));
    }
}

