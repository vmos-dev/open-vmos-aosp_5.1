<#--
        Copyright 2013 The Android Open Source Project

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

Steps to implement SimpleView template:
-in template-params.xml.ftl:
    -add the following line to common imports
        <common src="activities"/>

-Add a Fragment to handle behavior.  In your MainActivity.java class, it will reference a Fragment
 called (yourProjectName)Fragment.java.  Create that file in your project, using the "main" source
 folder instead of "common" or "templates".
   For instance, if your package name is com.example.foo, create the file
   src/main/java/com/example/foo/FooFragment.java

-refer to sampleSamples/simpleViewSample for a reference implementation of a
project built on this template.
