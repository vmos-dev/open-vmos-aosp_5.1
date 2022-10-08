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

package com.android.hierarchyviewer;

import com.android.ddmlib.IDevice;
import com.android.hierarchyviewer.device.Window;
import com.android.hierarchyviewer.scene.CaptureLoader;
import com.android.hierarchyviewer.ui.Workspace;
import com.android.hierarchyviewer.device.DeviceBridge;

import javax.swing.SwingUtilities;
import javax.swing.UIManager;
import javax.swing.UnsupportedLookAndFeelException;
import java.io.File;

public class HierarchyViewer {
    private static final CharSequence OS_WINDOWS = "Windows";
    private static final CharSequence OS_MACOSX = "Mac OS X";

    private static boolean sProfilingEnabled = true;

    public static boolean isProfilingEnabled() {
        return sProfilingEnabled;
    }

    private static void initUserInterface() {
        System.setProperty("apple.laf.useScreenMenuBar", "true");
        System.setProperty("apple.awt.brushMetalLook", "true");
        System.setProperty("com.apple.mrj.application.apple.menu.about.name", "HierarchyViewer");

        final String os = System.getProperty("os.name");

        try {
            if (os.contains(OS_WINDOWS) || os.contains(OS_MACOSX)) {
                UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
            } else {
                UIManager.setLookAndFeel(UIManager.getCrossPlatformLookAndFeelClassName());
            }
        } catch (ClassNotFoundException e) {
            e.printStackTrace();
        } catch (InstantiationException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (UnsupportedLookAndFeelException e) {
            e.printStackTrace();
        }
    }

    private static void listDevices() {
        System.out.println("List of devices attached");
        try {
            Thread.sleep(500);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        for (IDevice device : DeviceBridge.getDevices()) {
            printDevice(device);
        }
        DeviceBridge.terminate();
    }

    private static void printDevice(IDevice device) {
        System.out.println(device.toString() + "\t\t" +
                (device.isEmulator() ? "emulator" : "device"));
    }

    private static void outputPsd(String deviceName, String file) {
        IDevice device = selectDevice(deviceName);
        if (device != null) {
            if (DeviceBridge.isViewServerRunning(device)) {
                DeviceBridge.stopViewServer(device);
            }
            DeviceBridge.startViewServer(device);
            DeviceBridge.setupDeviceForward(device);
            System.out.println("Capturing layers to " + file);
            CaptureLoader.saveLayers(device, Window.FOCUSED_WINDOW, new File(file));
        } else {
            System.out.println("The selected device does not exist");
        }
        DeviceBridge.terminate();
    }

    private static IDevice selectDevice(String deviceName) {
        try {
            Thread.sleep(500);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        if (DeviceBridge.getDevices() == null) return null;
        if (deviceName == null) return DeviceBridge.getDevices()[0];
        for (IDevice device : DeviceBridge.getDevices()) {
            if (device.getSerialNumber().equalsIgnoreCase(deviceName)) {
                return device;
            }
        }
        return null;
    }

    public static void main(String[] args) {
        DeviceBridge.initDebugBridge();

        if (args.length > 0) {
            for (int i = 0; i < args.length; i++) {
                String arg = args[i];
                if ("--help".equalsIgnoreCase(arg)) {
                    System.out.println("Usage: hierarchyviewer1 [options]\n");
                    System.out.println("Options:");
                    System.out.println("  --help\t\t\t Show this help message and exit");
                    System.out.println("  --no-profiling\t Disable views profiling");
                    System.out.println("  --devices\t\t\t Show the list of available devices");
                    System.out.println("  --psd [device] <file>\t Export psd and exit");
                    System.exit(0);
                } else if ("--no-profiling".equalsIgnoreCase(arg)) {
                    sProfilingEnabled = false;
                } else if ("--devices".equalsIgnoreCase(arg)) {
                    listDevices();
                    System.exit(0);
                } else if ("--psd".equalsIgnoreCase(arg)) {
                    if (i == args.length - 1) {
                        System.out.println("You must specify at least an output file with --psd");
                        System.exit(1);
                    }
                    String device = null;
                    String file = null;
                    if (i < args.length - 2) {
                        device = args[++i];
                    }
                    if (i < args.length - 1) {
                        file = args[++i];
                    }
                    outputPsd(device, file);
                    System.exit(0);
                }
            }
        }

        initUserInterface();
        SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                Workspace workspace = new Workspace();
                workspace.setDefaultCloseOperation(Workspace.EXIT_ON_CLOSE);
                workspace.setLocationRelativeTo(null);
                workspace.setVisible(true);
            }
        });
    }
}
