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

package  com.android.hierarchyviewer.ui.util;

import java.awt.Graphics2D;
import java.awt.Point;
import java.awt.image.BufferedImage;
import java.io.BufferedOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;

/**
 * Writes PSD file.
 * 
 * Supports only 8 bits, RGB images with 4 channels.
 */
public class PsdFile {
    private final Header mHeader;
    private final ColorMode mColorMode;
    private final ImageResources mImageResources;
    private final LayersMasksInfo mLayersMasksInfo;
    private final LayersInfo mLayersInfo;

    private final BufferedImage mMergedImage;
    private final Graphics2D mGraphics;

    public PsdFile(int width, int height) {
        mHeader = new Header(width, height);
        mColorMode = new ColorMode();
        mImageResources = new ImageResources();
        mLayersMasksInfo = new LayersMasksInfo();
        mLayersInfo = new LayersInfo();

        mMergedImage = new BufferedImage(width, height, BufferedImage.TYPE_INT_ARGB);
        mGraphics = mMergedImage.createGraphics();
    }

    public void addLayer(String name, BufferedImage image, Point offset) {
        addLayer(name, image, offset, true);
    }
    
    public void addLayer(String name, BufferedImage image, Point offset, boolean visible) {
        mLayersInfo.addLayer(name, image, offset, visible);
        if (visible) mGraphics.drawImage(image, null, offset.x, offset.y);
    }
    
