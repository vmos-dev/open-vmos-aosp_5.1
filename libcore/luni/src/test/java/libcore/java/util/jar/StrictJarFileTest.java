/*
 * Copyright (C) 2013 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package libcore.java.util.jar;

import junit.framework.TestCase;
import tests.support.resource.Support_Resources;
import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.HashMap;
import java.util.Iterator;
import java.util.jar.StrictJarFile;
import java.util.zip.ZipEntry;
import libcore.io.Streams;

public class StrictJarFileTest extends TestCase {

    // A well formed jar file with 6 entries.
    private static final String JAR_1 = "hyts_patch.jar";

    private File resources;

    @Override
    protected void setUp() {
        resources = Support_Resources.createTempFolder();
    }

    public void testConstructor() throws Exception {
        try {
            StrictJarFile jarFile = new StrictJarFile("Wrong.file");
            fail("Should throw IOException");
        } catch (IOException e) {
            // expected
        }

        Support_Resources.copyFile(resources, null, JAR_1);
        String fileName = (new File(resources, JAR_1)).getCanonicalPath();
        StrictJarFile jarFile = new StrictJarFile(fileName);
        jarFile.close();
    }

    public void testIteration() throws Exception {
        Support_Resources.copyFile(resources, null, JAR_1);
        StrictJarFile jarFile = new StrictJarFile(new File(resources, JAR_1).getAbsolutePath());

        Iterator<ZipEntry> it = jarFile.iterator();
        HashMap<String, ZipEntry> entries = new HashMap<String, ZipEntry>();
        while (it.hasNext()) {
            final ZipEntry ze = it.next();
            entries.put(ze.getName(), ze);
        }

        assertEquals(6, entries.size());
        assertTrue(entries.containsKey("META-INF/"));

        assertTrue(entries.containsKey("META-INF/MANIFEST.MF"));
        ZipEntry ze = entries.get("META-INF/MANIFEST.MF");
        assertEquals(62, ze.getSize());
        assertEquals(ZipEntry.DEFLATED, ze.getMethod());
        assertEquals(61, ze.getCompressedSize());

        assertTrue(entries.containsKey("Blah.txt"));
        ze = entries.get("Blah.txt");
        assertEquals(4, ze.getSize());
        assertEquals(ZipEntry.DEFLATED, ze.getMethod());
        assertEquals(6, ze.getCompressedSize());
        assertEquals("Blah", new String(Streams.readFully(jarFile.getInputStream(ze)),
                Charset.forName("UTF-8")));

        assertTrue(entries.containsKey("foo/"));
        assertTrue(entries.containsKey("foo/bar/"));
        assertTrue(entries.containsKey("foo/bar/A.class"));
        ze = entries.get("foo/bar/A.class");
        assertEquals(311, ze.getSize());
        assertEquals(ZipEntry.DEFLATED, ze.getMethod());
        assertEquals(225, ze.getCompressedSize());
    }

    public void testFindEntry() throws Exception {
        Support_Resources.copyFile(resources, null, JAR_1);
        StrictJarFile jarFile = new StrictJarFile(new File(resources, JAR_1).getAbsolutePath());

        assertNull(jarFile.findEntry("foobar"));
        assertNull(jarFile.findEntry("blah.txt"));
        assertNotNull(jarFile.findEntry("Blah.txt"));
        final ZipEntry ze = jarFile.findEntry("Blah.txt");
        assertEquals(4, ze.getSize());
        assertEquals(ZipEntry.DEFLATED, ze.getMethod());
        assertEquals(6, ze.getCompressedSize());
        assertEquals("Blah", new String(Streams.readFully(jarFile.getInputStream(ze)),
                Charset.forName("UTF-8")));
    }

    public void testGetManifest() throws Exception {
        Support_Resources.copyFile(resources, null, JAR_1);
        StrictJarFile jarFile = new StrictJarFile(new File(resources, JAR_1).getAbsolutePath());

        assertNotNull(jarFile.getManifest());
        assertEquals("1.4.2 (IBM Corporation)", jarFile.getManifest().getMainAttributes().getValue("Created-By"));
    }

    public void testJarSigning_wellFormed() throws IOException {
        Support_Resources.copyFile(resources, null, "Integrate.jar");
        StrictJarFile jarFile = new StrictJarFile(new File(resources, "Integrate.jar").getAbsolutePath());
        Iterator<ZipEntry> entries = jarFile.iterator();
        while (entries.hasNext()) {
            ZipEntry zipEntry = entries.next();
            jarFile.getInputStream(zipEntry).skip(Long.MAX_VALUE);
            if ("Test.class".equals(zipEntry.getName())) {
                assertNotNull(jarFile.getCertificates(zipEntry));
                assertNotNull(jarFile.getCertificateChains(zipEntry));
            }
        }
    }

     public void testJarSigning_fudgedEntry() throws IOException {
        Support_Resources.copyFile(resources, null, "Integrate.jar");
        StrictJarFile jarFile = new StrictJarFile(
                new File(resources, "Integrate.jar").getAbsolutePath());

        ZipEntry ze = jarFile.findEntry("Test.class");
        jarFile.getInputStream(ze).skip(Long.MAX_VALUE);

        // Fudge the size so that certificates do not match.
        ze.setSize(ze.getSize() - 1);
        try {
            jarFile.getInputStream(ze).skip(Long.MAX_VALUE);
            fail();
        } catch (SecurityException expected) {
        }
    }

    public void testJarSigning_modifiedClass() throws IOException {
        Support_Resources.copyFile(resources, null,  "Modified_Class.jar");
        StrictJarFile jarFile = new StrictJarFile(
                new File(resources,  "Modified_Class.jar").getAbsolutePath());

        ZipEntry ze = jarFile.findEntry("Test.class");
        try {
            jarFile.getInputStream(ze).skip(Long.MAX_VALUE);
            fail();
        } catch (SecurityException expected) {
        }
    }

    public void testJarSigning_brokenMainAttributes() throws Exception {
        assertThrowsOnInit("Modified_Manifest_MainAttributes.jar");
    }

    public void testJarSigning_brokenEntryAttributes() throws Exception {
        assertThrowsOnInit("Modified_Manifest_EntryAttributes.jar");
    }

    public void testJarSigning_brokenSignatureFile() throws Exception {
        assertThrowsOnInit("Modified_SF_EntryAttributes.jar");
    }

    private void assertThrowsOnInit(String name) throws Exception {
        Support_Resources.copyFile(resources, null,  name);
        try {
            StrictJarFile jarFile = new StrictJarFile(
                    new File(resources,  name).getAbsolutePath());
            fail();
        } catch (SecurityException expected) {
        }
    }
}
