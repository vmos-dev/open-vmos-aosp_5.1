/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package libcore.net.url;

import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FilePermission;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintStream;
import java.net.URL;
import java.net.URLConnection;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import libcore.net.UriCodec;

/**
 * This subclass extends <code>URLConnection</code>.
 * <p>
 * This class is responsible for connecting, getting content and input stream of
 * the file.
 */
public class FileURLConnection extends URLConnection {

    private static final Comparator<String> HEADER_COMPARATOR = new Comparator<String>() {
        @Override
        public int compare(String a, String b) {
            if (a == b) {
                return 0;
            } else if (a == null) {
                return -1;
            } else if (b == null) {
                return 1;
            } else {
                return String.CASE_INSENSITIVE_ORDER.compare(a, b);
            }
        }
    };

    private String filename;

    private InputStream is;

    private long length = -1;

    private long lastModified = -1;

    private boolean isDir;

    private FilePermission permission;

    /**
     * A set of three key value pairs representing the headers we support.
     */
    private final String[] headerKeysAndValues;

    private static final int CONTENT_TYPE_VALUE_IDX = 1;
    private static final int CONTENT_LENGTH_VALUE_IDX = 3;
    private static final int LAST_MODIFIED_VALUE_IDX = 5;

    private Map<String, List<String>> headerFields;

    /**
     * Creates an instance of <code>FileURLConnection</code> for establishing
     * a connection to the file pointed by this <code>URL<code>
     *
     * @param url The URL this connection is connected to
     */
    public FileURLConnection(URL url) {
        super(url);
        filename = url.getFile();
        if (filename == null) {
            filename = "";
        }
        filename = UriCodec.decode(filename);
        headerKeysAndValues = new String[] {
                "content-type", null,
                "content-length", null,
                "last-modified", null };
    }

    /**
     * This methods will attempt to obtain the input stream of the file pointed
     * by this <code>URL</code>. If the file is a directory, it will return
     * that directory listing as an input stream.
     *
     * @throws IOException
     *             if an IO error occurs while connecting
     */
    @Override
    public void connect() throws IOException {
        File f = new File(filename);
        IOException error = null;
        if (f.isDirectory()) {
            isDir = true;
            is = getDirectoryListing(f);
            // use -1 for the contentLength
            lastModified = f.lastModified();
            headerKeysAndValues[CONTENT_TYPE_VALUE_IDX] = "text/html";
        } else {
            try {
                is = new BufferedInputStream(new FileInputStream(f));
            } catch (IOException ioe) {
                error = ioe;
            }

            if (error == null) {
                length = f.length();
                lastModified = f.lastModified();
                headerKeysAndValues[CONTENT_TYPE_VALUE_IDX] = getContentTypeForPlainFiles();
            } else {
                headerKeysAndValues[CONTENT_TYPE_VALUE_IDX] = "content/unknown";
            }
        }

        headerKeysAndValues[CONTENT_LENGTH_VALUE_IDX] = String.valueOf(length);
        headerKeysAndValues[LAST_MODIFIED_VALUE_IDX] = String.valueOf(lastModified);

        connected = true;
        if (error != null) {
            throw error;
        }
    }

    @Override
    public String getHeaderField(String key) {
        if (!connected) {
            try {
                connect();
            } catch (IOException ioe) {
                return null;
            }
        }

        for (int i = 0; i < headerKeysAndValues.length; i += 2) {
            if (headerKeysAndValues[i].equalsIgnoreCase(key)) {
                return headerKeysAndValues[i + 1];
            }
        }

        return null;
    }

    @Override
    public String getHeaderFieldKey(int position) {
        if (!connected) {
            try {
                connect();
            } catch (IOException ioe) {
                return null;
            }
        }

        if (position < 0 || position > headerKeysAndValues.length / 2) {
            return null;
        }

        return headerKeysAndValues[position * 2];
    }