    public void write(OutputStream stream) {
        mLayersMasksInfo.setLayersInfo(mLayersInfo);

        DataOutputStream out = new DataOutputStream(new BufferedOutputStream(stream));
        try {
            mHeader.write(out);
            out.flush();

            mColorMode.write(out);
            mImageResources.write(out);
            mLayersMasksInfo.write(out);
            mLayersInfo.write(out);
            out.flush();

            mLayersInfo.writeImageData(out);
            out.flush();
            
            writeImage(mMergedImage, out, false);
            out.flush();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                out.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    private static void writeImage(BufferedImage image, DataOutputStream out, boolean split)
            throws IOException {

        if (!split) out.writeShort(0);
        
        int width = image.getWidth();
        int height = image.getHeight();

        final int length = width * height;
        int[] pixels = new int[length];

        image.getData().getDataElements(0, 0, width, height, pixels);

        byte[] a = new byte[length];
        byte[] r = new byte[length];
        byte[] g = new byte[length];
        byte[] b = new byte[length];

        for (int i = 0; i < length; i++) {
            final int pixel = pixels[i];
            a[i] = (byte) ((pixel >> 24) & 0xFF);
            r[i] = (byte) ((pixel >> 16) & 0xFF);
            g[i] = (byte) ((pixel >>  8) & 0xFF);
            b[i] = (byte)  (pixel        & 0xFF);
        }

        if (split) out.writeShort(0);
        if (split) out.write(a);
        if (split) out.writeShort(0);
        out.write(r);
        if (split) out.writeShort(0);
        out.write(g);
        if (split) out.writeShort(0);
        out.write(b);
        if (!split) out.write(a);
    }

    @SuppressWarnings({"UnusedDeclaration"})
    static class Header {
        static final short MODE_BITMAP = 0;
        static final short MODE_GRAYSCALE = 1;
        static final short MODE_INDEXED = 2;
        static final short MODE_RGB = 3;
        static final short MODE_CMYK = 4;
        static final short MODE_MULTI_CHANNEL = 7;
        static final short MODE_DUOTONE = 8;
        static final short MODE_LAB = 9;

        final byte[] mSignature = "8BPS".getBytes();
        final short mVersion = 1;
        final byte[] mReserved = new byte[6];
        final short mChannelCount = 4;
        final int mHeight;
        final int mWidth;
        final short mDepth = 8;
        final short mMode = MODE_RGB;
        
        Header(int width, int height) {
            mWidth = width;
            mHeight = height;
        }

        void write(DataOutputStream out) throws IOException {
            out.write(mSignature);
            out.writeShort(mVersion);
            out.write(mReserved);
            out.writeShort(mChannelCount);
            out.writeInt(mHeight);
            out.writeInt(mWidth);
            out.writeShort(mDepth);
            out.writeShort(mMode);
        }
    }

    // Unused at the moment
    @SuppressWarnings({"UnusedDeclaration"})
    static class ColorMode {
        final int mLength = 0;
        
        void write(DataOutputStream out) throws IOException {
            out.writeInt(mLength);
        }
    }
    
    // Unused at the moment
    @SuppressWarnings({"UnusedDeclaration"})
    static class ImageResources {
        static final short RESOURCE_RESOLUTION_INFO = 0x03ED;
        
        int mLength = 0;

        final byte[] mSignature = "8BIM".getBytes();
        final short mResourceId = RESOURCE_RESOLUTION_INFO;
        
        final short mPad = 0;
        
        final int mDataLength = 16;

        final short mHorizontalDisplayUnit = 0x48; // 72 dpi
        final int mHorizontalResolution = 1;
        final short mWidthDisplayUnit = 1;

        final short mVerticalDisplayUnit = 0x48; // 72 dpi
        final int mVerticalResolution = 1;
        final short mHeightDisplayUnit = 1;
        
        ImageResources() {
            mLength = mSignature.length;
            mLength += 2;
            mLength += 2;            
            mLength += 4;            
            mLength += 8;            
            mLength += 8;            
        }

        void write(DataOutputStream out) throws IOException {
            out.writeInt(mLength);
            out.write(mSignature);
            out.writeShort(mResourceId);
            out.writeShort(mPad);
            out.writeInt(mDataLength);
            out.writeShort(mHorizontalDisplayUnit);
            out.writeInt(mHorizontalResolution);
            out.writeShort(mWidthDisplayUnit);
            out.writeShort(mVerticalDisplayUnit);
            out.writeInt(mVerticalResolution);
            out.writeShort(mHeightDisplayUnit);
        }
    }
    
    @SuppressWarnings({"UnusedDeclaration"})
    static class LayersMasksInfo {
        int mMiscLength;
        int mLayerInfoLength;
        
        void setLayersInfo(LayersInfo layersInfo) {
            mLayerInfoLength = layersInfo.getLength();
            // Round to the next multiple of 2
            if ((mLayerInfoLength & 0x1) == 0x1) mLayerInfoLength++;
            mMiscLength = mLayerInfoLength + 8;
        }

        void write(DataOutputStream out) throws IOException {
            out.writeInt(mMiscLength);
            out.writeInt(mLayerInfoLength);
        }
    }
    
    @SuppressWarnings({"UnusedDeclaration"})
    static class LayersInfo {
        final List<Layer> mLayers = new ArrayList<Layer>();

        void addLayer(String name, BufferedImage image, Point offset, boolean visible) {
            mLayers.add(new Layer(name, image, offset, visible));
        }

        int getLength() {
            int length = 2;
            for (Layer layer : mLayers) {
                length += layer.getLength();
            }
            return length;
        }

        void write(DataOutputStream out) throws IOException {
            out.writeShort((short) -mLayers.size());
            for (Layer layer : mLayers) {
                layer.write(out);
            }
        }

        void writeImageData(DataOutputStream out) throws IOException {
            for (Layer layer : mLayers) {
                layer.writeImageData(out);
            }
            // Global layer mask info length
            out.writeInt(0);
        }
    }
    
    @SuppressWarnings({"UnusedDeclaration"})
    static class Layer {
        static final byte OPACITY_TRANSPARENT = 0x0;
        static final byte OPACITY_OPAQUE = (byte) 0xFF;
        
        static final byte CLIPPING_BASE = 0x0;
        static final byte CLIPPING_NON_BASE = 0x1;
        
        static final byte FLAG_TRANSPARENCY_PROTECTED = 0x1;
        static final byte FLAG_INVISIBLE = 0x2;
        
        final int mTop;
        final int mLeft;
        final int mBottom;
        final int mRight;

        final short mChannelCount = 4;
        final Channel[] mChannelInfo = new Channel[mChannelCount];

        final byte[] mBlendSignature = "8BIM".getBytes();
        final byte[] mBlendMode = "norm".getBytes();

        final byte mOpacity = OPACITY_OPAQUE;
        final byte mClipping = CLIPPING_BASE;
        byte mFlags = 0x0;
        final byte mFiller = 0x0;
        
        int mExtraSize = 4 + 4;

        final int mMaskDataLength = 0;
        final int mBlendRangeDataLength = 0;

        final byte[] mName;

        final byte[] mLayerExtraSignature = "8BIM".getBytes();
        final byte[] mLayerExtraKey = "luni".getBytes();
        int mLayerExtraLength;
        final String mOriginalName;
        
        private BufferedImage mImage;

        Layer(String name, BufferedImage image, Point offset, boolean visible) {
            final int height = image.getHeight();
            final int width = image.getWidth();
            final int length = width * height;

            mChannelInfo[0] = new Channel(Channel.ID_ALPHA, length);
            mChannelInfo[1] = new Channel(Channel.ID_RED, length);
            mChannelInfo[2] = new Channel(Channel.ID_GREEN, length);
            mChannelInfo[3] = new Channel(Channel.ID_BLUE, length);

            mTop = offset.y;
            mLeft = offset.x;
            mBottom = offset.y + height;
            mRight = offset.x + width;

            mOriginalName = name;
            byte[] data = name.getBytes();

            try {
                mLayerExtraLength = 4 + mOriginalName.getBytes("UTF-16").length;
            } catch (UnsupportedEncodingException e) {
                e.printStackTrace();
            }

            final byte[] nameData = new byte[data.length + 1];
            nameData[0] = (byte) (data.length & 0xFF);
            System.arraycopy(data, 0, nameData, 1, data.length);

            // This could be done in the same pass as above
            if (nameData.length % 4 != 0) {
                data = new byte[nameData.length + 4 - (nameData.length % 4)];
                System.arraycopy(nameData, 0, data, 0, nameData.length);
                mName = data;
            } else {
                mName = nameData;
            }
            mExtraSize += mName.length;
            mExtraSize += mLayerExtraLength + 4 + mLayerExtraKey.length +
                    mLayerExtraSignature.length;

            mImage = image;
            
            if (!visible) {
                mFlags |= FLAG_INVISIBLE;
            }
        }

        int getLength() {
            int length = 4 * 4 + 2;

            for (Channel channel : mChannelInfo) {
                length += channel.getLength();
            }

            length += mBlendSignature.length;
            length += mBlendMode.length;
            length += 4;
            length += 4;
            length += mExtraSize;

            return length;
        }

        void write(DataOutputStream out) throws IOException {
            out.writeInt(mTop);
            out.writeInt(mLeft);
            out.writeInt(mBottom);
            out.writeInt(mRight);

            out.writeShort(mChannelCount);
            for (Channel channel : mChannelInfo) {
                channel.write(out);
            }            

            out.write(mBlendSignature);
            out.write(mBlendMode);

            out.write(mOpacity);
            out.write(mClipping);
            out.write(mFlags);
            out.write(mFiller);

            out.writeInt(mExtraSize);
            out.writeInt(mMaskDataLength);

            out.writeInt(mBlendRangeDataLength);            

            out.write(mName);

            out.write(mLayerExtraSignature);
            out.write(mLayerExtraKey);
            out.writeInt(mLayerExtraLength);
            out.writeInt(mOriginalName.length() + 1);
            out.write(mOriginalName.getBytes("UTF-16"));
        }

        void writeImageData(DataOutputStream out) throws IOException {
            writeImage(mImage, out, true);
        }
    }
    
    @SuppressWarnings({"UnusedDeclaration"})
    static class Channel {
        static final short ID_RED = 0;
        static final short ID_GREEN = 1;
        static final short ID_BLUE = 2;
        static final short ID_ALPHA = -1;
        static final short ID_LAYER_MASK = -2;
        
        final short mId;
        final int mDataLength;

        Channel(short id, int dataLength) {
            mId = id;
            mDataLength = dataLength + 2;
        }
        
        int getLength() {
            return 2 + 4 + mDataLength;
        }

        void write(DataOutputStream out) throws IOException {
            out.writeShort(mId);
            out.writeInt(mDataLength);
        }
    }
}
