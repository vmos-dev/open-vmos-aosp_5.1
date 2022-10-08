#!/usr/bin/python

#
# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""
Usage:
  metadata_validate.py <filename.xml>
  - validates that the metadata properties defined in filename.xml are
    semantically correct.
  - does not do any XSD validation, use xmllint for that (in metadata-validate)

Module:
  A set of helpful functions for dealing with BeautifulSoup element trees.
  Especially the find_* and fully_qualified_name functions.

Dependencies:
  BeautifulSoup - an HTML/XML parser available to download from
                  http://www.crummy.com/software/BeautifulSoup/
"""

from bs4 import BeautifulSoup
from bs4 import Tag
import sys


#####################
#####################

def fully_qualified_name(entry):
  """
  Calculates the fully qualified name for an entry by walking the path
  to the root node.

  Args:
    entry: a BeautifulSoup Tag corresponding to an <entry ...> XML node,
           or a <clone ...> XML node.

  Raises:
    ValueError: if entry does not correspond to one of the above XML nodes

  Returns:
    A string with the full name, e.g. "android.lens.info.availableApertureSizes"
  """

  filter_tags = ['namespace', 'section']
  parents = [i['name'] for i in entry.parents if i.name in filter_tags]

  if entry.name == 'entry':
    name = entry['name']
  elif entry.name == 'clone':
    name = entry['entry'].split(".")[-1] # "a.b.c" => "c"
  else:
    raise ValueError("Unsupported tag type '%s' for element '%s'" \
                        %(entry.name, entry))

  parents.reverse()
  parents.append(name)

  fqn = ".".join(parents)

  return fqn

def find_parent_by_name(element, names):
  """
  Find the ancestor for an element whose name matches one of those
  in names.

  Args:
    element: A BeautifulSoup Tag corresponding to an XML node

  Returns:
    A BeautifulSoup element corresponding to the matched parent, or None.

    For example, assuming the following XML structure:
      <static>
        <anything>
          <entry name="Hello" />   # this is in variable 'Hello'
        </anything>
      </static>

      el = find_parent_by_name(Hello, ['static'])
      # el is now a value pointing to the '<static>' element
  """
  matching_parents = [i.name for i in element.parents if i.name in names]

  if matching_parents:
    return matching_parents[0]
  else:
    return None

def find_all_child_tags(element, tag):
    """
    Finds all the children that are a Tag (as opposed to a NavigableString),
    with a name of tag. This is useful to filter out the NavigableString out
    of the children.

    Args:
      element: A BeautifulSoup Tag corresponding to an XML node
      tag: A string representing the name of the tag

    Returns:
      A list of Tag instances

      For example, given the following XML structure:
        <enum>                    # This is the variable el
          Hello world             # NavigableString
          <value>Apple</value>    # this is the variale apple (Tag)
          <value>Orange</value>   # this is the variable orange (Tag)
          Hello world again       # NavigableString
        </enum>

        lst = find_all_child_tags(el, 'value')
        # lst is [apple, orange]

    """
    matching_tags = [i for i in element.children if isinstance(i, Tag) and i.name == tag]
    return matching_tags

def find_child_tag(element, tag):
    """
    Finds the first child that is a Tag with the matching name.

    Args:
      element: a BeautifulSoup Tag
      tag: A String representing the name of the tag

    Returns:
      An instance of a Tag, or None if there was no matches.

      For example, given the following XML structure:
        <enum>                    # This is the variable el
          Hello world             # NavigableString
          <value>Apple</value>    # this is the variale apple (Tag)
          <value>Orange</value>   # this is the variable orange (Tag)
          Hello world again       # NavigableString
        </enum>

        res = find_child_tag(el, 'value')
        # res is apple
    """
    matching_tags = find_all_child_tags(element, tag)
    if matching_tags:
        return matching_tags[0]
    else:
        return None

def find_kind(element):
  """
  Finds the kind Tag ancestor for an element.

  Args:
    element: a BeautifulSoup Tag

  Returns:
    a BeautifulSoup tag, or None if there was no matches

  Remarks:
    This function only makes sense to be called for an Entry, Clone, or
    InnerNamespace XML types. It will always return 'None' for other nodes.
  """
  kinds = ['dynamic', 'static', 'controls']
  parent_kind = find_parent_by_name(element, kinds)
  return parent_kind

def validate_error(msg):
  """
  Print a validation error to stderr.

  Args:
    msg: a string you want to be printed
  """
  print >> sys.stderr, "ERROR: " + msg


def validate_clones(soup):
  """
  Validate that all <clone> elements point to an existing <entry> element.

  Args:
    soup - an instance of BeautifulSoup

  Returns:
    True if the validation succeeds, False otherwise
  """
  success = True

  for clone in soup.find_all("clone"):
    clone_entry = clone['entry']
    clone_kind = clone['kind']

    parent_kind = find_kind(clone)

    find_entry = lambda x: x.name == 'entry'                           \
                       and find_kind(x) == clone_kind                  \
                       and fully_qualified_name(x) == clone_entry
    matching_entry = soup.find(find_entry)

    if matching_entry is None:
      error_msg = ("Did not find corresponding clone entry '%s' " +    \
               "with kind '%s'") %(clone_entry, clone_kind)
      validate_error(error_msg)
      success = False

    clone_name = fully_qualified_name(clone)
    if clone_name != clone_entry:
      error_msg = ("Clone entry target '%s' did not match fully qualified "  + \
                   "name '%s'.") %(clone_entry, clone_name)
      validate_error(error_msg)
      success = False

  return success

# All <entry> elements with container=$foo have a <$foo> child
# If type="enum", <enum> tag is present
# In <enum> for all <value id="$x">, $x is numeric
def validate_entries(soup):
  """
  Validate all <entry> elements with the following rules:
    * If there is a container="$foo" attribute, there is a <$foo> child
    * If there is a type="enum" attribute, there is an <enum> child
    * In the <enum> child, all <value id="$x"> have a numeric $x

  Args:
    soup - an instance of BeautifulSoup

  Returns:
    True if the validation succeeds, False otherwise
  """
  success = True
  for entry in soup.find_all("entry"):
    entry_container = entry.attrs.get('container')

    if entry_container is not None:
      container_tag = entry.find(entry_container)

      if container_tag is None:
        success = False
        validate_error(("Entry '%s' in kind '%s' has type '%s' but " +  \
                 "missing child element <%s>")                          \
                 %(fully_qualified_name(entry), find_kind(entry),       \
                 entry_container, entry_container))

    enum = entry.attrs.get('enum')
    if enum and enum == 'true':
      if entry.enum is None:
        validate_error(("Entry '%s' in kind '%s' is missing enum")     \
                               % (fully_qualified_name(entry), find_kind(entry),
                                  ))
        success = False

      else:
        for value in entry.enum.find_all('value'):
          value_id = value.attrs.get('id')

          if value_id is not None:
            try:
              id_int = int(value_id, 0) #autoguess base
            except ValueError:
              validate_error(("Entry '%s' has id '%s', which is not" + \
                                        " numeric.")                   \
                             %(fully_qualified_name(entry), value_id))
              success = False
    else:
      if entry.enum:
        validate_error(("Entry '%s' kind '%s' has enum el, but no enum attr")  \
                               % (fully_qualified_name(entry), find_kind(entry),
                                  ))
        success = False

  return success

def validate_xml(xml):
  """
  Validate all XML nodes according to the rules in validate_clones and
  validate_entries.

  Args:
    xml - A string containing a block of XML to validate

  Returns:
    a BeautifulSoup instance if validation succeeds, None otherwise
  """

  soup = BeautifulSoup(xml, features='xml')

  succ = validate_clones(soup)
  succ = validate_entries(soup) and succ

  if succ:
    return soup
  else:
    return None

#####################
#####################

if __name__ == "__main__":
  if len(sys.argv) <= 1:
    print >> sys.stderr, "Usage: %s <filename.xml>" % (sys.argv[0])
    sys.exit(0)

  file_name = sys.argv[1]
  succ = validate_xml(file(file_name).read()) is not None

  if succ:
    print "%s: SUCCESS! Document validated" %(file_name)
    sys.exit(0)
  else:
    print >> sys.stderr, "%s: ERRORS: Document failed to validate" %(file_name)
    sys.exit(1)
