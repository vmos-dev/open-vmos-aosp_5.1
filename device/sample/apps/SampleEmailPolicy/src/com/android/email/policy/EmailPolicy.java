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

package com.android.email.policy;

import android.os.Bundle;

/**
 * This sample is the framework that can be used to build EmailPolicy packages for inclusion
 * on specific devices.  This sample is intended for use by OEMs or other creators of system
 * images.
 *
 * When this package is built and included in a system image, the Email application will detect
 * it (using reflection) and will make calls to the getPolicy() method at certain times.  This
 * can be used to provide local customization of the Email application.
 *
 * Do not change the package, class name, or method name - these must match the names used in
 * this sample code or the Email application will not find the helper.
 *
 * Three customization points are provided:
 *
 *   * Alternate strings for Exchange/ActiveSync UI
 *   * Insertion of device-specific text into IMAP ID commands
 *   * Device- or Carrier- specific account presets
 *
 * Each policy request may contain one or more parameters;  These are supplied as keyed entries
 * in the "arguments" bundle.  If there is a single argument, it will typically use the same key
 * as the policy name.  If there are multiple arguments, they keys are provided and called out.
 *
 * In all cases, getPolicy() should return a bundle.  For default behavior, or for any unknown
 * policy, simply return Bundle.EMPTY.
 *
 * To return actual data, create a new bundle and place result values in it.  If there is a single
 * return value, this value is placed in the return bundle using the same key as the request.
 * If there are multiple return values, keys will be provided for them as well.
 *
 * Future versions of the Email application may access additional customization points.  If
 * the call to getPolicy() is made with an unknown or unexpected policy keys, or the expected
 * argument values cannot be found, the method should Bundle.EMPTY.
 */
public class EmailPolicy {

    /**
     * This policy request configures the UI to conform to various Exchange/ActiveSync
     * license requirements.  In the default configuration, the UI will refer to this protocol as
     * "Exchange", while in the alternate configuration, the UI will refer to it as
     * "Exchange ActiveSync" or "Microsoft Exchange ActiveSync".
     *
     * For the default behavior, return the empty bundle.
     * For the alternate behavior, return a bundle containing a single entry with a key of
     * USE_ALTERNATE_EXCHANGE_STRINGS and a value of "true".
     */
    private static final String USE_ALTERNATE_EXCHANGE_STRINGS = "useAlternateExchangeStrings";

    /**
     * This policy request allows you to insert field/value pairs into the IMAP ID command, which
     * is sent to an IMAP server on each connection.
     *
     * The following arguments are provided:
     *   * GET_IMAP_ID_USER - the userid of the account being connected to
     *   * GET_IMAP_ID_HOST - the hostname of the server being connected to
     *   * GET_IMAP_ID_CAPA - the values returned by the "CAPA" command
     *
     * For default behavior (no values inserted), return the empty bundle.
     * To insert additional values into the IMAP ID command, return a bundle containing a single
     * entry with a key of GET_IMAP_ID and a String value.  The value must be field/value pairs
     * surrounded by quotes and separated by spaces.  Multiple field/value pairs may be provided.
     * See RFC 2971 for more information.
     */
    private static final String GET_IMAP_ID = "getImapId";
    private static final String GET_IMAP_ID_USER = "getImapId.user";
    private static final String GET_IMAP_ID_HOST = "getImapId.host";
    private static final String GET_IMAP_ID_CAPA = "getImapId.capabilities";

