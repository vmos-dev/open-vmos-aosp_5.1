## -*- coding: utf-8 -*-
<!DOCTYPE html>
<html>
<!-- Copyright (C) 2012 The Android Open Source Project

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

          http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
-->
<head>
  <!-- automatically generated from html.mako. do NOT edit directly -->
  <meta charset="utf-8" />
  <title>Android Camera HAL3.2 Properties</title>
  <style type="text/css">
     body { background-color: #f7f7f7; font-family: Roboto, sans-serif;}
     h1 { color: #333333; }
     h2 { color: #333333; }
     a:link { color: #258aaf; text-decoration: none}
     a:hover { color: #459aaf; text-decoration: underline }
     a:visited { color: #154a5f; text-decoration: none}
    .section { color: #eeeeee; font-size: 1.5em; font-weight: bold; background-color: #888888; padding: 0.5em 0em 0.5em 0.5em; border-width: thick thin thin thin; border-color: #111111 #777777 #777777 #777777}
    .kind { color: #eeeeee; font-size: 1.2em; font-weight: bold; padding-left: 1.5em; background-color: #aaaaaa }
    .entry { background-color: #f0f0f0 }
    .entry_cont { background-color: #f0f0f0 }
    .entries_header { background-color: #dddddd; text-align: center}

    /* toc style */
    .toc_section_header { font-size:1.3em;  }
    .toc_kind_header { font-size:1.2em;  }
    .toc_deprecated { text-decoration:line-through; }

    /* table column sizes */
    table { border-collapse:collapse; table-layout: fixed; width: 100%; word-wrap: break-word }
    td,th { border: 1px solid; border-color: #aaaaaa; padding-left: 0.5em; padding-right: 0.5em }
    .th_name { width: 20% }
    .th_units { width: 10% }
    .th_tags { width: 5% }
    .th_details { width: 25% }
    .th_type { width: 20% }
    .th_description { width: 20% }
    .th_range { width: 10% }
    td { font-size: 0.9em; }

    /* hide the first thead, we need it there only to enforce column sizes */
    .thead_dummy { visibility: hidden; }

    /* Entry flair */
    .entry_name { color: #333333; padding-left:1.0em; font-size:1.1em; font-family: monospace; vertical-align:top; }
    .entry_name_deprecated { text-decoration:line-through; }

    /* Entry type flair */
    .entry_type_name { font-size:1.1em; color: #669900; font-weight: bold;}
    .entry_type_name_enum:after { color: #669900; font-weight: bold; content:" (enum)" }
    .entry_type_visibility { font-weight: bolder; padding-left:1em}
    .entry_type_synthetic { font-weight: bolder; color: #996600; }
    .entry_type_hwlevel { font-weight: bolder; color: #000066; }
    .entry_type_deprecated { font-weight: bolder; color: #4D4D4D; }
    .entry_type_enum_name { font-family: monospace; font-weight: bolder; }
    .entry_type_enum_notes:before { content:" - " }
    .entry_type_enum_notes>p:first-child { display:inline; }
    .entry_type_enum_value:before { content:" = " }
    .entry_type_enum_value { font-family: monospace; }
    .entry ul { margin: 0 0 0 0; list-style-position: inside; padding-left: 0.5em; }
    .entry ul li { padding: 0 0 0 0; margin: 0 0 0 0;}
    .entry_range_deprecated { font-weight: bolder; }

    /* Entry tags flair */
    .entry_tags ul { list-style-type: none; }

    /* Entry details (full docs) flair */
    .entry_details_header { font-weight: bold; background-color: #dddddd;
      text-align: center; font-size: 1.1em; margin-left: 0em; margin-right: 0em; }

    /* Entry spacer flair */
    .entry_spacer { background-color: transparent; border-style: none; height: 0.5em; }

    /* TODO: generate abbr element for each tag link? */
    /* TODO for each x.y.z try to link it to the entry */

  </style>

  <style>

    {
      /* broken...
         supposedly there is a bug in chrome that it lays out tables before
         it knows its being printed, so the page-break-* styles are ignored
         */
        tr { page-break-after: always; page-break-inside: avoid; }
    }

  </style>
</head>

<%!
  import re
  from metadata_helpers import md
  from metadata_helpers import IMAGE_SRC_METADATA
  from metadata_helpers import filter_tags
  from metadata_helpers import wbr

  # insert line breaks after every two \n\n
  def br(text):
    return re.sub(r"(\r?\n)(\r?\n)", r"\1<br>\2<br>", text)

  # Convert node name "x.y.z" of kind w to an HTML anchor of form
  # <a href="#w_x.y.z">x.y.z</a>
  def html_anchor(node):
    return '<a href="#%s_%s">%s</a>' % (node.kind, node.name, node.name)

  # Render as markdown, and do HTML-doc-specific rewrites
  def md_html(text):
    return md(text, IMAGE_SRC_METADATA)

  # linkify tag names such as "android.x.y.z" into html anchors
  def linkify_tags(metadata):
    def linkify_filter(text):
      return filter_tags(text, metadata, html_anchor)
    return linkify_filter

  # Number of rows an entry will span
  def entry_cols(prop):
    cols = 1
    if prop.details: cols = cols + 2
    if prop.hal_details: cols = cols + 2
    return cols
%>

<body>
  <h1>Android Camera HAL3.2 Properties</h1>
\
<%def name="insert_toc_body(node)">
  % for nested in node.namespaces:
${    insert_toc_body(nested)}
  % endfor
  % for entry in node.merged_entries:
            <li
    % if entry.deprecated:
                class="toc_deprecated"
    % endif
            >${html_anchor(entry)}</li>
  % endfor
</%def>

  <h2>Table of Contents</h2>
  <ul class="toc">
    <li><a href="#tag_index" class="toc_section_header">Tags</a></li>
% for root in metadata.outer_namespaces:
  % for section in root.sections:
    <li>
      <span class="toc_section_header"><a href="#section_${section.name}">${section.name}</a></span>
      <ul class="toc_section">
      % for kind in section.merged_kinds: # dynamic,static,controls
        <li>
          <span class="toc_kind_header">${kind.name}</span>
          <ul class="toc_section">\
${          insert_toc_body(kind)}\
          </ul>
        </li>
      % endfor
      </ul> <!-- toc_section -->
    </li>
  % endfor
% endfor
  </ul>


  <h1>Properties</h1>
  <table class="properties">

    <thead class="thead_dummy">
      <tr>
        <th class="th_name">Property Name</th>
        <th class="th_type">Type</th>
        <th class="th_description">Description</th>
        <th class="th_units">Units</th>
        <th class="th_range">Range</th>
        <th class="th_tags">Tags</th>
      </tr>
    </thead> <!-- so that the first occurrence of thead is not
                         above the first occurrence of tr -->
% for root in metadata.outer_namespaces:
<!-- <namespace name="${root.name}"> -->
  % for section in root.sections:
  <tr><td colspan="6" id="section_${section.name}" class="section">${section.name}</td></tr>

    % if section.description is not None:
      <tr class="description"><td>${section.description}</td></tr>
    % endif

    % for kind in section.merged_kinds: # dynamic,static,controls
      <tr><td colspan="6" class="kind">${kind.name}</td></tr>

      <thead class="entries_header">
        <tr>
          <th class="th_name">Property Name</th>
          <th class="th_type">Type</th>
          <th class="th_description">Description</th>
          <th class="th_units">Units</th>
          <th class="th_range">Range</th>
          <th class="th_tags">Tags</th>
        </tr>
      </thead>

      <tbody>

        <%def name="insert_body(node)">
            % for nested in node.namespaces:
                ${insert_namespace(nested)}
            % endfor

            % for entry in node.merged_entries:
                ${insert_entry(entry)}
            % endfor
        </%def>

        <%def name="insert_namespace(namespace)">
            ${insert_body(namespace)}
        </%def>

        <%def name="insert_entry(prop)">
          <tr class="entry" id="${prop.kind}_${prop.name}">
            <td class="entry_name
              % if prop.deprecated:
                entry_name_deprecated
              % endif
             " rowspan="${entry_cols(prop)}">
              ${prop.name | wbr}
            </td>
            <td class="entry_type">
              % if prop.enum:
                <span class="entry_type_name entry_type_name_enum">${prop.type}</span>
              % else:
                <span class="entry_type_name">${prop.type}</span>
              % endif
              % if prop.container is not None:
                <span class="entry_type_container">x</span>
              % endif

              % if prop.container == 'array':
                <span class="entry_type_array">
                  ${" x ".join(prop.container_sizes)}
                </span>
              % elif prop.container == 'tuple':
                <ul class="entry_type_tuple">
                % for val in prop.tuple_values:
                  <li>${val}</li>
                % endfor
                </ul>
              % endif
              <span class="entry_type_visibility"> [${prop.applied_visibility}${" as %s" %prop.typedef.name if prop.typedef else ""}]</span>

              % if prop.synthetic:
              <span class="entry_type_synthetic">[synthetic] </span>
              % endif

              % if prop.hwlevel:
              <span class="entry_type_hwlevel">[${prop.hwlevel}] </span>
              % endif

              % if prop.deprecated:
              <span class="entry_type_deprecated">[deprecated] </span>
              % endif

              % if prop.type_notes is not None:
                <div class="entry_type_notes">${prop.type_notes | wbr}</div>
              % endif

              % if prop.enum:
                <ul class="entry_type_enum">
                  % for value in prop.enum.values:
                  <li>
                    <span class="entry_type_enum_name">${value.name}</span>
                  % if value.optional:
                    <span class="entry_type_enum_optional">[optional]</span>
                  % endif:
                  % if value.hidden:
                    <span class="entry_type_enum_optional">[hidden]</span>
                  % endif:
                  % if value.id is not None:
                    <span class="entry_type_enum_value">${value.id}</span>
                  % endif
                  % if value.notes is not None:
                    <span class="entry_type_enum_notes">${value.notes | md_html, linkify_tags(metadata), wbr}</span>
                  % endif
                  </li>
                  % endfor
                </ul>
              % endif

            </td> <!-- entry_type -->

            <td class="entry_description">
            % if prop.description is not None:
              ${prop.description | md_html, linkify_tags(metadata), wbr}
            % endif
            </td>

            <td class="entry_units">
            % if prop.units is not None:
              ${prop.units | wbr}
            % endif
            </td>

            <td class="entry_range">
            % if prop.deprecated:
              <p><span class="entry_range_deprecated">Deprecated</span>. Do not use.</p>
            % endif
            % if prop.range is not None:
              ${prop.range | md_html, linkify_tags(metadata), wbr}
            % endif
            </td>

            <td class="entry_tags">
            % if next(prop.tags, None):
              <ul class="entry_tags">
              % for tag in prop.tags:
                  <li><a href="#tag_${tag.id}">${tag.id}</a></li>
              % endfor
              </ul>
            % endif
            </td>

          </tr>
          % if prop.details is not None:
          <tr class="entries_header">
            <th class="th_details" colspan="5">Details</th>
          </tr>
          <tr class="entry_cont">
            <td class="entry_details" colspan="5">
              ${prop.details | md_html, linkify_tags(metadata), wbr}
            </td>
          </tr>
          % endif

          % if prop.hal_details is not None:
          <tr class="entries_header">
            <th class="th_details" colspan="5">HAL Implementation Details</th>
          </tr>
          <tr class="entry_cont">
            <td class="entry_details" colspan="5">
              ${prop.hal_details | md_html, linkify_tags(metadata), wbr}
            </td>
          </tr>
          % endif

          <tr class="entry_spacer"><td class="entry_spacer" colspan="6"></td></tr>
           <!-- end of entry -->
        </%def>

        ${insert_body(kind)}

      <!-- end of kind -->
      </tbody>
    % endfor # for each kind

  <!-- end of section -->
  % endfor
<!-- </namespace> -->
% endfor
  </table>

  <div class="tags" id="tag_index">
    <h2>Tags</h2>
    <ul>
    % for tag in metadata.tags:
      <li id="tag_${tag.id}">${tag.id} - ${tag.description}
        <ul class="tags_entries">
        % for prop in tag.entries:
          <li>${html_anchor(prop)} (${prop.kind})</li>
        % endfor
        </ul>
      </li> <!-- tag_${tag.id} -->
    % endfor
    </ul>
  </div>

  [ <a href="#">top</a> ]

</body>
</html>
