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
A set of classes (models) each closely representing an XML node in the
metadata_properties.xml file.

  Node: Base class for most nodes.
  Entry: A node corresponding to <entry> elements.
  Clone: A node corresponding to <clone> elements.
  MergedEntry: A node corresponding to either <entry> or <clone> elements.
  Kind: A node corresponding to <dynamic>, <static>, <controls> elements.
  InnerNamespace: A node corresponding to a <namespace> nested under a <kind>.
  OuterNamespace: A node corresponding to a <namespace> with <kind> children.
  Section: A node corresponding to a <section> element.
  Enum: A class corresponding an <enum> element within an <entry>
  EnumValue: A class corresponding to a <value> element within an Enum
  Metadata: Root node that also provides tree construction functionality.
  Tag: A node corresponding to a top level <tag> element.
  Typedef: A node corresponding to a <typedef> element under <types>.
"""

import sys
import itertools
from collections import OrderedDict

class Node(object):
  """
  Base class for most nodes that are part of the Metadata graph.

  Attributes (Read-Only):
    parent: An edge to a parent Node.
    name: A string describing the name, usually but not always the 'name'
          attribute of the corresponding XML node.
  """

  def __init__(self):
    self._parent = None
    self._name = None

  @property
  def parent(self):
    return self._parent

  @property
  def name(self):
    return self._name

  def find_all(self, pred):
    """
    Find all descendants that match the predicate.

    Args:
      pred: a predicate function that acts as a filter for a Node

    Yields:
      A sequence of all descendants for which pred(node) is true,
      in a pre-order visit order.
    """
    if pred(self):
      yield self

    if self._get_children() is None:
      return

    for i in self._get_children():
      for j in i.find_all(pred):
        yield j

  def find_first(self, pred):
    """
    Find the first descendant that matches the predicate.

    Args:
      pred: a predicate function that acts as a filter for a Node

    Returns:
      The first Node from find_all(pred), or None if there were no results.
    """
    for i in self.find_all(pred):
      return i

    return None

  def find_parent_first(self, pred):
    """
    Find the first ancestor that matches the predicate.

    Args:
      pred: A predicate function that acts as a filter for a Node

    Returns:
      The first ancestor closest to the node for which pred(node) is true.
    """
    for i in self.find_parents(pred):
      return i

    return None

  def find_parents(self, pred):
    """
    Find all ancestors that match the predicate.

    Args:
      pred: A predicate function that acts as a filter for a Node

    Yields:
      A sequence of all ancestors (closest to furthest) from the node,
      where pred(node) is true.
    """
    parent = self.parent

    while parent is not None:
      if pred(parent):
        yield parent
      parent = parent.parent

  def sort_children(self):
    """
    Sorts the immediate children in-place.
    """
    self._sort_by_name(self._children)

  def _sort_by_name(self, what):
    what.sort(key=lambda x: x.name)

  def _get_name(self):
    return lambda x: x.name

  # Iterate over all children nodes. None when node doesn't support children.
  def _get_children(self):
    return (i for i in self._children)

  def _children_name_map_matching(self, match=lambda x: True):
    d = {}
    for i in self._get_children():
      if match(i):
        d[i.name] = i
    return d

  @staticmethod
  def _dictionary_by_name(values):
    d = OrderedDict()
    for i in values:
      d[i.name] = i

    return d

  def validate_tree(self):
    """
    Sanity check the tree recursively, ensuring for a node n, all children's
    parents are also n.

    Returns:
      True if validation succeeds, False otherwise.
    """
    succ = True
    children = self._get_children()
    if children is None:
      return True

    for child in self._get_children():
      if child.parent != self:
        print >> sys.stderr, ("ERROR: Node '%s' doesn't match the parent" +    \
                             "(expected: %s, actual %s)")                      \
                             %(child, self, child.parent)
        succ = False

      succ = child.validate_tree() and succ

    return succ

  def __str__(self):
    return "<%s name='%s'>" %(self.__class__, self.name)

class Metadata(Node):
  """
  A node corresponding to a <metadata> entry.

  Attributes (Read-Only):
    parent: An edge to the parent Node. This is always None for Metadata.
    outer_namespaces: A sequence of immediate OuterNamespace children.
    tags: A sequence of all Tag instances available in the graph.
    types: An iterable of all Typedef instances available in the graph.
  """

  def __init__(self):
    """
    Initialize with no children. Use insert_* functions and then
    construct_graph() to build up the Metadata from some source.
    """
# Private
    self._entries = []
    # kind => { name => entry }
    self._entry_map = { 'static': {}, 'dynamic': {}, 'controls': {} }
    self._entries_ordered = [] # list of ordered Entry/Clone instances
    self._clones = []

# Public (Read Only)
    self._name = None
    self._parent = None
    self._outer_namespaces = None
    self._tags = []
    self._types = []

  @property
  def outer_namespaces(self):
    if self._outer_namespaces is None:
      return None
    else:
      return (i for i in self._outer_namespaces)

  @property
  def tags(self):
    return (i for i in self._tags)

  @property
  def types(self):
    return (i for i in self._types)

  def _get_properties(self):

    for i in self._entries:
      yield i

    for i in self._clones:
      yield i

  def insert_tag(self, tag, description=""):
    """
    Insert a tag into the metadata.

    Args:
      tag: A string identifier for a tag.
      description: A string description for a tag.

    Example:
      metadata.insert_tag("BC", "Backwards Compatibility for old API")

    Remarks:
      Subsequent calls to insert_tag with the same tag are safe (they will
      be ignored).
    """
    tag_ids = [tg.name for tg in self.tags if tg.name == tag]
    if not tag_ids:
      self._tags.append(Tag(tag, self, description))

  def insert_type(self, type_name, type_selector="typedef", **kwargs):
    """
    Insert a type into the metadata.

    Args:
      type_name: A type's name
      type_selector: The selector for the type, e.g. 'typedef'

    Args (if type_selector == 'typedef'):
      languages: A map of 'language name' -> 'fully qualified class path'

    Example:
      metadata.insert_type('rectangle', 'typedef',
                           { 'java': 'android.graphics.Rect' })

    Remarks:
      Subsequent calls to insert_type with the same type name are safe (they
      will be ignored)
    """

    if type_selector != 'typedef':
      raise ValueError("Unsupported type_selector given " + type_selector)

    type_names = [tp.name for tp in self.types if tp.name == tp]
    if not type_names:
      self._types.append(Typedef(type_name, self, kwargs.get('languages')))

  def insert_entry(self, entry):
    """
    Insert an entry into the metadata.

    Args:
      entry: A key-value dictionary describing an entry. Refer to
             Entry#__init__ for the keys required/optional.

    Remarks:
      Subsequent calls to insert_entry with the same entry+kind name are safe
      (they will be ignored).
    """
    e = Entry(**entry)
    self._entries.append(e)
    self._entry_map[e.kind][e.name] = e
    self._entries_ordered.append(e)

  def insert_clone(self, clone):
    """
    Insert a clone into the metadata.

    Args:
      clone: A key-value dictionary describing a clone. Refer to
            Clone#__init__ for the keys required/optional.

    Remarks:
      Subsequent calls to insert_clone with the same clone+kind name are safe
      (they will be ignored). Also the target entry need not be inserted
      ahead of the clone entry.
    """
    # figure out corresponding entry later. allow clone insert, entry insert
    entry = None
    c = Clone(entry, **clone)
    self._entry_map[c.kind][c.name] = c
    self._clones.append(c)
    self._entries_ordered.append(c)

  def prune_clones(self):
    """
    Remove all clones that don't point to an existing entry.

    Remarks:
      This should be called after all insert_entry/insert_clone calls have
      finished.
    """
    remove_list = []
    for p in self._clones:
      if p.entry is None:
        remove_list.append(p)

    for p in remove_list:

      # remove from parent's entries list
      if p.parent is not None:
        p.parent._entries.remove(p)
      # remove from parents' _leafs list
      for ancestor in p.find_parents(lambda x: not isinstance(x, Metadata)):
        ancestor._leafs.remove(p)

      # remove from global list
      self._clones.remove(p)
      self._entry_map[p.kind].pop(p.name)
      self._entries_ordered.remove(p)


  # After all entries/clones are inserted,
  # invoke this to generate the parent/child node graph all these objects
  def construct_graph(self):
    """
    Generate the graph recursively, after which all Entry nodes will be
    accessible recursively by crawling through the outer_namespaces sequence.

    Remarks:
      This is safe to be called multiple times at any time. It should be done at
      least once or there will be no graph.
    """
    self.validate_tree()
    self._construct_tags()
    self.validate_tree()
    self._construct_types()
    self.validate_tree()
    self._construct_clones()
    self.validate_tree()
    self._construct_outer_namespaces()
    self.validate_tree()

  def _construct_tags(self):
    tag_dict = self._dictionary_by_name(self.tags)
    for p in self._get_properties():
      p._tags = []
      for tag_id in p._tag_ids:
        tag = tag_dict.get(tag_id)

        if tag not in p._tags:
          p._tags.append(tag)

        if p not in tag.entries:
          tag._entries.append(p)

  def _construct_types(self):
    type_dict = self._dictionary_by_name(self.types)
    for p in self._get_properties():
      if p._type_name:
        type_node = type_dict.get(p._type_name)
        p._typedef = type_node

        if p not in type_node.entries:
          type_node._entries.append(p)

  def _construct_clones(self):
    for p in self._clones:
      target_kind = p.target_kind
      target_entry = self._entry_map[target_kind].get(p.name)
      p._entry = target_entry

      # should not throw if we pass validation
      # but can happen when importing obsolete CSV entries
      if target_entry is None:
        print >> sys.stderr, ("WARNING: Clone entry '%s' target kind '%s'" +   \
                              " has no corresponding entry")                   \
                             %(p.name, p.target_kind)

  def _construct_outer_namespaces(self):

    if self._outer_namespaces is None: #the first time this runs
      self._outer_namespaces = []

    root = self._dictionary_by_name(self._outer_namespaces)
    for ons_name, ons in root.iteritems():
      ons._leafs = []

    for p in self._entries_ordered:
      ons_name = p.get_outer_namespace()
      ons = root.get(ons_name, OuterNamespace(ons_name, self))
      root[ons_name] = ons

      if p not in ons._leafs:
        ons._leafs.append(p)

    for ons_name, ons in root.iteritems():

      ons.validate_tree()

      self._construct_sections(ons)

      if ons not in self._outer_namespaces:
        self._outer_namespaces.append(ons)

      ons.validate_tree()

  def _construct_sections(self, outer_namespace):

    sections_dict = self._dictionary_by_name(outer_namespace.sections)
    for sec_name, sec in sections_dict.iteritems():
      sec._leafs = []
      sec.validate_tree()

    for p in outer_namespace._leafs:
      does_exist = sections_dict.get(p.get_section())

      sec = sections_dict.get(p.get_section(), \
          Section(p.get_section(), outer_namespace))
      sections_dict[p.get_section()] = sec

      sec.validate_tree()

      if p not in sec._leafs:
        sec._leafs.append(p)

    for sec_name, sec in sections_dict.iteritems():

      if not sec.validate_tree():
        print >> sys.stderr, ("ERROR: Failed to validate tree in " +           \
                             "construct_sections (start), with section = '%s'")\
                             %(sec)

      self._construct_kinds(sec)

      if sec not in outer_namespace.sections:
        outer_namespace._sections.append(sec)

      if not sec.validate_tree():
        print >> sys.stderr, ("ERROR: Failed to validate tree in " +           \
                              "construct_sections (end), with section = '%s'") \
                             %(sec)

  # 'controls', 'static' 'dynamic'. etc
  def _construct_kinds(self, section):
    for kind in section.kinds:
      kind._leafs = []
      section.validate_tree()

    group_entry_by_kind = itertools.groupby(section._leafs, lambda x: x.kind)
    leaf_it = ((k, g) for k, g in group_entry_by_kind)

    # allow multiple kinds with the same name. merge if adjacent
    # e.g. dynamic,dynamic,static,static,dynamic -> dynamic,static,dynamic
    # this helps maintain ABI compatibility when adding an entry in a new kind
    for idx, (kind_name, entry_it) in enumerate(leaf_it):
      if idx >= len(section._kinds):
        kind = Kind(kind_name, section)
        section._kinds.append(kind)
        section.validate_tree()

      kind = section._kinds[idx]

      for p in entry_it:
        if p not in kind._leafs:
          kind._leafs.append(p)

    for kind in section._kinds:
      kind.validate_tree()
      self._construct_inner_namespaces(kind)
      kind.validate_tree()
      self._construct_entries(kind)
      kind.validate_tree()

      if not section.validate_tree():
        print >> sys.stderr, ("ERROR: Failed to validate tree in " +           \
                             "construct_kinds, with kind = '%s'") %(kind)

      if not kind.validate_tree():
        print >> sys.stderr, ("ERROR: Failed to validate tree in " +           \
                              "construct_kinds, with kind = '%s'") %(kind)

  def _construct_inner_namespaces(self, parent, depth=0):
    #parent is InnerNamespace or Kind
    ins_dict = self._dictionary_by_name(parent.namespaces)
    for name, ins in ins_dict.iteritems():
      ins._leafs = []

    for p in parent._leafs:
      ins_list = p.get_inner_namespace_list()

      if len(ins_list) > depth:
        ins_str = ins_list[depth]
        ins = ins_dict.get(ins_str, InnerNamespace(ins_str, parent))
        ins_dict[ins_str] = ins

        if p not in ins._leafs:
          ins._leafs.append(p)

    for name, ins in ins_dict.iteritems():
      ins.validate_tree()
      # construct children INS
      self._construct_inner_namespaces(ins, depth + 1)
      ins.validate_tree()
      # construct children entries
      self._construct_entries(ins, depth + 1)

      if ins not in parent.namespaces:
        parent._namespaces.append(ins)

      if not ins.validate_tree():
        print >> sys.stderr, ("ERROR: Failed to validate tree in " +           \
                              "construct_inner_namespaces, with ins = '%s'")   \
                             %(ins)

  # doesnt construct the entries, so much as links them
  def _construct_entries(self, parent, depth=0):
    #parent is InnerNamespace or Kind
    entry_dict = self._dictionary_by_name(parent.entries)
    for p in parent._leafs:
      ins_list = p.get_inner_namespace_list()

      if len(ins_list) == depth:
        entry = entry_dict.get(p.name, p)
        entry_dict[p.name] = entry

    for name, entry in entry_dict.iteritems():

      old_parent = entry.parent
      entry._parent = parent

      if entry not in parent.entries:
        parent._entries.append(entry)

      if old_parent is not None and old_parent != parent:
        print >> sys.stderr, ("ERROR: Parent changed from '%s' to '%s' for " + \
                              "entry '%s'")                                    \
                             %(old_parent.name, parent.name, entry.name)

  def _get_children(self):
    if self.outer_namespaces is not None:
      for i in self.outer_namespaces:
        yield i

    if self.tags is not None:
      for i in self.tags:
        yield i

class Tag(Node):
  """
  A tag Node corresponding to a top-level <tag> element.

  Attributes (Read-Only):
    name: alias for id
    id: The name of the tag, e.g. for <tag id="BC"/> id = 'BC'
    description: The description of the tag, the contents of the <tag> element.
    parent: An edge to the parent, which is always the Metadata root node.
    entries: A sequence of edges to entries/clones that are using this Tag.
  """
  def __init__(self, name, parent, description=""):
    self._name        = name  # 'id' attribute in XML
    self._id          = name
    self._description = description
    self._parent      = parent

    # all entries that have this tag, including clones
    self._entries     = []  # filled in by Metadata#construct_tags

  @property
  def id(self):
    return self._id

  @property
  def description(self):
    return self._description

  @property
  def entries(self):
    return (i for i in self._entries)

  def _get_children(self):
    return None

class Typedef(Node):
  """
  A typedef Node corresponding to a <typedef> element under a top-level <types>.

  Attributes (Read-Only):
    name: The name of this typedef as a string.
    languages: A dictionary of 'language name' -> 'fully qualified class'.
    parent: An edge to the parent, which is always the Metadata root node.
    entries: An iterable over all entries which reference this typedef.
  """
  def __init__(self, name, parent, languages=None):
    self._name        = name
    self._parent      = parent

    # all entries that have this typedef
    self._entries     = []  # filled in by Metadata#construct_types

    self._languages   = languages or {}

  @property
  def languages(self):
    return self._languages

  @property
  def entries(self):
    return (i for i in self._entries)

  def _get_children(self):
    return None

class OuterNamespace(Node):
  """
  A node corresponding to a <namespace> element under <metadata>

  Attributes (Read-Only):
    name: The name attribute of the <namespace name="foo"> element.
    parent: An edge to the parent, which is always the Metadata root node.
    sections: A sequence of Section children.
  """
  def __init__(self, name, parent, sections=[]):
    self._name = name
    self._parent = parent # MetadataSet
    self._sections = sections[:]
    self._leafs = []

    self._children = self._sections

  @property
  def sections(self):
    return (i for i in self._sections)

class Section(Node):
  """
  A node corresponding to a <section> element under <namespace>

  Attributes (Read-Only):
    name: The name attribute of the <section name="foo"> element.
    parent: An edge to the parent, which is always an OuterNamespace instance.
    description: A string description of the section, or None.
    kinds: A sequence of Kind children.
    merged_kinds: A sequence of virtual Kind children,
                  with each Kind's children merged by the kind.name
  """
  def __init__(self, name, parent, description=None, kinds=[]):
    self._name = name
    self._parent = parent
    self._description = description
    self._kinds = kinds[:]

    self._leafs = []


  @property
  def description(self):
    return self._description

  @property
  def kinds(self):
    return (i for i in self._kinds)

  def sort_children(self):
    self.validate_tree()
    # order is always controls,static,dynamic
    find_child = lambda x: [i for i in self._get_children() if i.name == x]
    new_lst = find_child('controls') \
            + find_child('static')   \
            + find_child('dynamic')
    self._kinds = new_lst
    self.validate_tree()

  def _get_children(self):
    return (i for i in self.kinds)

  @property
  def merged_kinds(self):

    def aggregate_by_name(acc, el):
      existing = [i for i in acc if i.name == el.name]
      if existing:
        k = existing[0]
      else:
        k = Kind(el.name, el.parent)
        acc.append(k)

      k._namespaces.extend(el._namespaces)
      k._entries.extend(el._entries)

      return acc

    new_kinds_lst = reduce(aggregate_by_name, self.kinds, [])

    for k in new_kinds_lst:
      yield k

  def combine_kinds_into_single_node(self):
    r"""
    Combines the section's Kinds into a single node.

    Combines all the children (kinds) of this section into a single
    virtual Kind node.

    Returns:
      A new Kind node that collapses all Kind siblings into one, combining
      all their children together.

      For example, given self.kinds == [ x, y ]

        x  y               z
      / |  | \    -->   / | | \
      a b  c d          a b c d

      a new instance z is returned in this example.

    Remarks:
      The children of the kinds are the same references as before, that is
      their parents will point to the old parents and not to the new parent.
    """
    combined = Kind(name="combined", parent=self)

    for k in self._get_children():
      combined._namespaces.extend(k.namespaces)
      combined._entries.extend(k.entries)

    return combined

class Kind(Node):
  """
  A node corresponding to one of: <static>,<dynamic>,<controls> under a
  <section> element.

  Attributes (Read-Only):
    name: A string which is one of 'static', 'dynamic, or 'controls'.
    parent: An edge to the parent, which is always a Section  instance.
    namespaces: A sequence of InnerNamespace children.
    entries: A sequence of Entry/Clone children.
    merged_entries: A sequence of MergedEntry virtual nodes from entries
  """
  def __init__(self, name, parent):
    self._name = name
    self._parent = parent
    self._namespaces = []
    self._entries = []

    self._leafs = []

  @property
  def namespaces(self):
    return self._namespaces

  @property
  def entries(self):
    return self._entries

  @property
  def merged_entries(self):
    for i in self.entries:
      yield i.merge()

  def sort_children(self):
    self._namespaces.sort(key=self._get_name())
    self._entries.sort(key=self._get_name())

  def _get_children(self):
    for i in self.namespaces:
      yield i
    for i in self.entries:
      yield i

  def combine_children_by_name(self):
    r"""
    Combine multiple children with the same name into a single node.

    Returns:
      A new Kind where all of the children with the same name were combined.

      For example:

      Given a Kind k:

              k
            / | \
            a b c
            | | |
            d e f

      a.name == "foo"
      b.name == "foo"
      c.name == "bar"

      The returned Kind will look like this:

             k'
            /  \
            a' c'
          / |  |
          d e  f

    Remarks:
      This operation is not recursive. To combine the grandchildren and other
      ancestors, call this method on the ancestor nodes.
    """
    return Kind._combine_children_by_name(self, new_type=type(self))

  # new_type is either Kind or InnerNamespace
  @staticmethod
  def _combine_children_by_name(self, new_type):
    new_ins_dict = OrderedDict()
    new_ent_dict = OrderedDict()

    for ins in self.namespaces:
      new_ins = new_ins_dict.setdefault(ins.name,
                                        InnerNamespace(ins.name, parent=self))
      new_ins._namespaces.extend(ins.namespaces)
      new_ins._entries.extend(ins.entries)

    for ent in self.entries:
      new_ent = new_ent_dict.setdefault(ent.name,
                                        ent.merge())

    kind = new_type(self.name, self.parent)
    kind._namespaces = new_ins_dict.values()
    kind._entries = new_ent_dict.values()

    return kind

class InnerNamespace(Node):
  """
  A node corresponding to a <namespace> which is an ancestor of a Kind.
  These namespaces may have other namespaces recursively, or entries as leafs.

  Attributes (Read-Only):
    name: Name attribute from the element, e.g. <namespace name="foo"> -> 'foo'
    parent: An edge to the parent, which is an InnerNamespace or a Kind.
    namespaces: A sequence of InnerNamespace children.
    entries: A sequence of Entry/Clone children.
    merged_entries: A sequence of MergedEntry virtual nodes from entries
  """
  def __init__(self, name, parent):
    self._name        = name
    self._parent      = parent
    self._namespaces  = []
    self._entries     = []
    self._leafs       = []

  @property
  def namespaces(self):
    return self._namespaces

  @property
  def entries(self):
    return self._entries

  @property
  def merged_entries(self):
    for i in self.entries:
      yield i.merge()

  def sort_children(self):
    self._namespaces.sort(key=self._get_name())
    self._entries.sort(key=self._get_name())

  def _get_children(self):
    for i in self.namespaces:
      yield i
    for i in self.entries:
      yield i

  def combine_children_by_name(self):
    r"""
    Combine multiple children with the same name into a single node.

    Returns:
      A new InnerNamespace where all of the children with the same name were
      combined.

      For example:

      Given an InnerNamespace i:

              i
            / | \
            a b c
            | | |
            d e f

      a.name == "foo"
      b.name == "foo"
      c.name == "bar"

      The returned InnerNamespace will look like this:

             i'
            /  \
            a' c'
          / |  |
          d e  f

    Remarks:
      This operation is not recursive. To combine the grandchildren and other
      ancestors, call this method on the ancestor nodes.
    """
    return Kind._combine_children_by_name(self, new_type=type(self))

class EnumValue(Node):
  """
  A class corresponding to a <value> element within an <enum> within an <entry>.

  Attributes (Read-Only):
    name: A string,                 e.g. 'ON' or 'OFF'
    id: An optional numeric string, e.g. '0' or '0xFF'
    optional: A boolean
    hidden: A boolean, True if the enum should be hidden.
    notes: A string describing the notes, or None.
    parent: An edge to the parent, always an Enum instance.
  """
  def __init__(self, name, parent, id=None, optional=False, hidden=False, notes=None):
    self._name = name                    # str, e.g. 'ON' or 'OFF'
    self._id = id                        # int, e.g. '0'
    self._optional = optional            # bool
    self._hidden = hidden                # bool
    self._notes = notes                  # None or str
    self._parent = parent

  @property
  def id(self):
    return self._id

  @property
  def optional(self):
    return self._optional

  @property
  def hidden(self):
    return self._hidden

  @property
  def notes(self):
    return self._notes

  def _get_children(self):
    return None

class Enum(Node):
  """
  A class corresponding to an <enum> element within an <entry>.

  Attributes (Read-Only):
    parent: An edge to the parent, always an Entry instance.
    values: A sequence of EnumValue children.
    has_values_with_id: A boolean representing if any of the children have a
        non-empty id property.
  """
  def __init__(self, parent, values, ids={}, optionals=[], hiddens=[], notes={}):
    self._values =                                                             \
      [ EnumValue(val, self, ids.get(val), val in optionals, val in hiddens,   \
                  notes.get(val))                                              \
        for val in values ]

    self._parent = parent
    self._name = None

  @property
  def values(self):
    return (i for i in self._values)

  @property
  def has_values_with_id(self):
    return bool(any(i for i in self.values if i.id))

  def _get_children(self):
    return (i for i in self._values)

class Entry(Node):
  """
  A node corresponding to an <entry> element.

  Attributes (Read-Only):
    parent: An edge to the parent node, which is an InnerNamespace or Kind.
    name: The fully qualified name string, e.g. 'android.shading.mode'
    name_short: The name attribute from <entry name="mode">, e.g. mode
    type: The type attribute from <entry type="bar">
    kind: A string ('static', 'dynamic', 'controls') corresponding to the
          ancestor Kind#name
    container: The container attribute from <entry container="array">, or None.
    container_sizes: A sequence of size strings or None if container is None.
    enum: An Enum instance if the enum attribute is true, None otherwise.
    visibility: The visibility of this entry ('system', 'hidden', 'public')
                across the system. System entries are only visible in native code
                headers. Hidden entries are marked @hide in managed code, while
                public entries are visible in the Android SDK.
    applied_visibility: As visibility, but always valid, defaulting to 'system'
                        if no visibility is given for an entry.
    synthetic: The C-level visibility of this entry ('false', 'true').
               Synthetic entries will not be generated into the native metadata
               list of entries (in C code). In general a synthetic entry is
               glued together at the Java layer from multiple visibiltity=hidden
               entries.
    hwlevel: The lowest hardware level at which the entry is guaranteed
             to be supported by the camera device. All devices with higher
             hwlevels will also include this entry. None means that the
             entry is optional on any hardware level.
    deprecated: Marks an entry as @Deprecated in the Java layer; if within an
               unreleased version this needs to be removed altogether. If applied
               to an entry from an older release, then this means the entry
               should be ignored by newer code.
    optional: a bool representing the optional attribute, which denotes the entry
              is required for hardware level full devices, but optional for other
              hardware levels.  None if not present.
    applied_optional: As optional but always valid, defaulting to False if no
                      optional attribute is present.
    tuple_values: A sequence of strings describing the tuple values,
                  None if container is not 'tuple'.
    description: A string description, or None.
    range: A string range, or None.
    units: A string units, or None.
    tags: A sequence of Tag nodes associated with this Entry.
    type_notes: A string describing notes for the type, or None.
    typedef: A Typedef associated with this Entry, or None.

  Remarks:
    Subclass Clone can be used interchangeable with an Entry,
    for when we don't care about the underlying type.

    parent and tags edges are invalid until after Metadata#construct_graph
    has been invoked.
  """
  def __init__(self, **kwargs):
    """
    Instantiate a new Entry node.

    Args:
      name: A string with the fully qualified name, e.g. 'android.shading.mode'
      type: A string describing the type, e.g. 'int32'
      kind: A string describing the kind, e.g. 'static'

    Args (if container):
      container: A string describing the container, e.g. 'array' or 'tuple'
      container_sizes: A list of string sizes if a container, or None otherwise

    Args (if container is 'tuple'):
      tuple_values: A list of tuple values, e.g. ['width', 'height']

    Args (if the 'enum' attribute is true):
      enum: A boolean, True if this is an enum, False otherwise
      enum_values: A list of value strings, e.g. ['ON', 'OFF']
      enum_optionals: A list of optional enum values, e.g. ['OFF']
      enum_notes: A dictionary of value->notes strings.
      enum_ids: A dictionary of value->id strings.

    Args (optional):
      description: A string with a description of the entry.
      range: A string with the range of the values of the entry, e.g. '>= 0'
      units: A string with the units of the values, e.g. 'inches'
      details: A string with the detailed documentation for the entry
      hal_details: A string with the HAL implementation details for the entry
      tag_ids: A list of tag ID strings, e.g. ['BC', 'V1']
      type_notes: A string with the notes for the type
      visibility: A string describing the visibility, eg 'system', 'hidden',
                  'public'
      synthetic: A bool to mark whether this entry is visible only at the Java
                 layer (True), or at both layers (False = default).
      hwlevel: A string of the HW level (one of 'legacy', 'limited', 'full')
      deprecated: A bool to mark whether this is @Deprecated at the Java layer
                 (default = False).
      optional: A bool to mark whether optional for non-full hardware devices
      typedef: A string corresponding to a typedef's name attribute.
    """

    if kwargs.get('type') is None:
      print >> sys.stderr, "ERROR: Missing type for entry '%s' kind  '%s'"     \
      %(kwargs.get('name'), kwargs.get('kind'))

    # Attributes are Read-Only, but edges may be mutated by
    # Metadata, particularly during construct_graph

    self._name = kwargs['name']
    self._type = kwargs['type']
    self._kind = kwargs['kind'] # static, dynamic, or controls

    self._init_common(**kwargs)

  @property
  def type(self):
    return self._type

  @property
  def kind(self):
    return self._kind

  @property
  def visibility(self):
    return self._visibility

  @property
  def applied_visibility(self):
    return self._visibility or 'system'

  @property
  def synthetic(self):
    return self._synthetic

  @property
  def hwlevel(self):
    return self._hwlevel

  @property
  def deprecated(self):
    return self._deprecated

  # TODO: optional should just return hwlevel is None
  @property
  def optional(self):
    return self._optional

  @property
  def applied_optional(self):
    return self._optional or False

  @property
  def name_short(self):
    return self.get_name_minimal()

  @property
  def container(self):
    return self._container

  @property
  def container_sizes(self):
    if self._container_sizes is None:
      return None
    else:
      return (i for i in self._container_sizes)

  @property
  def tuple_values(self):
    if self._tuple_values is None:
      return None
    else:
      return (i for i in self._tuple_values)

  @property
  def description(self):
    return self._description

  @property
  def range(self):
    return self._range

  @property
  def units(self):
    return self._units

  @property
  def details(self):
    return self._details

  @property
  def hal_details(self):
    return self._hal_details

  @property
  def tags(self):
    if self._tags is None:
      return None
    else:
      return (i for i in self._tags)

  @property
  def type_notes(self):
    return self._type_notes

  @property
  def typedef(self):
    return self._typedef

  @property
  def enum(self):
    return self._enum

  def _get_children(self):
    if self.enum:
      yield self.enum

  def sort_children(self):
    return None

  def is_clone(self):
    """
    Whether or not this is a Clone instance.

    Returns:
      False
    """
    return False

  def _init_common(self, **kwargs):

    self._parent = None # filled in by Metadata::_construct_entries

    self._container = kwargs.get('container')
    self._container_sizes = kwargs.get('container_sizes')

    # access these via the 'enum' prop
    enum_values = kwargs.get('enum_values')
    enum_optionals = kwargs.get('enum_optionals')
    enum_hiddens = kwargs.get('enum_hiddens')
    enum_notes = kwargs.get('enum_notes')  # { value => notes }
    enum_ids = kwargs.get('enum_ids')  # { value => notes }
    self._tuple_values = kwargs.get('tuple_values')

    self._description = kwargs.get('description')
    self._range = kwargs.get('range')
    self._units = kwargs.get('units')
    self._details = kwargs.get('details')
    self._hal_details = kwargs.get('hal_details')

    self._tag_ids = kwargs.get('tag_ids', [])
    self._tags = None  # Filled in by Metadata::_construct_tags

    self._type_notes = kwargs.get('type_notes')
    self._type_name = kwargs.get('type_name')
    self._typedef = None # Filled in by Metadata::_construct_types

    if kwargs.get('enum', False):
      self._enum = Enum(self, enum_values, enum_ids, enum_optionals,
                        enum_hiddens, enum_notes)
    else:
      self._enum = None

    self._visibility = kwargs.get('visibility')
    self._synthetic = kwargs.get('synthetic', False)
    self._hwlevel = kwargs.get('hwlevel')
    self._deprecated = kwargs.get('deprecated', False)
    self._optional = kwargs.get('optional')

    self._property_keys = kwargs

  def merge(self):
    """
    Copy the attributes into a new entry, merging it with the target entry
    if it's a clone.
    """
    return MergedEntry(self)

  # Helpers for accessing less than the fully qualified name

  def get_name_as_list(self):
    """
    Returns the name as a list split by a period.

    For example:
      entry.name is 'android.lens.info.shading'
      entry.get_name_as_list() == ['android', 'lens', 'info', 'shading']
    """
    return self.name.split(".")

  def get_inner_namespace_list(self):
    """
    Returns the inner namespace part of the name as a list

    For example:
      entry.name is 'android.lens.info.shading'
      entry.get_inner_namespace_list() == ['info']
    """
    return self.get_name_as_list()[2:-1]

  def get_outer_namespace(self):
    """
    Returns the outer namespace as a string.

    For example:
      entry.name is 'android.lens.info.shading'
      entry.get_outer_namespace() == 'android'

    Remarks:
      Since outer namespaces are non-recursive,
      and each entry has one, this does not need to be a list.
    """
    return self.get_name_as_list()[0]

  def get_section(self):
    """
    Returns the section as a string.

    For example:
      entry.name is 'android.lens.info.shading'
      entry.get_section() == ''

    Remarks:
      Since outer namespaces are non-recursive,
      and each entry has one, this does not need to be a list.
    """
    return self.get_name_as_list()[1]

  def get_name_minimal(self):
    """
    Returns only the last component of the fully qualified name as a string.

    For example:
      entry.name is 'android.lens.info.shading'
      entry.get_name_minimal() == 'shading'

    Remarks:
      entry.name_short it an alias for this
    """
    return self.get_name_as_list()[-1]

  def get_path_without_name(self):
    """
    Returns a string path to the entry, with the name component excluded.

    For example:
      entry.name is 'android.lens.info.shading'
      entry.get_path_without_name() == 'android.lens.info'
    """
    return ".".join(self.get_name_as_list()[0:-1])


class Clone(Entry):
  """
  A Node corresponding to a <clone> element. It has all the attributes of an
  <entry> element (Entry) plus the additions specified below.

  Attributes (Read-Only):
    entry: an edge to an Entry object that this targets
    target_kind: A string describing the kind of the target entry.
    name: a string of the name, same as entry.name
    kind: a string of the Kind ancestor, one of 'static', 'controls', 'dynamic'
          for the <clone> element.
    type: always None, since a clone cannot override the type.
  """
  def __init__(self, entry=None, **kwargs):
    """
    Instantiate a new Clone node.

    Args:
      name: A string with the fully qualified name, e.g. 'android.shading.mode'
      type: A string describing the type, e.g. 'int32'
      kind: A string describing the kind, e.g. 'static'
      target_kind: A string for the kind of the target entry, e.g. 'dynamic'

    Args (if container):
      container: A string describing the container, e.g. 'array' or 'tuple'
      container_sizes: A list of string sizes if a container, or None otherwise

    Args (if container is 'tuple'):
      tuple_values: A list of tuple values, e.g. ['width', 'height']

    Args (if the 'enum' attribute is true):
      enum: A boolean, True if this is an enum, False otherwise
      enum_values: A list of value strings, e.g. ['ON', 'OFF']
      enum_optionals: A list of optional enum values, e.g. ['OFF']
      enum_notes: A dictionary of value->notes strings.
      enum_ids: A dictionary of value->id strings.

    Args (optional):
      entry: An edge to the corresponding target Entry.
      description: A string with a description of the entry.
      range: A string with the range of the values of the entry, e.g. '>= 0'
      units: A string with the units of the values, e.g. 'inches'
      details: A string with the detailed documentation for the entry
      hal_details: A string with the HAL implementation details for the entry
      tag_ids: A list of tag ID strings, e.g. ['BC', 'V1']
      type_notes: A string with the notes for the type

    Remarks:
      Note that type is not specified since it has to be the same as the
      entry.type.
    """
    self._entry = entry  # Entry object
    self._target_kind = kwargs['target_kind']
    self._name = kwargs['name']  # same as entry.name
    self._kind = kwargs['kind']

    # illegal to override the type, it should be the same as the entry
    self._type = None
    # the rest of the kwargs are optional
    # can be used to override the regular entry data
    self._init_common(**kwargs)

  @property
  def entry(self):
    return self._entry

  @property
  def target_kind(self):
    return self._target_kind

  def is_clone(self):
    """
    Whether or not this is a Clone instance.

    Returns:
      True
    """
    return True

class MergedEntry(Entry):
  """
  A MergedEntry has all the attributes of a Clone and its target Entry merged
  together.

  Remarks:
    Useful when we want to 'unfold' a clone into a real entry by copying out
    the target entry data. In this case we don't care about distinguishing
    a clone vs an entry.
  """
  def __init__(self, entry):
    """
    Create a new instance of MergedEntry.

    Args:
      entry: An Entry or Clone instance
    """
    props_distinct = ['description', 'units', 'range', 'details',
                      'hal_details', 'tags', 'kind']

    for p in props_distinct:
      p = '_' + p
      if entry.is_clone():
        setattr(self, p, getattr(entry, p) or getattr(entry.entry, p))
      else:
        setattr(self, p, getattr(entry, p))

    props_common = ['parent', 'name', 'container',
                    'container_sizes', 'enum',
                    'tuple_values',
                    'type',
                    'type_notes',
                    'visibility',
                    'synthetic',
                    'hwlevel',
                    'deprecated',
                    'optional',
                    'typedef'
                   ]

    for p in props_common:
      p = '_' + p
      if entry.is_clone():
        setattr(self, p, getattr(entry.entry, p))
      else:
        setattr(self, p, getattr(entry, p))
