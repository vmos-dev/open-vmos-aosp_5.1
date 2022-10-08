import unittest
import itertools
from unittest import TestCase
from metadata_model import *
from metadata_helpers import *
from metadata_parser_xml import *

# Simple test metadata block used by the tests below
test_metadata_xml = \
'''
<?xml version="1.0" encoding="utf-8"?>
<metadata xmlns="http://schemas.android.com/service/camera/metadata/"
xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
xsi:schemaLocation="http://schemas.android.com/service/camera/metadata/ metadata_properties.xsd">

<namespace name="testOuter1">
  <section name="testSection1">
    <controls>
      <entry name="control1" type="byte" visibility="public">
      </entry>
      <entry name="control2" type="byte" visibility="public">
      </entry>
    </controls>
    <dynamic>
      <entry name="dynamic1" type="byte" visibility="public">
      </entry>
      <entry name="dynamic2" type="byte" visibility="public">
      </entry>
      <clone entry="testOuter1.testSection1.control1" kind="controls">
      </clone>
    </dynamic>
    <static>
      <entry name="static1" type="byte" visibility="public">
      </entry>
      <entry name="static2" type="byte" visibility="public">
      </entry>
    </static>
  </section>
</namespace>
<namespace name="testOuter2">
  <section name="testSection2">
    <controls>
      <entry name="control1" type="byte" visibility="public">
      </entry>
      <entry name="control2" type="byte" visibility="public">
      </entry>
    </controls>
    <dynamic>
      <entry name="dynamic1" type="byte" visibility="public">
      </entry>
      <entry name="dynamic2" type="byte" visibility="public">
      </entry>
      <clone entry="testOuter2.testSection2.control1" kind="controls">
      </clone>
    </dynamic>
    <static>
      <namespace name="testInner2">
        <entry name="static1" type="byte" visibility="public">
        </entry>
        <entry name="static2" type="byte" visibility="public">
        </entry>
      </namespace>
    </static>
  </section>
</namespace>
</metadata>
'''

class TestHelpers(TestCase):

  def test_enum_calculate_value_string(self):
    def compare_values_against_list(expected_list, enum):
      for (idx, val) in enumerate(expected_list):
        self.assertEquals(val,
                          enum_calculate_value_string(list(enum.values)[idx]))

    plain_enum = Enum(parent=None, values=['ON', 'OFF'])

    compare_values_against_list(['0', '1'],
                                plain_enum)

    ###
    labeled_enum = Enum(parent=None, values=['A', 'B', 'C'], ids={
      'A': '12345',
      'B': '0xC0FFEE',
      'C': '0xDEADF00D'
    })

    compare_values_against_list(['12345', '0xC0FFEE', '0xDEADF00D'],
                                labeled_enum)

    ###
    mixed_enum = Enum(parent=None,
                      values=['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'],
                      ids={
                        'C': '0xC0FFEE',
                        'E': '123',
                        'G': '0xDEADF00D'
                      })

    expected_values = ['0', '1', '0xC0FFEE', '0xC0FFEF', '123', '124',
                       '0xDEADF00D',
                       '0xDEADF00E']

    compare_values_against_list(expected_values, mixed_enum)

  def test_enumerate_with_last(self):
    empty_list = []

    for (x, y) in enumerate_with_last(empty_list):
      self.fail("Should not return anything for empty list")

    single_value = [1]
    for (x, last) in enumerate_with_last(single_value):
      self.assertEquals(1, x)
      self.assertEquals(True, last)

    multiple_values = [4, 5, 6]
    lst = list(enumerate_with_last(multiple_values))
    self.assertListEqual([(4, False), (5, False), (6, True)], lst)

  def test_filter_tags(self):
    metadata = MetadataParserXml(test_metadata_xml, 'metadata_helpers_test.py').metadata

    test_text = \