    /**
     * This policy request allows you to supply preset server configurations to provide
     * automatic setup/configuration for specific email providers.  These values supplement (or
     * override) the automatic configurations provided in res/xml/providers.xml in
     * the Email sources.  (See that file for more information and plenty of samples.)
     *
     * The only argument (with the key FIND_PROVIDER) is a string containing the domain that the
     * user entered as part of their email address;  For example, if the user enters
     * "MyEmailAddress@gmail.com", the domain will be "gmail.com".
     *
     * If no server information is provided for this domain, simply return Bundle.EMPTY.
     * If server information is available for this domain, it can be returned in the following
     * values:
     *   * FIND_PROVIDER_IN_URI The server configuration for the incoming (IMAP or POP) server
     *   * FIND_PROVIDER_IN_USER Format of the username (login) value
     *   * FIND_PROVIDER_OUT_URI The server configuration for the outgoing (SMTP) server
     *   * FIND_PROVIDER_OUT_USER Format of the username (login) value
     *
     * Valid incoming uri schemes are:
     *     imap        IMAP with no transport security.
     *     imap+tls+   IMAP with required TLS transport security.
     *                     If TLS is not available the connection fails.
     *     imap+ssl+   IMAP with required SSL transport security.
     *                     If SSL is not available the connection fails.
     *
     *     pop3        POP3 with no transport security.
     *     pop3+tls+   POP3 with required TLS transport security.
     *                     If TLS is not available the connection fails.
     *     pop3+ssl+   POP3 with required SSL transport security.
     *                     If SSL is not available the connection fails.
     *
     * Valid outgoing uri schemes are:
     *     smtp        SMTP with no transport security.
     *     smtp+tls+   SMTP with required TLS transport security.
     *                     If TLS is not available the connection fails.
     *     smtp+ssl+   SMTP with required SSL transport security.
     *                     If SSL is not available the connection fails.
     *
     * To the above schemes you may also add "trustallcerts" to indicate that,
     * although link encryption is still required, "non-trusted" certificates may
     * will be excepted.  For example, "imap+ssl+trustallcerts" or
     * "smtp+tls+trustallcerts".  This should only used when necessary, as it
     * could allow a spoofed server to intercept password and mail.
     *
     * The URIs should be full templates for connection, including a port if
     * the service uses a non-default port.  The default ports are as follows:
     *     imap        143     pop3        110     smtp        587
     *     imap+tls+   143     pop3+tls+   110     smtp+tls+   587
     *     imap+ssl+   993     pop3+ssl+   995     smtp+ssl+   465
     *
     * The username attribute is used to supply a template for the username
     * that will be presented to the server. This username is built from a
     * set of variables that are substituted with parts of the user
     * specified email address.
     *
     * Valid substitution values for the username attribute are:
     *     $email - the email address the user entered
     *     $user - the value before the @ sign in the email address the user entered
     *     $domain - the value after the @ signin the email address the user entered
     *
     * The username attribute MUST be specified for the incoming element, so the POP3 or IMAP
     * server can identify the mailbox to be opened.
     *
     * The username attribute MAY be the empty string for the outgoing element, but only if the
     * SMTP server supports anonymous transmission (most don't).
     *
     * For more information about these values, and many examples, see res/xml/providers.xml in
     * the Email sources.
     */
    private static final String FIND_PROVIDER = "findProvider";
    private static final String FIND_PROVIDER_IN_URI = "findProvider.inUri";
    private static final String FIND_PROVIDER_IN_USER = "findProvider.inUser";
    private static final String FIND_PROVIDER_OUT_URI = "findProvider.outUri";
    private static final String FIND_PROVIDER_OUT_USER = "findProvider.outUser";

    /**
     * The following data is simply examples, and would be changed (or removed) based on the
     * requirements for your device.
     */

    /**
     * Sample: Email domains that will be auto-configured.  In our example, a number of domains
     * are controlled by a single mail server.
     */
    private static final String[] KNOWN_DOMAINS = new String[] {
        "physics.school.edu", "math.school.edu", "language.school.edu", "history.school.edu"
    };

    /**
     * Sample: When we see a particular capability (identifying a particular server), send
     * back a special value in the IMAP ID command.
     */
    private static final String MY_SERVER_CAPABILITY = "MY-SERVER-CAPABILITY";
    private static final String MY_DEVICE_ID = "\"DEVICE-ID-FIELD\" \"MY-DEVICE-ID-VALUE\"";

    /**
     * Entry point from the Email application.
     *
     * @param policy A string requesting a particular policy
     * @param arguments A bundle containing zero or more argument values for the requested policy
     * @return A bundle containing zero or more return values for the requested policy
     */
    public static Bundle getPolicy(String policy, Bundle arguments) {
        /*
         * Policy: Use alternate exchange strings
         */
        if (USE_ALTERNATE_EXCHANGE_STRINGS.equals(policy)) {
            // Un-comment the following code to select alternate exchange strings
            // Bundle alternates = new Bundle();
            // alternates.putBoolean(USE_ALTERNATE_EXCHANGE_STRINGS, true);
            // return alternates;
        }

        /*
         * Policy: For a known domain, configure to the servers for that domain
         */
        if (FIND_PROVIDER.equals(policy)) {
            String domain = arguments.getString(FIND_PROVIDER);
            if (domain != null) {
                domain = domain.toLowerCase();
                boolean isKnownDomain = false;
                for (String knownDomain : KNOWN_DOMAINS) {
                    if (knownDomain.equals(domain)) {
                        isKnownDomain = true;
                        break;
                    }
                }
                if (isKnownDomain) {
                    Bundle b = new Bundle();
                    b.putString(FIND_PROVIDER_IN_URI, "imap+ssl://imap.school.edu");
                    b.putString(FIND_PROVIDER_IN_USER, "$email");
                    b.putString(FIND_PROVIDER_OUT_URI, "smtp+ssl://smtp.school.edu");
                    b.putString(FIND_PROVIDER_OUT_USER, "$email");
                    return b;
                }
            }
        }

        /**
         * Policy:  If the IMAP server presents a particular capability, send back a particular
         * identifier in the IMAP ID.
         */
        if (GET_IMAP_ID.equals(policy)) {
            String capabilities = arguments.getString(GET_IMAP_ID_CAPA);
            if (capabilities != null) {
                if (capabilities.toUpperCase().contains(MY_SERVER_CAPABILITY)) {
                    Bundle b = new Bundle();
                    b.putString(GET_IMAP_ID, MY_DEVICE_ID);
                    return b;
                }
            }
        }

        /**
         * For any other policy request, or any policy request that cannot be processed,
         * return an empty bundle.
         */
        return Bundle.EMPTY;
    }
}
