/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package libcore.javax.crypto;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.FilterInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.security.spec.AlgorithmParameterSpec;
import java.util.Arrays;
import javax.crypto.Cipher;
import javax.crypto.CipherInputStream;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import junit.framework.TestCase;

public final class CipherInputStreamTest extends TestCase {

    private final byte[] aesKeyBytes = {
            (byte) 0x50, (byte) 0x98, (byte) 0xF2, (byte) 0xC3, (byte) 0x85, (byte) 0x23,
            (byte) 0xA3, (byte) 0x33, (byte) 0x50, (byte) 0x98, (byte) 0xF2, (byte) 0xC3,
            (byte) 0x85, (byte) 0x23, (byte) 0xA3, (byte) 0x33,
    };

    private final byte[] aesIvBytes = {
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
            (byte) 0x00, (byte) 0x00, (byte) 0x00, (byte) 0x00,
    };

    private final byte[] aesCipherText = {
            (byte) 0x2F, (byte) 0x2C, (byte) 0x74, (byte) 0x31, (byte) 0xFF, (byte) 0xCC,
            (byte) 0x28, (byte) 0x7D, (byte) 0x59, (byte) 0xBD, (byte) 0xE5, (byte) 0x0A,
            (byte) 0x30, (byte) 0x7E, (byte) 0x6A, (byte) 0x4A
    };

    private final byte[] rc4CipherText = {
            (byte) 0x88, (byte) 0x01, (byte) 0xE3, (byte) 0x52, (byte) 0x7B
    };

    private final String plainText = "abcde";
    private SecretKey key;
    private SecretKey rc4Key;
    private AlgorithmParameterSpec iv;

    @Override protected void setUp() throws Exception {
        key = new SecretKeySpec(aesKeyBytes, "AES");
        rc4Key = new SecretKeySpec(aesKeyBytes, "RC4");
        iv = new IvParameterSpec(aesIvBytes);
    }

    private static class MeasuringInputStream extends FilterInputStream {
        private int totalRead;

        protected MeasuringInputStream(InputStream in) {
            super(in);
        }

        @Override
        public int read() throws IOException {
            int c = super.read();
            totalRead++;
            return c;
        }

        @Override
        public int read(byte[] buffer, int byteOffset, int byteCount) throws IOException {
            int numRead = super.read(buffer, byteOffset, byteCount);
            if (numRead != -1) {
                totalRead += numRead;
            }
            return numRead;
        }

        public int getTotalRead() {
            return totalRead;
        }
    }

    public void testAvailable() throws Exception {
        Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
        cipher.init(Cipher.DECRYPT_MODE, key, iv);
        MeasuringInputStream in = new MeasuringInputStream(new ByteArrayInputStream(aesCipherText));
        InputStream cin = new CipherInputStream(in, cipher);
        assertTrue(cin.read() != -1);
        assertEquals(aesCipherText.length, in.getTotalRead());
    }

    public void testDecrypt_NullInput_Discarded() throws Exception {
        Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
        cipher.init(Cipher.DECRYPT_MODE, key, iv);
        InputStream in = new CipherInputStream(new ByteArrayInputStream(aesCipherText), cipher);
        int discard = 3;
        while (discard != 0) {
            discard -= in.read(null, 0, discard);
        }
        byte[] bytes = readAll(in);
        assertEquals(Arrays.toString(plainText.substring(3).getBytes("UTF-8")),
                Arrays.toString(bytes));
    }

    public void testEncrypt() throws Exception {
        Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
        cipher.init(Cipher.ENCRYPT_MODE, key, iv);
        InputStream in = new CipherInputStream(
                new ByteArrayInputStream(plainText.getBytes("UTF-8")), cipher);
        byte[] bytes = readAll(in);
        assertEquals(Arrays.toString(aesCipherText), Arrays.toString(bytes));

        // Reading again shouldn't throw an exception.
        assertEquals(-1, in.read());
    }

    public void testEncrypt_RC4() throws Exception {
        Cipher cipher = Cipher.getInstance("RC4");
        cipher.init(Cipher.ENCRYPT_MODE, rc4Key);
        InputStream in = new CipherInputStream(
                new ByteArrayInputStream(plainText.getBytes("UTF-8")), cipher);
        byte[] bytes = readAll(in);
        assertEquals(Arrays.toString(rc4CipherText), Arrays.toString(bytes));

        // Reading again shouldn't throw an exception.
        assertEquals(-1, in.read());
    }

    public void testDecrypt() throws Exception {
        Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
        cipher.init(Cipher.DECRYPT_MODE, key, iv);
        InputStream in = new CipherInputStream(new ByteArrayInputStream(aesCipherText), cipher);
        byte[] bytes = readAll(in);
        assertEquals(Arrays.toString(plainText.getBytes("UTF-8")), Arrays.toString(bytes));
    }

    public void testDecrypt_RC4() throws Exception {
        Cipher cipher = Cipher.getInstance("RC4");
        cipher.init(Cipher.DECRYPT_MODE, rc4Key);
        InputStream in = new CipherInputStream(new ByteArrayInputStream(rc4CipherText), cipher);
        byte[] bytes = readAll(in);
        assertEquals(Arrays.toString(plainText.getBytes("UTF-8")), Arrays.toString(bytes));
    }

    public void testSkip() throws Exception {
        Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
        cipher.init(Cipher.DECRYPT_MODE, key, iv);
        InputStream in = new CipherInputStream(new ByteArrayInputStream(aesCipherText), cipher);
        assertTrue(in.skip(5) >= 0);
    }

    private byte[] readAll(InputStream in) throws IOException {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        int count;
        byte[] buffer = new byte[1024];
        while ((count = in.read(buffer)) != -1) {
            out.write(buffer, 0, count);
        }
        return out.toByteArray();
    }

    public void testCipherInputStream_TruncatedInput_Failure() throws Exception {
        Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
        cipher.init(Cipher.DECRYPT_MODE, key, iv);
        InputStream is = new CipherInputStream(new ByteArrayInputStream(new byte[31]), cipher);
        is.read(new byte[4]);
        is.close();
    }

    public void testCipherInputStream_NullInputStream_Failure() throws Exception {
        Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
        cipher.init(Cipher.DECRYPT_MODE, key, iv);
        InputStream is = new CipherInputStream(null, cipher);
        try {
            is.read();
            fail("Expected NullPointerException");
        } catch (NullPointerException expected) {
        }

        byte[] buffer = new byte[128];
        try {
            is.read(buffer);
            fail("Expected NullPointerException");
        } catch (NullPointerException expected) {
        }

        try {
            is.read(buffer, 0, buffer.length);
            fail("Expected NullPointerException");
        } catch (NullPointerException expected) {
        }
    }
}
