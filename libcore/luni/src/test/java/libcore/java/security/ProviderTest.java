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

package libcore.java.security;

import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidParameterException;
import java.security.NoSuchAlgorithmException;
import java.security.Provider;
import java.security.SecureRandom;
import java.security.SecureRandomSpi;
import java.security.Security;
import java.security.cert.CRL;
import java.security.cert.CRLSelector;
import java.security.cert.CertSelector;
import java.security.cert.CertStoreException;
import java.security.cert.CertStoreParameters;
import java.security.cert.CertStoreSpi;
import java.security.cert.Certificate;
import java.security.interfaces.RSAPrivateKey;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import javax.crypto.Cipher;
import javax.crypto.NoSuchPaddingException;
import junit.framework.TestCase;
import libcore.javax.crypto.MockKey;

public class ProviderTest extends TestCase {
    private static final boolean LOG_DEBUG = false;

    /**
     * Makes sure all all expected implementations (but not aliases)
     * and that there are no extras, according to what we expect from
     * StandardNames
     */
    public void test_Provider_getServices() throws Exception {
        // build set of expected algorithms
        Map<String,Set<String>> remainingExpected
                = new HashMap<String,Set<String>>(StandardNames.PROVIDER_ALGORITHMS);
        for (Entry<String,Set<String>> entry : remainingExpected.entrySet()) {
            entry.setValue(new HashSet<String>(entry.getValue()));
        }

        List<String> extra = new ArrayList<String>();
        List<String> missing = new ArrayList<String>();

        Provider[] providers = Security.getProviders();
        for (Provider provider : providers) {
            String providerName = provider.getName();
            // ignore BouncyCastle provider if it is installed on the RI
            if (StandardNames.IS_RI && providerName.equals("BC")) {
                continue;
            }
            Set<Provider.Service> services = provider.getServices();
            assertNotNull(services);
            assertFalse(services.isEmpty());

            for (Provider.Service service : services) {
                String type = service.getType();
                String algorithm = service.getAlgorithm().toUpperCase();
                String className = service.getClassName();
                if (LOG_DEBUG) {
                    System.out.println(providerName
                                       + " " + type
                                       + " " + algorithm
                                       + " " + className);
                }

                // remove from remaining, assert unknown if missing
                Set<String> remainingAlgorithms = remainingExpected.get(type);
                if (remainingAlgorithms == null || !remainingAlgorithms.remove(algorithm)) {
                    // seems to be missing, but sometimes the same
                    // algorithm is available from multiple providers
                    // (e.g. KeyFactory RSA is available from
                    // SunRsaSign and SunJSSE), so double check in
                    // original source before giving error
                    if (!(StandardNames.PROVIDER_ALGORITHMS.containsKey(type)
                            && StandardNames.PROVIDER_ALGORITHMS.get(type).contains(algorithm))) {
                        extra.add("Unknown " + type + " " + algorithm + " " + providerName + "\n");
                    }
                } else if ("Cipher".equals(type) && !algorithm.contains("/")) {
                    /*
                     * Cipher selection follows special rules where you can
                     * specify the mode and padding during the getInstance call.
                     * Try to see if the service supports this.
                     */
                    Set<String> toRemove = new HashSet<String>();
                    for (String remainingAlgo : remainingAlgorithms) {
                        String[] parts = remainingAlgo.split("/");
                        if (parts.length == 3 && algorithm.equals(parts[0])) {
                            try {
                                Cipher.getInstance(remainingAlgo, provider);
                                toRemove.add(remainingAlgo);
                            } catch (NoSuchAlgorithmException ignored) {
                            } catch (NoSuchPaddingException ignored) {
                            }
                        }
                    }
                    remainingAlgorithms.removeAll(toRemove);
                }
                if (remainingAlgorithms != null && remainingAlgorithms.isEmpty()) {
                    remainingExpected.remove(type);
                }

                // make sure class exists and can be initialized
                try {
                    assertNotNull(Class.forName(className,
                                                true,
                                                provider.getClass().getClassLoader()));
                } catch (ClassNotFoundException e) {
                    // Sun forgot their own class
                    if (!className.equals("sun.security.pkcs11.P11MAC")) {
                        missing.add(className);
                    }
                }
            }
        }

        // assert that we don't have any extra in the implementation
        Collections.sort(extra); // sort so that its grouped by type
        assertEquals("Extra algorithms", Collections.EMPTY_LIST, extra);

        // assert that we don't have any missing in the implementation
        assertEquals("Missing algorithms", Collections.EMPTY_MAP, remainingExpected);

        // assert that we don't have any missing classes
        Collections.sort(missing); // sort it for readability
        assertEquals("Missing classes", Collections.EMPTY_LIST, missing);
    }

