import unittest
from unittest import TestCase
from metadata_model import *

class TestInnerNamespace(TestCase):
  def test_combine_children_by_name(self):
    #
    # Set up
    #
    kind = Kind("some_root_kind", parent=None)
    ins_outer = InnerNamespace("static", parent=kind)
    kind._namespaces = [ins_outer]

    ins1 = InnerNamespace("ins1", parent=ins_outer)
    ins1a = InnerNamespace("ins1", parent=ins_outer)  # same name deliberately
    entry1 = Entry(name="entry1", type="int32", kind="static",
                   parent=ins1)
    entry2 = Entry(name="entry2", type="int32", kind="static",
                   parent=ins1a)
    entry3 = Entry(name="entry3", type="int32", kind="static",
                   parent=ins_outer)

    ins_outer._namespaces = [ins1, ins1a]
    ins_outer._entries = [entry3]

    ins1._entries = [entry1]
    ins1a._entries = [entry2]

    #
    # Test
    #
    combined_children_namespace = ins_outer.combine_children_by_name()

    self.assertIsInstance(combined_children_namespace, InnerNamespace)
    combined_ins = [i for i in combined_children_namespace.namespaces]
    combined_ent = [i for i in combined_children_namespace.entries]

    self.assertEquals(kind, combined_children_namespace.parent)
    self.assertEquals(1, len(combined_ins))
    self.assertEquals(1, len(combined_ent))

    self.assertEquals("ins1", combined_ins[0].name)
    self.assertEquals("entry3", combined_ent[0].name)

    new_ins = combined_ins[0]
    self.assertIn(entry1, new_ins.entries)
    self.assertIn(entry2, new_ins.entries)


class TestKind(TestCase):
  def test_combine_kinds_into_single_node(self):
    #
    # Set up
    #
    section = Section("some_section", parent=None)
    kind_static = Kind("static", parent=section)
    kind_dynamic = Kind("dynamic", parent=section)
    section._kinds = [kind_static, kind_dynamic]

    ins1 = InnerNamespace("ins1", parent=kind_static)
    ins2 = InnerNamespace("ins2", parent=kind_dynamic)
    entry1 = Entry(name="entry1", type="int32", kind="static",
                   parent=kind_static)
    entry2 = Entry(name="entry2", type="int32", kind="static",
                   parent=kind_dynamic)

    kind_static._namespaces = [ins1]
    kind_static._entries = [entry1]

    kind_dynamic._namespaces = [ins2]
    kind_dynamic._entries = [entry2]

    #
    # Test
    #
    combined_kind = section.combine_kinds_into_single_node()

    self.assertEquals(section, combined_kind.parent)

    self.assertIn(ins1, combined_kind.namespaces)
    self.assertIn(ins2, combined_kind.namespaces)

    self.assertIn(entry1, combined_kind.entries)
    self.assertIn(entry2, combined_kind.entries)

  def test_combine_children_by_name(self):
    #
    # Set up
    #
    section = Section("some_section", parent=None)
    kind_static = Kind("static", parent=section)
    section._kinds = [kind_static]

    ins1 = InnerNamespace("ins1", parent=kind_static)
    ins1a = InnerNamespace("ins1", parent=kind_static)  # same name deliberately
    entry1 = Entry(name="entry1", type="int32", kind="static",
                   parent=ins1)
    entry2 = Entry(name="entry2", type="int32", kind="static",
                   parent=ins1a)
    entry3 = Entry(name="entry3", type="int32", kind="static",
                   parent=kind_static)

    kind_static._namespaces = [ins1, ins1a]
    kind_static._entries = [entry3]

    ins1._entries = [entry1]
    ins1a._entries = [entry2]

    #
    # Test
    #
    combined_children_kind = kind_static.combine_children_by_name()

    self.assertIsInstance(combined_children_kind, Kind)
    combined_ins = [i for i in combined_children_kind.namespaces]
    combined_ent = [i for i in combined_children_kind.entries]

    self.assertEquals(section, combined_children_kind.parent)
    self.assertEquals(1, len(combined_ins))
    self.assertEquals(1, len(combined_ent))

    self.assertEquals("ins1", combined_ins[0].name)
    self.assertEquals("entry3", combined_ent[0].name)

    new_ins = combined_ins[0]
    self.assertIn(entry1, new_ins.entries)
    self.assertIn(entry2, new_ins.entries)

if __name__ == '__main__':
    unittest.main()
