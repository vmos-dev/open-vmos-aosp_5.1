#!/usr/bin/env python
#
# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# copy related utils for all PDK scripts

import os, string, sys, shutil, zipfile

def copy_dir(src_top, dest_top, dir_name, cp_option = ""):
  """copy all the files under src_top/dir_name to dest_top/dir_name."""
  src_full_path = src_top + "/" + dir_name
  # do not create the leaf dir as cp will create it
  [mid_path, leaf_path] = dir_name.rsplit("/", 1)
  dest_full_path = dest_top + "/" + mid_path
  if not os.path.isdir(dest_full_path):
    os.makedirs(dest_full_path)
  print "copy dir ", src_full_path, " to ", dest_full_path
  os.system("cp -a " + " " + cp_option + " " + src_full_path + " " + dest_full_path)


def copy_dir_only_file(src_top, dest_top, dir_name):
  """copy only files directly under the given dir_name"""
  src_full_path = src_top + "/" + dir_name
  dest_full_path = dest_top + "/" + dir_name
  if not os.path.isdir(dest_full_path):
    os.makedirs(dest_full_path)
  children = os.listdir(src_full_path)
  for child in children:
    child_full_name = src_full_path + "/" + child
    if os.path.isfile(child_full_name):
      print "copy file ", child_full_name, " to ", dest_full_path
      os.system("cp -a " + child_full_name + " " + dest_full_path)


def copy_files(src_top, dest_top, files_name):
  """copy files from src_top to dest_top.
     Note that files_name can include directories which will be created
     under dest_top"""
  src_full_path = src_top + "/" + files_name
  # do not create the leaf dir as cp will create it
  [mid_path, leaf_path] = files_name.rsplit("/", 1)
  dest_full_path = dest_top + "/" + mid_path
  if not os.path.isdir(dest_full_path):
    os.makedirs(dest_full_path)
  print "copy files ", src_full_path, " to ", dest_full_path
  os.system("cp -a " + src_full_path + " " + dest_full_path)


def copy_file_if_exists(src_top, dest_top, file_name):
  """copy file src_top/file_name to dest_top/file_name
     returns false if such file does not exist in source."""
  src_full_name = src_top + "/" + file_name
  if not os.path.isfile(src_full_name):
    print "file " + src_full_name + " not found"
    return False
  dest_file = dest_top + "/" + file_name
  dest_dir = os.path.dirname(dest_file)
  if not os.path.isdir(dest_dir):
    os.makedirs(dest_dir)
  print "copy file ", src_full_name, " to ", dest_file
  os.system("cp -a " + src_full_name + " " +  dest_file)
  return True


def copy_file_new_name_if_exists(src_full_name, dest_dir, dest_file):
  """copy src_full_name (including dir + file name) to dest_dir/dest_file
     will be used when renaming is necessary"""
  if not os.path.isfile(src_full_name):
    print "file " + src_full_name + " not found"
    return False
  dest_full_name = dest_dir + "/" + dest_file
  if not os.path.isdir(dest_dir):
    os.makedirs(dest_dir)
  print "copy file ", src_full_name, " to ", dest_full_name
  os.system("cp -a " + src_full_name + " " + dest_full_name)
  return True

def list_files(dir_name, dir_exclusion_filter = ""):
  """recursively list all files under given dir_name directory.
     exluding subdirs ending with dir_exlusion_filter in name
     returns list of files which can be [] if there is no file"""
  file_list = []
  if dir_exclusion_filter != "" and dir_name.endswith(dir_exclusion_filter):
    return file_list
  for item in os.listdir(dir_name):
    item_full_path = dir_name + "/" + item
    # do not include symbolic link to recursion
    if os.path.islink(item_full_path) or os.path.isfile(item_full_path):
      file_list.append(item_full_path)
    elif os.path.isdir(item_full_path):
      result_list = list_files(item_full_path, dir_exclusion_filter)
      for file_name in result_list:
        file_list.append(file_name)
  return file_list

def src_newer_than_dest(src, dest):
  """return True if src file/dir is newer than dest file/dir"""
  result = True
  src_mod_time = os.path.getmtime(src)
  if os.path.isfile(dest) or os.path.isdir(dest):
    dest_mod_time = os.path.getmtime(dest)
    if dest_mod_time > src_mod_time:
      result = False
  return result

def remove_if_exists(entry):
  if os.path.exists(entry):
    os.system("rm -rf " + entry)


def list_files_in_zip(zip_file_path, no_directory = True):
  """ list all files/directories inside the given zip_file_path.
      Directories are not included if no_directory is True"""
  entry_list = []
  if not zipfile.is_zipfile(zip_file_path):
    return entry_list
  zip_file = zipfile.ZipFile(zip_file_path, 'r')
  entries =  zip_file.namelist()
  for entry in entries:
    if not no_directory or not entry.endswith("/"):
      entry_list.append(entry)

  #print entry_list
  return entry_list

def save_list(list_to_save, file_name):
  f = open(file_name, "w")
  for entry in list_to_save:
    f.write("%s\n" % entry)
  f.close()

def load_list(file_name):
  result = []
  if not os.path.isfile(file_name):
    return result

  for line in open(file_name, "r"):
    result.append(line.strip())

  #print result
  return result

def remove_files_listed(top_dir, files_list):
  top_dir_ = top_dir + "/"
  for entry in files_list:
    path = top_dir_ + entry
    print "remove " + path
    os.system("rm -f " + path)

def execute_command(command, error_msg):
  if os.system(command) != 0:
    raise RuntimeError(error_msg)
