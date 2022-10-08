/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.eclipse.org/org/documents/epl-v10.php
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.Reader;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

/**
 * Gathers statistics about attribute usage in layout files. This is how the "topAttrs"
 * attributes listed in ADT's extra-view-metadata.xml (which drives the common attributes
 * listed in the top of the context menu) is determined by running this script on a body
 * of sample layout code.
 * <p>
 * This program takes one or more directory paths, and then it searches all of them recursively
 * for layout files that are not in folders containing the string "test", and computes and
 * prints frequency statistics.
 */
public class Analyzer {
    /** Number of attributes to print for each view */
    public static final int ATTRIBUTE_COUNT = 6;
    /** Separate out any attributes that constitute less than N percent of the total */
    public static final int THRESHOLD = 10; // percent

    private List<File> mDirectories;
    private File mCurrentFile;
    private boolean mListAdvanced;

    /** Map from view id to map from attribute to frequency count */
    private Map<String, Map<String, Usage>> mFrequencies =
            new HashMap<String, Map<String, Usage>>(100);

    private Map<String, Map<String, Usage>> mLayoutAttributeFrequencies =
            new HashMap<String, Map<String, Usage>>(100);

    private Map<String, String> mTopAttributes = new HashMap<String, String>(100);
    private Map<String, String> mTopLayoutAttributes = new HashMap<String, String>(100);

    private int mFileVisitCount;
    private int mLayoutFileCount;
    private File mXmlMetadataFile;

    private Analyzer(List<File> directories, File xmlMetadataFile, boolean listAdvanced) {
        mDirectories = directories;
        mXmlMetadataFile = xmlMetadataFile;
        mListAdvanced = listAdvanced;
    }

    public static void main(String[] args) {
        if (args.length < 1) {
            System.err.println("Usage: " + Analyzer.class.getSimpleName()
                    + " <directory1> [directory2 [directory3 ...]]\n");
            System.err.println("Recursively scans for layouts in the given directory and");
            System.err.println("computes statistics about attribute frequencies.");
            System.exit(-1);
        }

        File metadataFile = null;
        List<File> directories = new ArrayList<File>();
        boolean listAdvanced = false;
        for (int i = 0, n = args.length; i < n; i++) {
            String arg = args[i];

            if (arg.equals("--list")) {
                // List ALL encountered attributes
                listAdvanced = true;
                continue;
            }

            // The -metadata flag takes a pointer to an ADT extra-view-metadata.xml file
            // and attempts to insert topAttrs attributes into it (and saves it as same
            // file +.mod as an extension). This isn't listed on the usage flag because
            // it's pretty brittle and requires some manual fixups to the file afterwards.
            if (arg.equals("--metadata")) {
                i++;
                File file = new File(args[i]);
                if (!file.exists()) {
                    System.err.println(file.getName() + " does not exist");
                    System.exit(-5);
                }
                if (!file.isFile() || !file.getName().endsWith(".xml")) {
                    System.err.println(file.getName() + " must be an XML file");
                    System.exit(-4);
                }
                metadataFile = file;
                continue;
            }
            File directory = new File(arg);
            if (!directory.exists()) {
                System.err.println(directory.getName() + " does not exist");
                System.exit(-2);
            }

            if (!directory.isDirectory()) {
                System.err.println(directory.getName() + " is not a directory");
                System.exit(-3);
            }

            directories.add(directory);
        }

        new Analyzer(directories, metadataFile, listAdvanced).analyze();
    }

    private void analyze() {
        for (File directory : mDirectories) {
            scanDirectory(directory);
        }

        if (mListAdvanced) {
            listAdvanced();
        }

        printStatistics();

        if (mXmlMetadataFile != null) {
            printMergedMetadata();
        }
    }

    private void scanDirectory(File directory) {
        File[] files = directory.listFiles();
        if (files == null) {
            return;
        }

        for (File file : files) {
            mFileVisitCount++;
            if (mFileVisitCount % 50000 == 0) {
                System.out.println("Analyzed " + mFileVisitCount + " files...");
            }

            if (file.isFile()) {
                scanFile(file);
            } else if (file.isDirectory()) {
                // Skip stuff related to tests
                if (file.getName().contains("test")) {
                    continue;
                }

                // Recurse over subdirectories
                scanDirectory(file);
            }
        }
    }

    private void scanFile(File file) {
        if (file.getName().endsWith(".xml")) {
            File parent = file.getParentFile();
            if (parent.getName().startsWith("layout")) {
                analyzeLayout(file);
            }
        }

    }

