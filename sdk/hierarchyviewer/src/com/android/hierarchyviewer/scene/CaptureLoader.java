/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.android.hierarchyviewer.scene;

import com.android.ddmlib.IDevice;
import com.android.hierarchyviewer.device.Window;
import com.android.hierarchyviewer.device.DeviceBridge;
import com.android.hierarchyviewer.ui.util.PsdFile;

import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.Point;
import java.awt.image.BufferedImage;
import java.io.BufferedInputStream;
import java.io.BufferedWriter;
import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.net.InetSocketAddress;
import java.net.Socket;
import javax.imageio.ImageIO;

public class CaptureLoader {
    public static boolean saveLayers(IDevice device, Window window, File file) {
        Socket socket = null;
        DataInputStream in = null;
        BufferedWriter out = null;
        boolean result = false;

        try {
            socket = new Socket();
            socket.connect(new InetSocketAddress("127.0.0.1",
                    DeviceBridge.getDeviceLocalPort(device)));

            out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
            in = new DataInputStream(new BufferedInputStream(socket.getInputStream()));

            out.write("CAPTURE_LAYERS " + window.encode());
            out.newLine();
            out.flush();

            int width = in.readInt();
            int height = in.readInt();

            PsdFile psd = new PsdFile(width, height);

            while (readLayer(in, psd)) {
            }
            
            psd.write(new FileOutputStream(file));

            result = true;
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (out != null) {
                    out.close();
                }
                if (in != null) {
                    in.close();
                }
                if (socket != null) {
                    socket.close();
                }
            } catch (IOException ex) {
                ex.printStackTrace();
            }
        }

        return result;
    }

    private static boolean readLayer(DataInputStream in, PsdFile psd) {
        try {
            if (in.read() == 2) {
                System.out.println("Found end of layers list");
                return false;
            }
            String name = in.readUTF();
            System.out.println("name = " + name);
            boolean visible = in.read() == 1;
            int x = in.readInt();
            int y = in.readInt();
            int dataSize = in.readInt();

            byte[] data = new byte[dataSize];
            int read = 0;
            while (read < dataSize) {
                read += in.read(data, read, dataSize - read);
            }

            ByteArrayInputStream arrayIn = new ByteArrayInputStream(data);
            BufferedImage chunk = ImageIO.read(arrayIn);

            // Ensure the image is in the right format
            BufferedImage image = new BufferedImage(chunk.getWidth(), chunk.getHeight(),
                    BufferedImage.TYPE_INT_ARGB);
            Graphics2D g = image.createGraphics();
            g.drawImage(chunk, null, 0, 0);
            g.dispose();

            psd.addLayer(name, image, new Point(x, y), visible);

            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public static Image loadCapture(IDevice device, Window window, String params) {
        Socket socket = null;
        BufferedInputStream in = null;
        BufferedWriter out = null;

        try {
            socket = new Socket();
            socket.connect(new InetSocketAddress("127.0.0.1",
                    DeviceBridge.getDeviceLocalPort(device)));

            out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream()));
            in = new BufferedInputStream(socket.getInputStream());

            out.write("CAPTURE " + window.encode() + " " + params);
            out.newLine();
            out.flush();

            return ImageIO.read(in);
        } catch (IOException e) {
            // Empty
        } finally {
            try {
                if (out != null) {
                    out.close();
                }
                if (in != null) {
                    in.close();
                }
                if (socket != null) {
                    socket.close();
                }
            } catch (IOException ex) {
                ex.printStackTrace();
            }
        }

        return null;
    }
}
