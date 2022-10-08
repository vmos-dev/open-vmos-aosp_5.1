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

package libcore.java.text;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.text.DecimalFormatSymbols;
import java.util.Currency;
import java.util.Locale;

public class DecimalFormatSymbolsTest extends junit.framework.TestCase {
    public void test_getInstance_unknown_or_invalid_locale() throws Exception {
        // http://b/17374604: this test passes on the host but fails on the target.
        // ICU uses setlocale(3) to determine its default locale, and glibc (on my box at least)
        // returns "en_US.UTF-8". bionic before L returned NULL and in L returns "C.UTF-8", both
        // of which get treated as "en_US_POSIX". What that means for this test is that you get
        // "INF" for infinity instead of "\u221e".
        // On the RI, this test fails for a different reason: their DecimalFormatSymbols.equals
        // appears to be broken. It could be that they're accidentally checking the Locale field?
        checkLocaleIsEquivalentToRoot(new Locale("xx", "XX"));
        checkLocaleIsEquivalentToRoot(new Locale("not exist language", "not exist country"));
    }
    private void checkLocaleIsEquivalentToRoot(Locale locale) {
        DecimalFormatSymbols dfs = DecimalFormatSymbols.getInstance(locale);
        assertEquals(DecimalFormatSymbols.getInstance(Locale.ROOT), dfs);
    }

    // http://code.google.com/p/android/issues/detail?id=14495
    public void testSerialization() throws Exception {
        DecimalFormatSymbols originalDfs = DecimalFormatSymbols.getInstance(Locale.GERMANY);

        // Serialize...
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        new ObjectOutputStream(out).writeObject(originalDfs);
        byte[] bytes = out.toByteArray();

        // Deserialize...
        ObjectInputStream in = new ObjectInputStream(new ByteArrayInputStream(bytes));
        DecimalFormatSymbols deserializedDfs = (DecimalFormatSymbols) in.readObject();
        assertEquals(-1, in.read());

        // The two objects should claim to be equal.
        assertEquals(originalDfs, deserializedDfs);
    }

    // https://code.google.com/p/android/issues/detail?id=79925
    public void testSetSameCurrency() throws Exception {
        DecimalFormatSymbols dfs = new DecimalFormatSymbols(Locale.US);
        dfs.setCurrency(Currency.getInstance("USD"));
        assertEquals("$", dfs.getCurrencySymbol());
        dfs.setCurrencySymbol("poop");
        assertEquals("poop", dfs.getCurrencySymbol());
        dfs.setCurrency(Currency.getInstance("USD"));
        assertEquals("$", dfs.getCurrencySymbol());
    }

    public void testSetNulInternationalCurrencySymbol() throws Exception {
        Currency usd = Currency.getInstance("USD");

        DecimalFormatSymbols dfs = new DecimalFormatSymbols(Locale.US);
        dfs.setCurrency(usd);
        assertEquals(usd, dfs.getCurrency());
        assertEquals("$", dfs.getCurrencySymbol());
        assertEquals("USD", dfs.getInternationalCurrencySymbol());

        // Setting the international currency symbol to null sets the currency to null too,
        // but not the currency symbol.
        dfs.setInternationalCurrencySymbol(null);
        assertEquals(null, dfs.getCurrency());
        assertEquals("$", dfs.getCurrencySymbol());
        assertEquals(null, dfs.getInternationalCurrencySymbol());
    }
}
