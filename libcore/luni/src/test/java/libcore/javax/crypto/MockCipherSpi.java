/*
 * Copyright 2013 The Android Open Source Project
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

package libcore.javax.crypto;

import java.security.AlgorithmParameters;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.Key;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.spec.AlgorithmParameterSpec;

import javax.crypto.BadPaddingException;
import javax.crypto.CipherSpi;
import javax.crypto.IllegalBlockSizeException;
import javax.crypto.NoSuchPaddingException;
import javax.crypto.ShortBufferException;

/**
 * Mock CipherSpi used by {@link CipherTest}.
 */
public class MockCipherSpi extends CipherSpi {
    public static class SpecificKeyTypes extends MockCipherSpi {
        @Override
        public void checkKeyType(Key key) throws InvalidKeyException {
            if (!(key instanceof MockKey)) {
                throw new InvalidKeyException("Must be MockKey!");
            }
        }
    }

    public static class SpecificKeyTypes2 extends MockCipherSpi {
        @Override
        public void checkKeyType(Key key) throws InvalidKeyException {
            System.err.println("Checking key of type " + key.getClass().getName());
            if (!(key instanceof MockKey2)) {
                throw new InvalidKeyException("Must be MockKey2!");
            }
        }
    }

    public static class AllKeyTypes extends MockCipherSpi {
    }

    public void checkKeyType(Key key) throws InvalidKeyException {
    }

    @Override
    protected void engineSetMode(String mode) throws NoSuchAlgorithmException {
        if (!"FOO".equals(mode)) {
            throw new UnsupportedOperationException("not implemented");
        }
    }

    @Override
    protected void engineSetPadding(String padding) throws NoSuchPaddingException {
        if (!"FOO".equals(padding)) {
            throw new UnsupportedOperationException("not implemented");
        }
    }

    @Override
    protected int engineGetBlockSize() {
        throw new UnsupportedOperationException("not implemented");
    }

    @Override
    protected int engineGetOutputSize(int inputLen) {
        throw new UnsupportedOperationException("not implemented");
    }

    @Override
    protected byte[] engineGetIV() {
        throw new UnsupportedOperationException("not implemented");
    }

    @Override
    protected AlgorithmParameters engineGetParameters() {
        throw new UnsupportedOperationException("not implemented");
    }

    @Override
    protected void engineInit(int opmode, Key key, SecureRandom random) throws InvalidKeyException {
        checkKeyType(key);
    }

    @Override
    protected void engineInit(int opmode, Key key, AlgorithmParameterSpec params,
            SecureRandom random) throws InvalidKeyException, InvalidAlgorithmParameterException {
        checkKeyType(key);
    }

    @Override
    protected void engineInit(int opmode, Key key, AlgorithmParameters params, SecureRandom random)
            throws InvalidKeyException, InvalidAlgorithmParameterException {
        checkKeyType(key);
    }

    @Override
    protected byte[] engineUpdate(byte[] input, int inputOffset, int inputLen) {
        throw new UnsupportedOperationException("not implemented");
    }

    @Override
    protected int engineUpdate(byte[] input, int inputOffset, int inputLen, byte[] output,
            int outputOffset) throws ShortBufferException {
        throw new UnsupportedOperationException("not implemented");
    }

    @Override
    protected byte[] engineDoFinal(byte[] input, int inputOffset, int inputLen)
            throws IllegalBlockSizeException, BadPaddingException {
        throw new UnsupportedOperationException("not implemented");
    }

    @Override
    protected int engineDoFinal(byte[] input, int inputOffset, int inputLen, byte[] output,
            int outputOffset) throws ShortBufferException, IllegalBlockSizeException,
            BadPaddingException {
        throw new UnsupportedOperationException("not implemented");
    }
}