    private static final Pattern alias = Pattern.compile("Alg\\.Alias\\.([^.]*)\\.(.*)");

    /**
     * Makes sure all provider properties either point to a class
     * implementation that exists or are aliases to known algorithms.
     */
    public void test_Provider_Properties() throws Exception {
        /*
         * A useful reference on Provider properties
         * <a href="http://java.sun.com/javase/6/docs/technotes/guides/security/crypto/HowToImplAProvider.html>
         * How to Implement a Provider in the Java &trade; Cryptography Architecture
         * </a>
         */

        Provider[] providers = Security.getProviders();
        for (Provider provider : providers) {
            // check Provider.id proprieties
            assertEquals(provider.getName(),
                         provider.get("Provider.id name"));
            assertEquals(String.valueOf(provider.getVersion()),
                         provider.get("Provider.id version"));
            assertEquals(provider.getInfo(),
                         provider.get("Provider.id info"));
            assertEquals(provider.getClass().getName(),
                         provider.get("Provider.id className"));

            // build map of all known aliases and implementations
            Map<String,String> aliases = new HashMap<String,String>();
            Map<String,String> implementations = new HashMap<String,String>();
            for (Entry<Object,Object> entry : provider.entrySet()) {
                Object k = entry.getKey();
                Object v = entry.getValue();
                assertEquals(String.class, k.getClass());
                assertEquals(String.class, v.getClass());
                String key = (String)k;
                String value = (String)v;

                // skip Provider.id keys, we check well known ones values above
                if (key.startsWith("Provider.id ")) {
                    continue;
                }

                // skip property settings such as: "Signature.SHA1withDSA ImplementedIn" "Software"
                if (key.indexOf(' ') != -1) {
                    continue;
                }

                Matcher m = alias.matcher(key);
                if (m.find()) {
                    String type = m.group(1);
                    aliases.put(key, type + "." + value);
                } else {
                    implementations.put(key, value);
                }
            }

            // verify implementation classes are available
            for (Entry<String,String> entry : implementations.entrySet()) {
                String typeAndAlgorithm = entry.getKey();
                String className = entry.getValue();
                try {
                    assertNotNull(Class.forName(className,
                                                true,
                                                provider.getClass().getClassLoader()));
                } catch (ClassNotFoundException e) {
                    // Sun forgot their own class
                    if (!className.equals("sun.security.pkcs11.P11MAC")) {
                        fail("Could not find class " + className + " for " + typeAndAlgorithm);
                    }
                }
            }

            // make sure all aliases point to some known implementation
            for (Entry<String,String> entry : aliases.entrySet()) {
                String alias  = entry.getKey();
                String actual = entry.getValue();
                assertTrue("Could not find implementation " + actual + " for alias " + alias,
                           implementations.containsKey(actual));
            }
        }
    }

    private static final String[] TYPES_SERVICES_CHECKED = new String[] {
            "KeyFactory", "CertPathBuilder", "Cipher", "SecureRandom",
            "AlgorithmParameterGenerator", "Signature", "KeyPairGenerator", "CertificateFactory",
            "MessageDigest", "KeyAgreement", "CertStore", "SSLContext", "AlgorithmParameters",
            "TrustManagerFactory", "KeyGenerator", "Mac", "CertPathValidator", "SecretKeyFactory",
            "KeyManagerFactory", "KeyStore",
    };

    private static final HashSet<String> TYPES_SUPPORTS_PARAMETER = new HashSet<String>(
            Arrays.asList(new String[] {
                    "Mac", "KeyAgreement", "Cipher", "Signature",
            }));

