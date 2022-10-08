<#ftl>
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

<#-- This build script is a bootstrapper for the "real" android build script that
is contained in templates/base. It includes only what's necessary for Android Studio
to recognize this as an Android project and start the template engine. -->

buildscript {
    repositories {
        mavenCentral()
    }

    dependencies {
        classpath 'com.android.tools.build:gradle:1.0.0'
    }
}

apply plugin: 'android'


android {
     <#-- Note that target SDK is hardcoded in this template. We expect all samples
          to always use the most current SDK as their target. -->
    compileSdkVersion ${compile_sdk}
    buildToolsVersion ${build_tools_version}

    defaultConfig {
        testPackageName "${sample.package}.test"
        testInstrumentationRunner "android.test.InstrumentationTestRunner"
    }
}

task preflight (dependsOn: parent.preflight) {
    project.afterEvaluate {
        <#noparse>
        // Inject a preflight task into each variant so we have a place to hook tasks
        // that need to run before any of the android build tasks.
        //
        android.applicationVariants.each { variant ->
            println variant.name
            tasks.getByPath("prepare${variant.name.capitalize()}Dependencies").dependsOn preflight
        }
        </#noparse>
    }
}



