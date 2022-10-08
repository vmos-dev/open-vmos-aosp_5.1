#!/usr/bin/env python

# Copyright (C) 2011 The Android Open Source Project
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

import codecs
import glob
import markdown
import os
import shutil
import string
import subprocess


# read just the title (first heading) from a source page
def get_title(raw_file):
  for line in open(raw_file, 'r'):
    if '#' in line:
      return line.strip(' #\n')
  return ''


# directory to compile the site to (will be clobbered during build!)
HTML_DIR = 'out'
# directory to look in for markdown source files
SRC_DIR = 'src'
# directory to look in for html templates
TEMPLATE_DIR = 'templates'

# filenames of templates to load, in order
TEMPLATE_LIST = ['includes', 'header', 'sidebar', 'main', 'footer']

# Step 1, concatenate the template pieces into a single template string
t = ''
for f in TEMPLATE_LIST:
  t += open(os.path.join(TEMPLATE_DIR, f), 'r').read()
template = string.Template(t)

# Step 2, rm -rf HTML_DIR if it exists, and then re-create it
if os.path.exists(HTML_DIR):
  shutil.rmtree(HTML_DIR)

os.mkdir(HTML_DIR)

# Step 3, recursively mirror SRC_DIR to HTML_DIR, directory by directory, translating *.md
category = 'home'
parents = {}
for curdir, subdirs, files in os.walk(SRC_DIR):
  def md(path):
    text = codecs.open(path, encoding='utf8').read()
    extensions = ['tables', 'def_list', 'toc(title=In This Document)']
    return markdown.markdown(text, extensions)

  print 'Processing %s...'  % (curdir,),
  # Step A: split path, and update cached category name if needed
  curdir = os.path.normpath(curdir)
  outdir = curdir.split(os.path.sep)
  outdir[0] = HTML_DIR
  if len(outdir) == 2:
    category = outdir[-1]
  outdir = os.path.join(*outdir)

  # Step B: mirror the hierarchy of immediate subdirectories
  for subdir in subdirs:
    os.mkdir(os.path.join(outdir, subdir))

  # Step C: cache the translated sidebars, keyed by parent dir, so we can do sidebar inheritance
  # FIXME: make this depth-agnostic, perhaps by caching all sidebars and moving the resolution
  # FIXME: complexity out of the datastructure and into the resolution algorithm.
  parentdir = os.path.dirname(curdir)
  if parentdir in parents:
    parent = parents[parentdir]
  else:
    parent = ('', '', '')

  if 'sidebar.md' in files:
    sidebar = md(os.path.join(curdir, 'sidebar.md'))
    del files[files.index('sidebar.md')]
  else:
    sidebar = parent[0]

  if 'sidebar2.md' in files:
    sidebar2 = md(os.path.join(curdir, 'sidebar2.md'))
    del files[files.index('sidebar2.md')]
  else:
    sidebar2 = parent[1]

  if 'sidebar3.md' in files:
    sidebar3 = md(os.path.join(curdir, 'sidebar3.md'))
    del files[files.index('sidebar3.md')]
  else:
    sidebar3 = parent[2]

  parents[curdir] = (sidebar, sidebar2, sidebar3)

  # Step D: mirror all non-*.md files, and translate (file).md files into (file).html
  for f in files:
    print ' .',
    # Note that this "absolute" filename has a root at SRC_DIR, not "/"
    absfilename = os.path.join(curdir, f)

    if f.endswith('.md'):
      main = md(absfilename)
      final = template.safe_substitute(main=main, sidebar=sidebar, sidebar2=sidebar2, \
          sidebar3=sidebar3, category=category, title=get_title(absfilename))

      html = codecs.open(os.path.join(outdir, f.replace('.md', '.html')), 'w', encoding="utf8")
      html.write(final)
    else:
      shutil.copy(absfilename, os.path.join(outdir, f))
  print

print 'Done.'