    private static final HashSet<String> TYPES_NOT_SUPPORTS_PARAMETER = new HashSet<String>(
            Arrays.asList(TYPES_SERVICES_CHECKED));
    static {
        TYPES_NOT_SUPPORTS_PARAMETER.removeAll(TYPES_SUPPORTS_PARAMETER);
    }

    public void test_Provider_getServices_supportsParameter() throws Exception {
        HashSet<String> remainingTypes = new HashSet<String>(Arrays.asList(TYPES_SERVICES_CHECKED));

        HashSet<String> supportsParameterTypes = new HashSet<String>();
        HashSet<String> noSupportsParameterTypes = new HashSet<String>();

        Provider[] providers = Security.getProviders();
        for (Provider provider : providers) {
            Set<Provider.Service> services = provider.getServices();
            assertNotNull(services);
            assertFalse(services.isEmpty());

            for (Provider.Service service : services) {
                final String type = service.getType();
                remainingTypes.remove(type);
                try {
                    service.supportsParameter(new MockKey());
                    supportsParameterTypes.add(type);
                } catch (InvalidParameterException e) {
                    noSupportsParameterTypes.add(type);
                    try {
                        service.supportsParameter(new Object());
                        fail("Should throw on non-Key parameter");
                    } catch (InvalidParameterException expected) {
                    }
                }
            }
        }

        supportsParameterTypes.retainAll(TYPES_SUPPORTS_PARAMETER);
        assertEquals("Types that should support parameters", TYPES_SUPPORTS_PARAMETER,
                supportsParameterTypes);

        noSupportsParameterTypes.retainAll(TYPES_NOT_SUPPORTS_PARAMETER);
        assertEquals("Types that should not support parameters", TYPES_NOT_SUPPORTS_PARAMETER,
                noSupportsParameterTypes);

        assertEquals("Types that should be checked", Collections.EMPTY_SET, remainingTypes);
    }

    public static class MockSpi {
        public Object parameter;

        public MockSpi(MockKey parameter) {
            this.parameter = parameter;
        }
    };

    @SuppressWarnings("serial")
    public void testProviderService_supportsParameter_UnknownService_Success() throws Exception {
        Provider provider = new MockProvider("MockProvider") {
            public void setup() {
                put("Fake.FOO", MockSpi.class.getName());
            }
        };

        Security.addProvider(provider);
        try {
            Provider.Service service = provider.getService("Fake", "FOO");
            assertTrue(service.supportsParameter(new Object()));
        } finally {
            Security.removeProvider(provider.getName());
        }
    }

    @SuppressWarnings("serial")
    public void testProviderService_supportsParameter_KnownService_NoClassInitialization_Success()
            throws Exception {
        Provider provider = new MockProvider("MockProvider") {
            public void setup() {
                put("Signature.FOO", MockSpi.class.getName());
                put("Signature.FOO SupportedKeyClasses", getClass().getName()
                        + ".UninitializedMockKey");
            }
        };

        Security.addProvider(provider);
        try {
            Provider.Service service = provider.getService("Signature", "FOO");
            assertFalse(service.supportsParameter(new MockKey()));
        } finally {
            Security.removeProvider(provider.getName());
        }
    }

    @SuppressWarnings("serial")
    public static class UninitializedMockKey extends MockKey {
        static {
            fail("This should not be initialized");
        }
    }

    @SuppressWarnings("serial")
    public void testProviderService_supportsParameter_TypeDoesNotSupportParameter_Failure()
            throws Exception {
        Provider provider = new MockProvider("MockProvider") {
            public void setup() {
                put("KeyFactory.FOO", MockSpi.class.getName());
            }
        };

        Security.addProvider(provider);
        try {
            Provider.Service service = provider.getService("KeyFactory", "FOO");
            try {
                service.supportsParameter(new MockKey());
                fail("Should always throw exception");
            } catch (InvalidParameterException expected) {
            }
        } finally {
            Security.removeProvider(provider.getName());
        }
    }