    private void analyzeLayout(File file) {
        mCurrentFile = file;
        mLayoutFileCount++;
        Document document = null;
        DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
        InputSource is = new InputSource(new StringReader(readFile(file)));
        try {
            factory.setNamespaceAware(true);
            factory.setValidating(false);
            DocumentBuilder builder = factory.newDocumentBuilder();
            document = builder.parse(is);

            analyzeDocument(document);

        } catch (ParserConfigurationException e) {
            // pass -- ignore files we can't parse
        } catch (SAXException e) {
            // pass -- ignore files we can't parse
        } catch (IOException e) {
            // pass -- ignore files we can't parse
        }
    }


    private void analyzeDocument(Document document) {
        analyzeElement(document.getDocumentElement());
    }

    private void analyzeElement(Element element) {
        if (element.getTagName().equals("item")) {
            // Resource files shouldn't be in the layout/ folder but I came across
            // some cases
            System.out.println("Warning: found <item> tag in a layout file in "
                    + mCurrentFile.getPath());
            return;
        }

        countAttributes(element);
        countLayoutAttributes(element);

        // Recurse over children
        NodeList childNodes = element.getChildNodes();
        for (int i = 0, n = childNodes.getLength(); i < n; i++) {
            Node child = childNodes.item(i);
            if (child.getNodeType() == Node.ELEMENT_NODE) {
                analyzeElement((Element) child);
            }
        }
    }

    private void countAttributes(Element element) {
        String tag = element.getTagName();
        Map<String, Usage> attributeMap = mFrequencies.get(tag);
        if (attributeMap == null) {
            attributeMap = new HashMap<String, Usage>(70);
            mFrequencies.put(tag, attributeMap);
        }

        NamedNodeMap attributes = element.getAttributes();
        for (int i = 0, n = attributes.getLength(); i < n; i++) {
            Node attribute = attributes.item(i);
            String name = attribute.getNodeName();

            if (name.startsWith("android:layout_")) {
                // Skip layout attributes; they are a function of the parent layout that this
                // view is embedded within, not the view itself.
                // TODO: Consider whether we should incorporate this info or make statistics
                // about that as well?
                continue;
            }

            if (name.equals("android:id")) {
                // Skip ids: they are (mostly) unrelated to the view type and the tool
                // already offers id editing prominently
                continue;
            }

            if (name.startsWith("xmlns:")) {
                // Unrelated to frequency counts
                continue;
            }

            Usage usage = attributeMap.get(name);
            if (usage == null) {
                usage = new Usage(name);
            } else {
                usage.incrementCount();
            }
            attributeMap.put(name, usage);
        }
    }

    private void countLayoutAttributes(Element element) {
        String parentTag = element.getParentNode().getNodeName();
        Map<String, Usage> attributeMap = mLayoutAttributeFrequencies.get(parentTag);
        if (attributeMap == null) {
            attributeMap = new HashMap<String, Usage>(70);
            mLayoutAttributeFrequencies.put(parentTag, attributeMap);
        }

        NamedNodeMap attributes = element.getAttributes();
        for (int i = 0, n = attributes.getLength(); i < n; i++) {
            Node attribute = attributes.item(i);
            String name = attribute.getNodeName();

            if (!name.startsWith("android:layout_")) {
                continue;
            }

            // Skip layout_width and layout_height; they are mandatory in all but GridLayout so not
            // very interesting
            if (name.equals("android:layout_width") || name.equals("android:layout_height")) {
                continue;
            }

            Usage usage = attributeMap.get(name);
            if (usage == null) {
                usage = new Usage(name);
            } else {
                usage.incrementCount();
            }
            attributeMap.put(name, usage);
        }
    }

    // Copied from AdtUtils
    private static String readFile(File file) {
        try {
            return readFile(new FileReader(file));
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }

        return null;
    }

