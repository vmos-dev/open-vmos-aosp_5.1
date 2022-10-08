/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.verity;

import java.io.IOException;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.Security;
import java.security.Signature;
import java.security.cert.X509Certificate;
import java.util.Enumeration;
import org.bouncycastle.asn1.ASN1Encodable;
import org.bouncycastle.asn1.ASN1EncodableVector;
import org.bouncycastle.asn1.ASN1InputStream;
import org.bouncycastle.asn1.ASN1Integer;
import org.bouncycastle.asn1.ASN1Object;
import org.bouncycastle.asn1.ASN1Primitive;
import org.bouncycastle.asn1.ASN1Sequence;
import org.bouncycastle.asn1.DEROctetString;
import org.bouncycastle.asn1.DERPrintableString;
import org.bouncycastle.asn1.DERSequence;
import org.bouncycastle.asn1.pkcs.PKCSObjectIdentifiers;
import org.bouncycastle.asn1.pkcs.RSAPublicKey;
import org.bouncycastle.asn1.util.ASN1Dump;
import org.bouncycastle.asn1.x509.AlgorithmIdentifier;
import org.bouncycastle.jce.provider.BouncyCastleProvider;

/**
 * AndroidVerifiedBootKeystore DEFINITIONS ::=
 * BEGIN
 *     FormatVersion ::= INTEGER
 *     KeyBag ::= SEQUENCE {
 *         Key  ::= SEQUENCE {
 *             AlgorithmIdentifier  ::=  SEQUENCE {
 *                 algorithm OBJECT IDENTIFIER,
 *                 parameters ANY DEFINED BY algorithm OPTIONAL
 *             }
 *             KeyMaterial ::= RSAPublicKey
 *         }
 *     }
 *     Signature ::= AndroidVerifiedBootSignature
 * END
 */

class BootKey extends ASN1Object
{
    private AlgorithmIdentifier algorithmIdentifier;
    private RSAPublicKey keyMaterial;

    public BootKey(PublicKey key) throws Exception {
        java.security.interfaces.RSAPublicKey k =
                (java.security.interfaces.RSAPublicKey) key;
        this.keyMaterial = new RSAPublicKey(
                k.getModulus(),
                k.getPublicExponent());
        this.algorithmIdentifier = Utils.getSignatureAlgorithmIdentifier(key);
    }

    public ASN1Primitive toASN1Primitive() {
        ASN1EncodableVector v = new ASN1EncodableVector();
        v.add(algorithmIdentifier);
        v.add(keyMaterial);
        return new DERSequence(v);
    }

    public void dump() throws Exception {
        System.out.println(ASN1Dump.dumpAsString(toASN1Primitive()));
    }
}

class BootKeystore extends ASN1Object
{
    private ASN1Integer             formatVersion;
    private ASN1EncodableVector     keyBag;
    private BootSignature           signature;
    private X509Certificate         certificate;

    private static final int FORMAT_VERSION = 0;

    public BootKeystore() {
        this.formatVersion = new ASN1Integer(FORMAT_VERSION);
        this.keyBag = new ASN1EncodableVector();
    }

    public void addPublicKey(byte[] der) throws Exception {
        PublicKey pubkey = Utils.loadDERPublicKey(der);
        BootKey k = new BootKey(pubkey);
        keyBag.add(k);
    }

    public void setCertificate(X509Certificate cert) {
        certificate = cert;
    }

    public byte[] getInnerKeystore() throws Exception {
        ASN1EncodableVector v = new ASN1EncodableVector();
        v.add(formatVersion);
        v.add(new DERSequence(keyBag));
        return new DERSequence(v).getEncoded();
    }

    public ASN1Primitive toASN1Primitive() {
        ASN1EncodableVector v = new ASN1EncodableVector();
        v.add(formatVersion);
        v.add(new DERSequence(keyBag));
        v.add(signature);
        return new DERSequence(v);
    }

    public void parse(byte[] input) throws Exception {
        ASN1InputStream stream = new ASN1InputStream(input);
        ASN1Sequence sequence = (ASN1Sequence) stream.readObject();

        formatVersion = (ASN1Integer) sequence.getObjectAt(0);
        if (formatVersion.getValue().intValue() != FORMAT_VERSION) {
            throw new IllegalArgumentException("Unsupported format version");
        }

        ASN1Sequence keys = (ASN1Sequence) sequence.getObjectAt(1);
        Enumeration e = keys.getObjects();
        while (e.hasMoreElements()) {
            keyBag.add((ASN1Encodable) e.nextElement());
        }

        ASN1Object sig = sequence.getObjectAt(2).toASN1Primitive();
        signature = new BootSignature(sig.getEncoded());
    }

    public boolean verify() throws Exception {
        byte[] innerKeystore = getInnerKeystore();
        return Utils.verify(signature.getPublicKey(), innerKeystore,
                signature.getSignature(), signature.getAlgorithmIdentifier());
    }

    public void sign(PrivateKey privateKey) throws Exception {
        byte[] innerKeystore = getInnerKeystore();
        byte[] rawSignature = Utils.sign(privateKey, innerKeystore);
        signature = new BootSignature("keystore", innerKeystore.length);
        signature.setCertificate(certificate);
        signature.setSignature(rawSignature,
                Utils.getSignatureAlgorithmIdentifier(privateKey));
    }

    public void dump() throws Exception {
        System.out.println(ASN1Dump.dumpAsString(toASN1Primitive()));
    }

    private static void usage() {
        System.err.println("usage: KeystoreSigner <privatekey.pk8> " +
                "<certificate.x509.pem> <outfile> <publickey0.der> " +
                "... <publickeyN-1.der> | -verify <keystore>");
        System.exit(1);
    }

    public static void main(String[] args) throws Exception {
        if (args.length < 2) {
            usage();
            return;
        }

        Security.addProvider(new BouncyCastleProvider());
        BootKeystore ks = new BootKeystore();

        if ("-verify".equals(args[0])) {
            ks.parse(Utils.read(args[1]));

            try {
                if (ks.verify()) {
                    System.err.println("Signature is VALID");
                    System.exit(0);
                } else {
                    System.err.println("Signature is INVALID");
                }
            } catch (Exception e) {
                e.printStackTrace(System.err);
            }
            System.exit(1);
        } else {
            String privkeyFname = args[0];
            String certFname = args[1];
            String outfileFname = args[2];

            ks.setCertificate(Utils.loadPEMCertificate(certFname));

            for (int i = 3; i < args.length; i++) {
                ks.addPublicKey(Utils.read(args[i]));
            }

            ks.sign(Utils.loadDERPrivateKeyFromFile(privkeyFname));
            Utils.write(ks.getEncoded(), outfileFname);
        }
    }
}