    @SuppressWarnings("serial")
    public void testProviderService_supportsParameter_SupportedKeyClasses_NonKeyClass_Success()
            throws Exception {
        Provider provider = new MockProvider("MockProvider") {
            public void setup() {
                put("Signature.FOO", MockSpi.class.getName());
                put("Signature.FOO SupportedKeyClasses", MockSpi.class.getName());
            }
        };

        Security.addProvider(provider);
        try {
            Provider.Service service = provider.getService("Signature", "FOO");
            assertFalse(service.supportsParameter(new MockKey()));
        } finally {
            Security.removeProvider(provider.getName());
        }
    }

    @SuppressWarnings("serial")
    public void testProviderService_supportsParameter_KnownService_NonKey_Failure()
            throws Exception {
        Provider provider = new MockProvider("MockProvider") {
            public void setup() {
                put("Signature.FOO", MockSpi.class.getName());
            }
        };

        Security.addProvider(provider);
        try {
            Provider.Service service = provider.getService("Signature", "FOO");
            try {
                service.supportsParameter(new Object());
                fail("Should throw when non-Key passed in");
            } catch (InvalidParameterException expected) {
            }
        } finally {
            Security.removeProvider(provider.getName());
        }
    }

    @SuppressWarnings("serial")
    public void testProviderService_supportsParameter_KnownService_SupportedKeyClasses_NonKey_Failure()
            throws Exception {
        Provider provider = new MockProvider("MockProvider") {
            public void setup() {
                put("Signature.FOO", MockSpi.class.getName());
                put("Signature.FOO SupportedKeyClasses", RSAPrivateKey.class.getName());
            }
        };

        Security.addProvider(provider);
        try {
            Provider.Service service = provider.getService("Signature", "FOO");
            try {
                service.supportsParameter(new Object());
                fail("Should throw on non-Key instance passed in");
            } catch (InvalidParameterException expected) {
            }
        } finally {
            Security.removeProvider(provider.getName());
        }
    }

    @SuppressWarnings("serial")
    public void testProviderService_supportsParameter_KnownService_Null_Failure() throws Exception {
        Provider provider = new MockProvider("MockProvider") {
            public void setup() {
                put("Signature.FOO", MockSpi.class.getName());
                put("Signature.FOO SupportedKeyClasses", RSAPrivateKey.class.getName());
            }
        };

        Security.addProvider(provider);
        try {
            Provider.Service service = provider.getService("Signature", "FOO");
            assertFalse(service.supportsParameter(null));
        } finally {
            Security.removeProvider(provider.getName());
        }
    }

    @SuppressWarnings("serial")
    public void testProviderService_supportsParameter_SupportedKeyClasses_Success()
            throws Exception {
        Provider provider = new MockProvider("MockProvider") {
            public void setup() {
                put("Signature.FOO", MockSpi.class.getName());
                put("Signature.FOO SupportedKeyClasses", MockKey.class.getName());
            }
        };

        Security.addProvider(provider);
        try {
            Provider.Service service = provider.getService("Signature", "FOO");
            assertTrue(service.supportsParameter(new MockKey()));
        } finally {
            Security.removeProvider(provider.getName());
        }
    }

    @SuppressWarnings("serial")
    public void testProviderService_supportsParameter_SupportedKeyClasses_Failure()
            throws Exception {
        Provider provider = new MockProvider("MockProvider") {
            public void setup() {
                put("Signature.FOO", MockSpi.class.getName());
                put("Signature.FOO SupportedKeyClasses", RSAPrivateKey.class.getName());
            }
        };

        Security.addProvider(provider);
        try {
            Provider.Service service = provider.getService("Signature", "FOO");
            assertFalse(service.supportsParameter(new MockKey()));
        } finally {
            Security.removeProvider(provider.getName());
        }
    }

    @SuppressWarnings("serial")
    public void testProviderService_supportsParameter_SupportedKeyFormats_Success()
            throws Exception {
        Provider provider = new MockProvider("MockProvider") {
            public void setup() {
                put("Signature.FOO", MockSpi.class.getName());
                put("Signature.FOO SupportedKeyFormats", new MockKey().getFormat());
            }
        };

        Security.addProvider(provider);
        try {
            Provider.Service service = provider.getService("Signature", "FOO");
            assertTrue(service.supportsParameter(new MockKey()));
        } finally {
            Security.removeProvider(provider.getName());
        }
    }

