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

package libcore.javax.net.ssl;

import javax.net.ssl.DistinguishedNameParser;
import javax.security.auth.x500.X500Principal;
import junit.framework.TestCase;
import java.util.Arrays;

public final class DistinguishedNameParserTest extends TestCase {
    public void testGetCns() {
        assertCns("");
        assertCns("ou=xxx");
        assertCns("ou=xxx,cn=xxx", "xxx");
        assertCns("ou=xxx+cn=yyy,cn=zzz+cn=abc", "yyy", "zzz", "abc");
        assertCns("cn=a,cn=b", "a", "b");
        assertCns("cn=Cc,cn=Bb,cn=Aa", "Cc", "Bb", "Aa");
        assertCns("cn=imap.gmail.com", "imap.gmail.com");
        assertCns("l=\"abcn=a,b\", cn=c", "c");
    }

    public void testGetCnsWithOid() {
        assertCns("2.5.4.3=a,ou=xxx", "a");
    }

    public void testGetCnsWithQuotedStrings() {
        assertCns("cn=\"\\\" a ,=<>#;\"", "\" a ,=<>#;");
        assertCns("cn=abc\\,def", "abc,def");
    }

    public void testGetCnsWithUtf8() {
        assertCns("cn=Lu\\C4\\8Di\\C4\\87", "\u004c\u0075\u010d\u0069\u0107");
    }

    public void testGetCnsWithWhitespace() {
        assertCns("ou=a, cn=  a  b  ,o=x", "a  b");
        assertCns("cn=\"  a  b  \" ,o=x", "  a  b  ");
    }

    private void assertCns(String dn, String... expected) {
        X500Principal principal = new X500Principal(dn);
        DistinguishedNameParser parser = new DistinguishedNameParser(principal);

        // Test getAllMostSpecificFirst
        assertEquals(dn, Arrays.asList(expected), parser.getAllMostSpecificFirst("cn"));

        // Test findMostSpecific
        if (expected.length > 0) {
            assertEquals(dn, expected[0], parser.findMostSpecific("cn"));
        } else {
            assertNull(dn, parser.findMostSpecific("cn"));
        }
    }
}
