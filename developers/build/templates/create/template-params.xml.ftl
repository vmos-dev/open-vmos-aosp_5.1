<#ftl>
<#--
        Copyright 2014 The Android Open Source Project

        Licensed under the Apache License, Version 2.0 (the "License");
        you may not use this file except in compliance with the License.
        You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

        Unless required by applicable law or agreed to in writing, software
        distributed under the License is distributed on an "AS IS" BASIS,
        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
        See the License for the specific language governing permissions and
        limitations under the License.
-->
<sample>
    <name>${sample.name}</name>
    <group>NoGroup</group>  <!-- This field will be deprecated in the future
                            and replaced with the "categories" tags below. -->
    <package>${sample.package}</package>

    <!-- change minSdk if needed-->
    <minSdk>${sample.minSdk}</minSdk>

    <!-- Include additional dependencies here.-->
    <!-- dependency>com.google.android.gms:play-services:5.0.+</dependency -->

    <strings>
        <intro>
            <![CDATA[
            Introductory text that explains what the sample is intended to demonstrate. Edit
            in template-params.xml.
            ]]>
        </intro>
    </strings>

    <!-- The basic templates have already been enabled. Uncomment more as desired. -->
    <template src="base" />
    <!-- template src="ActivityCards" / -->
    <!-- template src="FragmentView" / -->
    <!-- template src="CardStream" / -->
    <!-- template src="SimpleView" / -->
    <template src="SingleView" />

    <!-- Include common code modules by uncommenting them below. -->
    <common src="logger" />
    <!-- common src="activities"/ -->

    <metadata>
        <!-- Values: {DRAFT | PUBLISHED | INTERNAL | DEPRECATED | SUPERCEDED} -->
        <status>DRAFT</status>
        <!-- See http://go/sample-categories for details on the next 4 fields. -->
        <!-- Most samples just need to udpate the Categories field. This is a comma-
             seperated list of topic tags. Unlike the old category system, samples
             may have multiple categories, so feel free to add extras. Try to avoid
             simply tagging everything with "UI". :)-->
        <categories>Getting Started, UI</categories>
        <technologies>Android</technologies>
        <languages>Java</languages>
        <solutions>Mobile</solutions>
        <!-- Values: {BEGINNER | INTERMEDIATE | ADVANCED | EXPERT} -->
        <!-- Beginner is for "getting started" type content, or essential content.
             (e.g. "Hello World", activities, intents)

             Intermediate is for content that covers material a beginner doesn't need
             to know, but that a skilled developer is expected to know.
             (e.g. services, basic styles and theming, sync adapters)

             Advanced is for highly technical content geared towards experienced developers.
             (e.g. performance optimizations, custom views, bluetooth)

             Expert is reserved for highly technical or specialized content, and should
             be used sparingly. (e.g. VPN clients, SELinux, custom instrumentation runners) -->
        <level>BEGINNER</level>
        <!-- Dimensions: 512x512, PNG fomrat -->
        <icon>screenshots/icon-web.png</icon>
        <!-- Path to screenshots. Use <img> tags for each. -->
        <screenshots>
            <img>screenshots/1-main.png</img>
            <img>screenshots/2-settings.png</img>
        </screenshots>
        <!-- List of APIs that this sample should be cross-referenced under. Use <android>
        for fully-qualified Framework class names ("android:" namespace).

        Use <ext> for custom namespaces, if needed. See "Samples Index API" documentation
        for more details. -->
        <api_refs>
            <android>android.app.ActionBar</android>
        </api_refs>

        <!-- 1-3 line description of the sample here.

            Avoid simply rearranging the sample's title. What does this sample actually
            accomplish, and how does it do it? -->
        <description>
            Sample demonstrating how to instantiate an ActionBar on Android, define
            action items, and set an "up" navigation link. Uses the Support Library
            for compatibility with pre-3.0 devices.
        </description>

        <!-- Multi-paragraph introduction to sample, from an educational point-of-view.
        Makrdown formatting allowed. This will be used to generate a mini-article for the
        sample on DAC. -->
        <intro>
            Long intro here.

            Multi-paragraph introduction to sample, from an educational point-of-view.
            *Makrdown* formatting allowed. See [Markdown Documentation][1]
            for details.

            [1]: http://daringfireball.net/projects/markdown/syntax
        </intro>
    </metadata>
</sample>