    private static String readFile(Reader inputStream) {
        BufferedReader reader = null;
        try {
            reader = new BufferedReader(inputStream);
            StringBuilder sb = new StringBuilder(2000);
            while (true) {
                int c = reader.read();
                if (c == -1) {
                    return sb.toString();
                } else {
                    sb.append((char)c);
                }
            }
        } catch (IOException e) {
            // pass -- ignore files we can't read
        } finally {
            try {
                if (reader != null) {
                    reader.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        return null;
    }

    private void printStatistics() {
        System.out.println("Analyzed " + mLayoutFileCount
                + " layouts (in a directory trees containing " + mFileVisitCount + " files)");
        System.out.println("Top " + ATTRIBUTE_COUNT
                + " for each view (excluding layout_ attributes) :");
        System.out.println("\n");
        System.out.println(" Rank    Count    Share  Attribute");
        System.out.println("=========================================================");
        List<String> views = new ArrayList<String>(mFrequencies.keySet());
        Collections.sort(views);
        for (String view : views) {
            String top = processUageMap(view, mFrequencies.get(view));
            if (top != null) {
                mTopAttributes.put(view,  top);
            }
        }

        System.out.println("\n\n\nTop " + ATTRIBUTE_COUNT + " layout attributes (excluding "
                + "mandatory layout_width and layout_height):");
        System.out.println("\n");
        System.out.println(" Rank    Count    Share  Attribute");
        System.out.println("=========================================================");
        views = new ArrayList<String>(mLayoutAttributeFrequencies.keySet());
        Collections.sort(views);
        for (String view : views) {
            String top = processUageMap(view, mLayoutAttributeFrequencies.get(view));
            if (top != null) {
                mTopLayoutAttributes.put(view,  top);
            }
        }
    }

    private static String processUageMap(String view, Map<String, Usage> map) {
        if (map == null) {
            return null;
        }

        if (view.indexOf('.') != -1 && !view.startsWith("android.")) {
            // Skip custom views
            return null;
        }

        List<Usage> values = new ArrayList<Usage>(map.values());
        if (values.size() == 0) {
            return null;
        }

        Collections.sort(values);
        int totalCount = 0;
        for (Usage usage : values) {
            totalCount += usage.count;
        }

        System.out.println("\n<" + view + ">:");
        if (view.equals("#document")) {
            System.out.println("(Set on root tag, probably intended for included context)");
        }

        int place = 1;
        int count = 0;
        int prevCount = -1;
        float prevPercentage = 0f;
        StringBuilder sb = new StringBuilder();
        for (Usage usage : values) {
            if (count++ >= ATTRIBUTE_COUNT && usage.count < prevCount) {
                break;
            }

            float percentage = 100 * usage.count/(float)totalCount;
            if (percentage < THRESHOLD && prevPercentage >= THRESHOLD) {
                System.out.println("  -----Less than 10%-------------------------------------");
            }
            System.out.printf("  %1d.    %5d    %5.1f%%  %s\n", place, usage.count,
                    percentage, usage.attribute);

            prevPercentage = percentage;
            if (prevCount != usage.count) {
                prevCount = usage.count;
                place++;
            }

            if (percentage >= THRESHOLD /*&& usage.count > 1*/) { // 1:Ignore when not enough data?
                if (sb.length() > 0) {
                    sb.append(',');
                }
                String name = usage.attribute;
                if (name.startsWith("android:")) {
                    name = name.substring("android:".length());
                }
                sb.append(name);
            }
        }

        return sb.length() > 0 ? sb.toString() : null;
    }

    private void printMergedMetadata() {
        assert mXmlMetadataFile != null;
        String metadata = readFile(mXmlMetadataFile);
        if (metadata == null || metadata.length() == 0) {
            System.err.println("Invalid metadata file");
            System.exit(-6);
        }

        System.err.flush();
        System.out.println("\n\nUpdating layout metadata file...");
        System.out.flush();

        StringBuilder sb = new StringBuilder((int) (2 * mXmlMetadataFile.length()));
        String[] lines = metadata.split("\n");
        for (int i = 0; i < lines.length; i++) {
            String line = lines[i];
            sb.append(line).append('\n');
            int classIndex = line.indexOf("class=\"");
            if (classIndex != -1) {
                int start = classIndex + "class=\"".length();
                int end = line.indexOf('"', start + 1);
                if (end != -1) {
                    String view = line.substring(start, end);
                    if (view.startsWith("android.widget.")) {
                        view = view.substring("android.widget.".length());
                    } else if (view.startsWith("android.view.")) {
                        view = view.substring("android.view.".length());
                    } else if (view.startsWith("android.webkit.")) {
                        view = view.substring("android.webkit.".length());
                    }
                    String top = mTopAttributes.get(view);
                    if (top == null) {
                        System.err.println("Warning: No frequency data for view " + view);
                    } else {
                        sb.append(line.substring(0, classIndex)); // Indentation

                        sb.append("topAttrs=\"");
                        sb.append(top);
                        sb.append("\"\n");
                    }

                    top = mTopLayoutAttributes.get(view);
                    if (top != null) {
                        // It's a layout attribute
                        sb.append(line.substring(0, classIndex)); // Indentation

                        sb.append("topLayoutAttrs=\"");
                        sb.append(top);
                        sb.append("\"\n");
                    }
                }
            }
        }

        System.out.println("\nTop attributes:");
        System.out.println("--------------------------");
        List<String> views = new ArrayList<String>(mTopAttributes.keySet());
        Collections.sort(views);
        for (String view : views) {
            String top = mTopAttributes.get(view);
            System.out.println(view + ": " + top);
        }

        System.out.println("\nTop layout attributes:");
        System.out.println("--------------------------");
        views = new ArrayList<String>(mTopLayoutAttributes.keySet());
        Collections.sort(views);
        for (String view : views) {
            String top = mTopLayoutAttributes.get(view);
            System.out.println(view + ": " + top);
        }

        System.out.println("\nModified XML metadata file:\n");
        String newContent = sb.toString();
        File output = new File(mXmlMetadataFile.getParentFile(), mXmlMetadataFile.getName() + ".mod");
        if (output.exists()) {
            output.delete();
        }
        try {
            BufferedWriter writer = new BufferedWriter(new FileWriter(output));
            writer.write(newContent);
            writer.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        System.out.println("Done - wrote " + output.getPath());
    }

    //private File mPublicFile = new File(location, "data/res/values/public.xml");
    private File mPublicFile = new File("/Volumes/AndroidWork/git/frameworks/base/core/res/res/values/public.xml");

    private void listAdvanced() {
        Set<String> keys = new HashSet<String>(1000);

        // Merged usages across view types
        Map<String, Usage> mergedUsages = new HashMap<String, Usage>(100);

        for (Entry<String,Map<String,Usage>> entry : mFrequencies.entrySet()) {
            String view = entry.getKey();
            if (view.indexOf('.') != -1 && !view.startsWith("android.")) {
                // Skip custom views etc
                continue;
            }
            Map<String, Usage> map = entry.getValue();
            for (Usage usage : map.values()) {
//                if (usage.count == 1) {
//                    System.out.println("Only found *one* usage of " + usage.attribute);
//                }
//                if (usage.count < 4) {
//                    System.out.println("Only found " + usage.count + " usage of " + usage.attribute);
//                }

                String attribute = usage.attribute;
                int index = attribute.indexOf(':');
                if (index == -1 || attribute.startsWith("android:")) {
                    Usage merged = mergedUsages.get(attribute);
                    if (merged == null) {
                        merged = new Usage(attribute);
                        merged.count = usage.count;
                        mergedUsages.put(attribute, merged);
                    } else {
                        merged.count += usage.count;
                    }
                }
            }
        }

        for (Usage usage : mergedUsages.values())  {
            String attribute = usage.attribute;
            if (usage.count < 4) {
                System.out.println("Only found " + usage.count + " usage of " + usage.attribute);
                continue;
            }
            int index = attribute.indexOf(':');
            if (index != -1) {
                attribute = attribute.substring(index + 1); // +1: skip ':'
            }
            keys.add(attribute);
        }

        List<String> sorted = new ArrayList<String>(keys);
        Collections.sort(sorted);
        System.out.println("\nEncountered Attributes");
        System.out.println("-----------------------------");
        for (String attribute : sorted) {
            System.out.println(attribute);
        }

        System.out.println();
    }

    private static class Usage implements Comparable<Usage> {
        public String attribute;
        public int count;


        public Usage(String attribute) {
            super();
            this.attribute = attribute;

            count = 1;
        }

        public void incrementCount() {
            count++;
        }

        @Override
        public int compareTo(Usage o) {
            // Sort by decreasing frequency, then sort alphabetically
            int frequencyDelta = o.count - count;
            if (frequencyDelta != 0) {
                return frequencyDelta;
            } else {
                return attribute.compareTo(o.attribute);
            }
        }

        @Override
        public String toString() {
            return attribute + ": " + count;
        }

        @Override
        public int hashCode() {
            final int prime = 31;
            int result = 1;
            result = prime * result + ((attribute == null) ? 0 : attribute.hashCode());
            return result;
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj)
                return true;
            if (obj == null)
                return false;
            if (getClass() != obj.getClass())
                return false;
            Usage other = (Usage) obj;
            if (attribute == null) {
                if (other.attribute != null)
                    return false;
            } else if (!attribute.equals(other.attribute))
                return false;
            return true;
        }
    }
}