    @SuppressWarnings("serial")
    public void testProviderService_supportsParameter_SupportedKeyFormats_Failure()
            throws Exception {
        Provider provider = new MockProvider("MockProvider") {
            public void setup() {
                put("Signature.FOO", MockSpi.class.getName());
                put("Signature.FOO SupportedKeyFormats", "Invalid");
            }
        };

        Security.addProvider(provider);
        try {
            Provider.Service service = provider.getService("Signature", "FOO");
            assertFalse(service.supportsParameter(new MockKey()));
        } finally {
            Security.removeProvider(provider.getName());
        }
    }

    @SuppressWarnings("serial")
    public void testProviderService_newInstance_DoesNotCallSupportsParameter_Success()
            throws Exception {
        MockProvider provider = new MockProvider("MockProvider");

        provider.putServiceForTest(new Provider.Service(provider, "CertStore", "FOO",
                MyCertStoreSpi.class.getName(), null, null) {
            @Override
            public boolean supportsParameter(Object parameter) {
                fail("This should not be called");
                return false;
            }
        });

        Security.addProvider(provider);
        try {
            Provider.Service service = provider.getService("CertStore", "FOO");
            assertNotNull(service.newInstance(new MyCertStoreParameters()));
        } finally {
            Security.removeProvider(provider.getName());
        }
    }

    public static class MyCertStoreSpi extends CertStoreSpi {
        public MyCertStoreSpi(CertStoreParameters params) throws InvalidAlgorithmParameterException {
            super(params);
        }

        @Override
        public Collection<? extends Certificate> engineGetCertificates(CertSelector selector)
                throws CertStoreException {
            throw new UnsupportedOperationException();
        }

        @Override
        public Collection<? extends CRL> engineGetCRLs(CRLSelector selector)
                throws CertStoreException {
            throw new UnsupportedOperationException();
        }
    }

    public static class MyCertStoreParameters implements CertStoreParameters {
        public Object clone() {
            return new MyCertStoreParameters();
        }
    }

    /**
     * http://code.google.com/p/android/issues/detail?id=21449
     */
    public void testSecureRandomImplementationOrder() {
        @SuppressWarnings("serial")
        Provider srp = new MockProvider("SRProvider") {
            public void setup() {
                put("SecureRandom.SecureRandom1", SecureRandom1.class.getName());
                put("SecureRandom.SecureRandom2", SecureRandom2.class.getName());
                put("SecureRandom.SecureRandom3", SecureRandom3.class.getName());
            }
        };
        try {
            int position = Security.insertProviderAt(srp, 1); // first is one, not zero
            assertEquals(1, position);
            SecureRandom sr = new SecureRandom();
            if (!sr.getAlgorithm().equals("SecureRandom1")) {
                throw new IllegalStateException("Expected SecureRandom1 was " + sr.getAlgorithm());
            }
        } finally {
            Security.removeProvider(srp.getName());
        }
    }

    @SuppressWarnings("serial")
    private static class MockProvider extends Provider {
        public MockProvider(String name) {
            super(name, 1.0, "Mock provider used for testing");
            setup();
        }

        public void setup() {
        }

        public void putServiceForTest(Provider.Service service) {
            putService(service);
        }
    }

    @SuppressWarnings("serial")
    public static abstract class AbstractSecureRandom extends SecureRandomSpi {
        protected void engineSetSeed(byte[] seed) {
            throw new UnsupportedOperationException();
        }
        protected void engineNextBytes(byte[] bytes) {
            throw new UnsupportedOperationException();
        }
        protected byte[] engineGenerateSeed(int numBytes) {
            throw new UnsupportedOperationException();
        }
    }

    @SuppressWarnings("serial")
    public static class SecureRandom1 extends AbstractSecureRandom {}

    @SuppressWarnings("serial")
    public static class SecureRandom2 extends AbstractSecureRandom {}

    @SuppressWarnings("serial")
    public static class SecureRandom3 extends AbstractSecureRandom {}

}
