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

package libcore.javax.net.ssl;

import java.io.IOException;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLEngine;
import javax.net.ssl.SSLParameters;
import javax.net.ssl.SSLServerSocket;
import javax.net.ssl.SSLServerSocketFactory;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;
import junit.framework.Assert;
import libcore.java.security.StandardNames;

/**
 * Assertions about the default configuration of TLS/SSL primitives.
 */
public abstract class SSLDefaultConfigurationAsserts extends Assert {

  /** Hidden constructor to prevent instantiation. */
  private SSLDefaultConfigurationAsserts() {}

  /**
   * Asserts that the provided {@link SSLContext} has the expected default configuration.
   */
  public static void assertSSLContext(SSLContext sslContext) throws IOException {
    assertDefaultSSLParametersClient(sslContext.getDefaultSSLParameters());
    assertSupportedSSLParametersClient(sslContext.getSupportedSSLParameters());
    assertSSLSocketFactory(sslContext.getSocketFactory());
    assertSSLServerSocketFactory(sslContext.getServerSocketFactory());
    assertSSLEngine(sslContext.createSSLEngine());
    assertSSLEngine(sslContext.createSSLEngine(null, -1));
  }

  /**
   * Asserts that the provided {@link SSLSocketFactory} has the expected default configuration.
   */
  public static void assertSSLSocketFactory(SSLSocketFactory sslSocketFactory) throws IOException {
    StandardNames.assertDefaultCipherSuites(sslSocketFactory.getDefaultCipherSuites());
    StandardNames.assertSupportedCipherSuites(sslSocketFactory.getSupportedCipherSuites());
    assertContainsAll("Unsupported default cipher suites",
        sslSocketFactory.getSupportedCipherSuites(),
        sslSocketFactory.getDefaultCipherSuites());

    assertSSLSocket((SSLSocket) sslSocketFactory.createSocket());
  }

  /**
   * Asserts that the provided {@link SSLServerSocketFactory} has the expected default
   * configuration.
   */
  public static void assertSSLServerSocketFactory(SSLServerSocketFactory sslServerSocketFactory)
      throws IOException {
    StandardNames.assertDefaultCipherSuites(sslServerSocketFactory.getDefaultCipherSuites());
    StandardNames.assertSupportedCipherSuites(sslServerSocketFactory.getSupportedCipherSuites());
    assertContainsAll("Unsupported default cipher suites",
        sslServerSocketFactory.getSupportedCipherSuites(),
        sslServerSocketFactory.getDefaultCipherSuites());

    assertSSLServerSocket((SSLServerSocket) sslServerSocketFactory.createServerSocket());
  }

  /**
   * Asserts that the provided {@link SSLSocket} has the expected default configuration.
   */
  public static void assertSSLSocket(SSLSocket sslSocket) {
    assertSSLParametersClient(sslSocket.getSSLParameters());

    StandardNames.assertDefaultCipherSuites(sslSocket.getEnabledCipherSuites());
    StandardNames.assertSupportedCipherSuites(sslSocket.getSupportedCipherSuites());
    assertContainsAll("Unsupported enabled cipher suites",
        sslSocket.getSupportedCipherSuites(),
        sslSocket.getEnabledCipherSuites());

    StandardNames.assertDefaultProtocolsClient(sslSocket.getEnabledProtocols());
    StandardNames.assertSupportedProtocols(sslSocket.getSupportedProtocols());
    assertContainsAll("Unsupported enabled protocols",
        sslSocket.getSupportedProtocols(),
        sslSocket.getEnabledProtocols());

    assertTrue(sslSocket.getUseClientMode());
    assertTrue(sslSocket.getEnableSessionCreation());
    assertFalse(sslSocket.getNeedClientAuth());
    assertFalse(sslSocket.getWantClientAuth());
  }

