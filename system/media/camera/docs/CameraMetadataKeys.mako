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
## These sections of metadata Key definitions are inserted into the middle of
## android.hardware.camera2.CameraCharacteristics, CaptureRequest, and CaptureResult.
<%page args="java_class, xml_kind" />\
    /*@O~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~
     * The key entries below this point are generated from metadata
     * definitions in /system/media/camera/docs. Do not modify by hand or
     * modify the comment blocks at the start or end.
     *~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~*/

##
## Generate a single key and docs
<%def name="generate_key(entry)">\
    /**
<%
    # Dedent fixes markdown not to generate code blocks. Then do the rest.
    description = ""
    if entry.description:
        description = dedent(entry.description) + "\n\n"
    details = ""
    if entry.details:
        details = dedent(entry.details)
    # Unconditionally add extra information if necessary
    extra_detail = generate_extra_javadoc_detail(entry)("")

    concatenated_info = description + details + extra_detail
%>\
## Glue description and details together before javadoc-izing. Otherwise @see in middle of javadoc.
${concatenated_info | javadoc(metadata)}\
  % if entry.enum and not (entry.typedef and entry.typedef.languages.get('java')):
    % for value in entry.enum.values:
     % if not value.hidden:
     * @see #${jenum_value(entry, value)}
     % endif
    % endfor
  % endif
  % if entry.deprecated:
     * @deprecated
  % endif
  % if entry.applied_visibility == 'hidden':
     * @hide
  % endif
     */
  % if entry.deprecated:
    @Deprecated
  % endif
  % if entry.applied_visibility == 'public':
    @PublicKey
  % endif
  % if entry.synthetic:
    @SyntheticKey
  % endif
    public static final Key<${jtype_boxed(entry)}> ${entry.name | jkey_identifier} =
            new Key<${jtype_boxed(entry)}>("${entry.name}", ${jkey_type_token(entry)});
</%def>\
##
## Generate a list of only Static, Controls, or Dynamic properties.
<%def name="single_kind_keys(java_name, xml_name)">\
% for outer_namespace in metadata.outer_namespaces: ## assumes single 'android' namespace
  % for section in outer_namespace.sections:
    % if section.find_first(lambda x: isinstance(x, metadata_model.Entry) and x.kind == xml_name) and \
         any_visible(section, xml_name, ('public','hidden') ):
      % for inner_namespace in get_children_by_filtering_kind(section, xml_name, 'namespaces'):
## We only support 1 level of inner namespace, i.e. android.a.b and android.a.b.c works, but not android.a.b.c.d
## If we need to support more, we should use a recursive function here instead.. but the indentation gets trickier.
        % for entry in filter_visibility(inner_namespace.merged_entries, ('hidden','public')):
${generate_key(entry)}
       % endfor
    % endfor
    % for entry in filter_visibility( \
        get_children_by_filtering_kind(section, xml_name, 'merged_entries'), \
                                         ('hidden', 'public')):
${generate_key(entry)}
    % endfor
    % endif
  % endfor
% endfor
</%def>\
##
## Static properties only
##${single_kind_keys('CameraCharacteristicsKeys', 'static')}
##
## Controls properties only
##${single_kind_keys('CaptureRequestKeys', 'controls')}
##
## Dynamic properties only
##${single_kind_keys('CaptureResultKeys', 'dynamic')}
${single_kind_keys(java_class, xml_kind)}\
    /*~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~
     * End generated code
     *~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~@~O@*/