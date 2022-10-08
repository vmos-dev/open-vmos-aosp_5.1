## -*- coding: utf-8 -*-
##
## Copyright (C) 2013 The Android Open Source Project
##
## Licensed under the Apache License, Version 2.0 (the "License");
## you may not use this file except in compliance with the License.
## You may obtain a copy of the License at
##
##      http://www.apache.org/licenses/LICENSE-2.0
##
## Unless required by applicable law or agreed to in writing, software
## distributed under the License is distributed on an "AS IS" BASIS,
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
## See the License for the specific language governing permissions and
## limitations under the License.
##
\
## This section of enum integer definitions is inserted into
## android.hardware.camera2.CameraMetadata.
    /*@O~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~
     * The enum values below this point are generated from metadata
     * definitions in /system/media/camera/docs. Do not modify by hand or
     * modify the comment blocks at the start or end.
     *~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~*/
##
## Generate an enum's integers
<%def name="generate_enum(entry, target_class)">\
    //
    // Enumeration values for ${target_class}#${entry.name | jkey_identifier}
    //

  % for value in entry.enum.values:
    /**
    % if value.notes:
${value.notes | javadoc(metadata)}\
    % endif
     * @see ${target_class}#${entry.name | jkey_identifier}
    % if entry.applied_visibility == 'hidden' or value.hidden:
     * @hide
    %endif
     */
    public static final int ${jenum_value(entry, value)} = ${enum_calculate_value_string(value)};

  % endfor
</%def>\
##
## Generate a list of only Static, Controls, or Dynamic properties.
<%def name="single_kind_keys(xml_name, target_class)">\
% for outer_namespace in metadata.outer_namespaces: ## assumes single 'android' namespace
  % for section in outer_namespace.sections:
    % if section.find_first(lambda x: isinstance(x, metadata_model.Entry) and x.kind == xml_name) and \
         any_visible(section, xml_name, ('public','hidden') ):
      % for inner_namespace in get_children_by_filtering_kind(section, xml_name, 'namespaces'):
## We only support 1 level of inner namespace, i.e. android.a.b and android.a.b.c works, but not android.a.b.c.d
## If we need to support more, we should use a recursive function here instead.. but the indentation gets trickier.
        % for entry in filter_visibility(inner_namespace.entries, ('hidden','public')):
          % if entry.enum \
              and not (entry.typedef and entry.typedef.languages.get('java')) \
              and not entry.is_clone():
${generate_enum(entry, target_class)}\
          % endif
        % endfor
      % endfor
      % for entry in filter_visibility( \
          get_children_by_filtering_kind(section, xml_name, 'entries'), \
                                         ('hidden', 'public')):
        % if entry.enum \
             and not (entry.typedef and entry.typedef.languages.get('java')) \
             and not entry.is_clone():
${generate_enum(entry, target_class)}\
        % endif
      % endfor
    % endif
  % endfor
% endfor
</%def>\

##
## Static properties only
${single_kind_keys('static','CameraCharacteristics')}\
##
## Controls properties only
${single_kind_keys('controls','CaptureRequest')}\
##
## Dynamic properties only
${single_kind_keys('dynamic','CaptureResult')}\
    /*~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~
     * End generated code
     *~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~O@*/