'''
In the unlikely event of a
water landing, testOuter1.testSection1.control1 will deploy.
If testOuter2.testSection2.testInner2.static1,
then testOuter1.testSection1.
dynamic1 will ensue. That should be avoided if testOuter2.testSection2.
Barring issues, testOuter1.testSection1.dynamic1, and testOuter2.testSection2.control1.
In the third instance of testOuter1.testSection1.control1
we will take the other option.
If the path foo/android.testOuter1.testSection1.control1/bar.txt exists, then oh well.
'''
    def filter_test(node):
      return '*'

    def summary_test(node_set):
      text = "*" * len(node_set) + "\n"
      return text

    expected_text = \
'''
In the unlikely event of a
water landing, * will deploy.
If *,
then * will ensue. That should be avoided if testOuter2.testSection2.
Barring issues, *, and *.
In the third instance of *
we will take the other option.
If the path foo/android.testOuter1.testSection1.control1/bar.txt exists, then oh well.
****
'''
    result_text = filter_tags(test_text, metadata, filter_test, summary_test)

    self.assertEqual(result_text, expected_text)

  def test_wbr(self):
    wbr_string = "<wbr/>"
    wbr_gen = itertools.repeat(wbr_string)

    # No special characters, do nothing
    self.assertEquals("no-op", wbr("no-op"))
    # Insert WBR after characters in [ '.', '/', '_' ]
    self.assertEquals("word.{0}".format(wbr_string), wbr("word."))
    self.assertEquals("word/{0}".format(wbr_string), wbr("word/"))
    self.assertEquals("word_{0}".format(wbr_string), wbr("word_"))

    self.assertEquals("word.{0}break".format(wbr_string), wbr("word.break"))
    self.assertEquals("word/{0}break".format(wbr_string), wbr("word/break"))
    self.assertEquals("word_{0}break".format(wbr_string), wbr("word_break"))

    # Test words with more components
    self.assertEquals("word_{0}break_{0}again".format(wbr_string),
                      wbr("word_break_again"))
    self.assertEquals("word_{0}break_{0}again_{0}emphasis".format(wbr_string),
                      wbr("word_break_again_emphasis"))

    # Words with 2 or less subcomponents are ignored for the capital letters
    self.assertEquals("word_{0}breakIgnored".format(wbr_string),
                      wbr("word_breakIgnored"))
    self.assertEquals("wordIgnored".format(wbr_string),
                      wbr("wordIgnored"))

    # Words with at least 3 sub components get word breaks before caps
    self.assertEquals("word_{0}break_{0}again{0}Capitalized".format(wbr_string),
                      wbr("word_break_againCapitalized"))
    self.assertEquals("word.{0}break.{0}again{0}Capitalized".format(wbr_string),
                      wbr("word.break.againCapitalized"))
    self.assertEquals("a.{0}b{0}C.{0}d{0}E.{0}f{0}G".format(wbr_string),
                      wbr("a.bC.dE.fG"))

    # Don't be overly aggressive with all caps
    self.assertEquals("TRANSFORM_{0}MATRIX".format(wbr_string),
                      wbr("TRANSFORM_MATRIX"))

    self.assertEquals("SCENE_{0}MODE_{0}FACE_{0}PRIORITY".format(wbr_string),
                      wbr("SCENE_MODE_FACE_PRIORITY"))

    self.assertEquals("android.{0}color{0}Correction.{0}mode is TRANSFORM_{0}MATRIX.{0}".format(wbr_string),
                      wbr("android.colorCorrection.mode is TRANSFORM_MATRIX."))

    self.assertEquals("The overrides listed for SCENE_{0}MODE_{0}FACE_{0}PRIORITY are ignored".format(wbr_string),
                      wbr("The overrides listed for SCENE_MODE_FACE_PRIORITY are ignored"));

  def test_dedent(self):
    # Remove whitespace from 2nd and 3rd line (equal ws)
    self.assertEquals("bar\nline1\nline2", dedent("bar\n  line1\n  line2"))
    # Remove whitespace from all lines (1st line ws < 2/3 line ws)
    self.assertEquals("bar\nline1\nline2", dedent(" bar\n  line1\n  line2"))
    # Remove some whitespace from 2nd line, all whitespace from other lines
    self.assertEquals("bar\n  line1\nline2", dedent(" bar\n    line1\n  line2"))

if __name__ == '__main__':
    unittest.main()
