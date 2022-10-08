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

import java.io.File;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.lang.Process;
import java.lang.Runtime;
import java.security.PublicKey;
import java.security.PrivateKey;
import java.security.Security;
import java.security.cert.X509Certificate;
import org.bouncycastle.jce.provider.BouncyCastleProvider;

public class VerityVerifier {

    private static final int EXT4_SB_MAGIC = 0xEF53;
    private static final int EXT4_SB_OFFSET = 0x400;
    private static final int EXT4_SB_OFFSET_MAGIC = EXT4_SB_OFFSET + 0x38;
    private static final int EXT4_SB_OFFSET_LOG_BLOCK_SIZE = EXT4_SB_OFFSET + 0x18;
    private static final int EXT4_SB_OFFSET_BLOCKS_COUNT_LO = EXT4_SB_OFFSET + 0x4;
    private static final int EXT4_SB_OFFSET_BLOCKS_COUNT_HI = EXT4_SB_OFFSET + 0x150;
    private static final int VERITY_MAGIC = 0xB001B001;
    private static final int VERITY_SIGNATURE_SIZE = 256;
    private static final int VERITY_VERSION = 0;

    /**
     * Converts a 4-byte little endian value to a Java integer
     * @param value Little endian integer to convert
     */
     public static int fromle(int value) {
        byte[] bytes = ByteBuffer.allocate(4).putInt(value).array();
        return ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN).getInt();
    }

     /**
     * Converts a 2-byte little endian value to Java a integer
     * @param value Little endian short to convert
     */
     public static int fromle(short value) {
        return fromle(value << 16);
    }

    /**
     * Unsparses a sparse image into a temporary file and returns a
     * handle to the file
     * @param fname Path to a sparse image file
     */
     public static RandomAccessFile openImage(String fname) throws Exception {
        File tmp = File.createTempFile("system", ".raw");
        tmp.deleteOnExit();

        Process p = Runtime.getRuntime().exec("simg2img " + fname +
                            " " + tmp.getAbsoluteFile());

        p.waitFor();
        if (p.exitValue() != 0) {
            throw new IllegalArgumentException("Invalid image: failed to unsparse");
        }

        return new RandomAccessFile(tmp, "r");
    }

    /**
     * Reads the ext4 superblock and calculates the size of the system image,
     * after which we should find the verity metadata
     * @param img File handle to the image file
     */
    public static long getMetadataPosition(RandomAccessFile img)
            throws Exception {
        img.seek(EXT4_SB_OFFSET_MAGIC);
        int magic = fromle(img.readShort());

        if (magic != EXT4_SB_MAGIC) {
            throw new IllegalArgumentException("Invalid image: not a valid ext4 image");
        }

        img.seek(EXT4_SB_OFFSET_BLOCKS_COUNT_LO);
        long blocksCountLo = fromle(img.readInt());

        img.seek(EXT4_SB_OFFSET_LOG_BLOCK_SIZE);
        long logBlockSize = fromle(img.readInt());

        img.seek(EXT4_SB_OFFSET_BLOCKS_COUNT_HI);
        long blocksCountHi = fromle(img.readInt());

        long blockSizeBytes = 1L << (10 + logBlockSize);
        long blockCount = (blocksCountHi << 32) + blocksCountLo;
        return blockSizeBytes * blockCount;
    }

    /**
     * Reads and validates verity metadata, and check the signature against the
     * given public key
     * @param img File handle to the image file
     * @param key Public key to use for signature verification
     */
    public static boolean verifyMetaData(RandomAccessFile img, PublicKey key)
            throws Exception {
        img.seek(getMetadataPosition(img));
        int magic = fromle(img.readInt());

        if (magic != VERITY_MAGIC) {
            throw new IllegalArgumentException("Invalid image: verity metadata not found");
        }

        int version = fromle(img.readInt());

        if (version != VERITY_VERSION) {
            throw new IllegalArgumentException("Invalid image: unknown metadata version");
        }

        byte[] signature = new byte[VERITY_SIGNATURE_SIZE];
        img.readFully(signature);

        int tableSize = fromle(img.readInt());

        byte[] table = new byte[tableSize];
        img.readFully(table);

        return Utils.verify(key, table, signature,
                   Utils.getSignatureAlgorithmIdentifier(key));
    }

    public static void main(String[] args) throws Exception {
        if (args.length != 2) {
            System.err.println("Usage: VerityVerifier <sparse.img> <certificate.x509.pem>");
            System.exit(1);
        }

        Security.addProvider(new BouncyCastleProvider());

        X509Certificate cert = Utils.loadPEMCertificate(args[1]);
        PublicKey key = cert.getPublicKey();
        RandomAccessFile img = openImage(args[0]);

        try {
            if (verifyMetaData(img, key)) {
                System.err.println("Signature is VALID");
                System.exit(0);
            } else {
                System.err.println("Signature is INVALID");
            }
        } catch (Exception e) {
            e.printStackTrace(System.err);
        }

        System.exit(1);
    }
}