  /**
   * Asserts that the provided {@link SSLServerSocket} has the expected default configuration.
   */
  public static void assertSSLServerSocket(SSLServerSocket sslServerSocket) {
    // TODO: Check SSLParameters when supported by SSLServerSocket API

    StandardNames.assertDefaultCipherSuites(sslServerSocket.getEnabledCipherSuites());
    StandardNames.assertSupportedCipherSuites(sslServerSocket.getSupportedCipherSuites());
    assertContainsAll("Unsupported enabled cipher suites",
        sslServerSocket.getSupportedCipherSuites(),
        sslServerSocket.getEnabledCipherSuites());

    StandardNames.assertDefaultProtocolsServer(sslServerSocket.getEnabledProtocols());
    StandardNames.assertSupportedProtocols(sslServerSocket.getSupportedProtocols());
    assertContainsAll("Unsupported enabled protocols",
        sslServerSocket.getSupportedProtocols(),
        sslServerSocket.getEnabledProtocols());

    assertTrue(sslServerSocket.getEnableSessionCreation());
    assertFalse(sslServerSocket.getNeedClientAuth());
    assertFalse(sslServerSocket.getWantClientAuth());
  }

  /**
   * Asserts that the provided {@link SSLEngine} has the expected default configuration.
   */
  public static void assertSSLEngine(SSLEngine sslEngine) {
    assertFalse(sslEngine.getUseClientMode());
    assertSSLEngineSSLParameters(sslEngine.getSSLParameters());

    StandardNames.assertDefaultCipherSuites(sslEngine.getEnabledCipherSuites());
    StandardNames.assertSupportedCipherSuites(sslEngine.getSupportedCipherSuites());
    assertContainsAll("Unsupported enabled cipher suites",
        sslEngine.getSupportedCipherSuites(),
        sslEngine.getEnabledCipherSuites());

    StandardNames.assertSSLEngineDefaultProtocols(sslEngine.getEnabledProtocols());
    StandardNames.assertSupportedProtocols(sslEngine.getSupportedProtocols());
    assertContainsAll("Unsupported enabled protocols",
        sslEngine.getSupportedProtocols(),
        sslEngine.getEnabledProtocols());

    assertTrue(sslEngine.getEnableSessionCreation());
    assertFalse(sslEngine.getNeedClientAuth());
    assertFalse(sslEngine.getWantClientAuth());
  }

  /**
   * Asserts that the provided {@link SSLParameters} describe the expected default configuration
   * for client-side mode of operation.
   */
  public static void assertSSLParametersClient(SSLParameters sslParameters) {
    assertDefaultSSLParametersClient(sslParameters);
  }

  /**
   * Asserts that the provided default {@link SSLParameters} are as expected for client-side mode of
   * operation.
   */
  private static void assertDefaultSSLParametersClient(SSLParameters sslParameters) {
    StandardNames.assertDefaultCipherSuites(sslParameters.getCipherSuites());
    StandardNames.assertDefaultProtocolsClient(sslParameters.getProtocols());
    assertFalse(sslParameters.getWantClientAuth());
    assertFalse(sslParameters.getNeedClientAuth());
  }

  /**
   * Asserts that the provided supported {@link SSLParameters} are as expected for client-side mode
   * of operation.
   */
  private static void assertSupportedSSLParametersClient(SSLParameters sslParameters) {
    StandardNames.assertSupportedCipherSuites(sslParameters.getCipherSuites());
    StandardNames.assertSupportedProtocols(sslParameters.getProtocols());
    assertFalse(sslParameters.getWantClientAuth());
    assertFalse(sslParameters.getNeedClientAuth());
  }

  /**
   * Asserts that the provided {@link SSLParameters} has the expected default configuration for
   * {@link SSLEngine}.
   */
  public static void assertSSLEngineSSLParameters(SSLParameters sslParameters) {
    StandardNames.assertDefaultCipherSuites(sslParameters.getCipherSuites());
    StandardNames.assertSSLEngineDefaultProtocols(sslParameters.getProtocols());
    assertFalse(sslParameters.getWantClientAuth());
    assertFalse(sslParameters.getNeedClientAuth());
  }

  /**
   * Asserts that the {@code container} contains all the {@code elements}.
   */
  private static void assertContainsAll(String message, String[] container, String[] elements) {
    Set<String> elementsNotInContainer = new HashSet<String>(Arrays.asList(elements));
    elementsNotInContainer.removeAll(Arrays.asList(container));
    assertEquals(message, Collections.EMPTY_SET, elementsNotInContainer);
  }
}