    @Override
    public String getHeaderField(int position) {
        if (!connected) {
            try {
                connect();
            } catch (IOException ioe) {
                return null;
            }
        }

        if (position < 0 || position > headerKeysAndValues.length / 2) {
            return null;
        }

        return headerKeysAndValues[(position * 2) + 1];
    }

    @Override
    public Map<String, List<String>> getHeaderFields() {
        if (headerFields == null) {
            final TreeMap<String, List<String>> headerFieldsMap = new TreeMap<>(HEADER_COMPARATOR);

            for (int i = 0; i < headerKeysAndValues.length; i+=2) {
                headerFieldsMap.put(headerKeysAndValues[i],
                        Collections.singletonList(headerKeysAndValues[i + 1]));
            }

            headerFields = Collections.unmodifiableMap(headerFieldsMap);
        }

        return headerFields;
    }

    /**
     * Returns the length of the file in bytes, or {@code -1} if the length cannot be
     * represented as an {@code int}. See {@link #getContentLengthLong()} for a method that can
     * handle larger files.
     */
    @Override
    public int getContentLength() {
        long length = getContentLengthLong();
        return length <= Integer.MAX_VALUE ? (int) length : -1;
    }

    /**
     * Returns the length of the file in bytes.
     */
    private long getContentLengthLong() {
        try {
            if (!connected) {
                connect();
            }
        } catch (IOException e) {
            // default is -1
        }
        return length;
    }

    /**
     * Returns the content type of the resource. Just takes a guess based on the
     * name.
     *
     * @return the content type
     */
    @Override
    public String getContentType() {
        // The content-type header field is always at position 0.
        return getHeaderField(0);
    }

    private String getContentTypeForPlainFiles() {
        String result = guessContentTypeFromName(url.getFile());
        if (result != null) {
            return result;
        }

        try {
            result = guessContentTypeFromStream(is);
        } catch (IOException e) {
            // Ignore
        }
        if (result != null) {
            return result;
        }

        return "content/unknown";
    }

    /**
     * Returns the directory listing of the file component as an input stream.
     *
     * @return the input stream of the directory listing
     */
    private InputStream getDirectoryListing(File f) {
        String fileList[] = f.list();
        ByteArrayOutputStream bytes = new java.io.ByteArrayOutputStream();
        PrintStream out = new PrintStream(bytes);
        out.print("<title>Directory Listing</title>\n");
        out.print("<base href=\"file:");
        out.print(f.getPath().replace('\\', '/') + "/\"><h1>" + f.getPath()
                + "</h1>\n<hr>\n");
        int i;
        for (i = 0; i < fileList.length; i++) {
            out.print(fileList[i] + "<br>\n");
        }
        out.close();
        return new ByteArrayInputStream(bytes.toByteArray());
    }

    /**
     * Returns the input stream of the object referred to by this
     * <code>URLConnection</code>
     *
     * File Sample : "/ZIP211/+/harmony/tools/javac/resources/javac.properties"
     * Invalid File Sample:
     * "/ZIP/+/harmony/tools/javac/resources/javac.properties"
     * "ZIP211/+/harmony/tools/javac/resources/javac.properties"
     *
     * @return input stream of the object
     *
     * @throws IOException
     *             if an IO error occurs
     */
    @Override
    public InputStream getInputStream() throws IOException {
        if (!connected) {
            connect();
        }
        return is;
    }

    /**
     * Returns the permission, in this case the subclass, FilePermission object
     * which represents the permission necessary for this URLConnection to
     * establish the connection.
     *
     * @return the permission required for this URLConnection.
     *
     * @throws IOException
     *             if an IO exception occurs while creating the permission.
     */
    @Override
    public java.security.Permission getPermission() throws IOException {
        if (permission == null) {
            String path = filename;
            if (File.separatorChar != '/') {
                path = path.replace('/', File.separatorChar);
            }
            permission = new FilePermission(path, "read");
        }
        return permission;
    }
}
