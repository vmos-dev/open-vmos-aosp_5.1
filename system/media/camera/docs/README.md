# Camera Metadata XML
## Introduction
This is a set of scripts to manipulate the camera metadata in an XML form.

## Generated Files
Many files can be generated from XML, such as the documentation (html/pdf),
C code, Java code, and even XML itself (as a sanity check).

## Dependencies
* Python 2.7.x+
* Beautiful Soup 4+ - HTML/XML parser, used to parse `metadata_properties.xml`
* Mako 0.7+         - Template engine, needed to do file generation.
* Markdown 2.1+     - Plain text to HTML converter, for docs formatting.
* Tidy              - Cleans up the XML/HTML files.
* XML Lint          - Validates XML against XSD schema.

## Quick Setup (Ubuntu Precise):
sudo apt-get install python-mako
sudo apt-get install python-bs4
sudo apt-get install python-markdown
sudo apt-get install tidy
sudo apt-get install libxml2-utils #xmllint

## Quick Setup (MacPorts)
sudo port install py27-beautifulsoup4
sudo port install py27-mako
sudo port install py27-markdown
sudo port install tidy
sudo port install libxml2 #xmllint
