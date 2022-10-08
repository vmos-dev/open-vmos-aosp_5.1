Attribute Statistics
---------------------

This program gathers statistics about attribute usage in layout
files. This is how the "topAttrs" attributes listed in ADT's
extra-view-metadata.xml file (which drives the common attributes
listed in the top of the context menu) is determined by running this
script on a body of sample Android code, such as the AOSP repository.

This program takes one or more directory paths, and then it searches
all of them recursively for layout files that are not in folders
containing the string "test", and computes and prints frequency
statistics.
