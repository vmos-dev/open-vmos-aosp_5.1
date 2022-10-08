## -*- coding: utf-8 -*-
<?xml version="1.0" encoding="UTF-8"?>
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
<metadata
    xmlns="http://schemas.android.com/service/camera/metadata/"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xsi:schemaLocation="http://schemas.android.com/service/camera/metadata/ metadata_properties.xsd">

<tags>
% for tag in metadata.tags:
  % if tag.description and tag.description.strip():
  <tag id="${tag.id}">${tag.description | x}</tag>
  % else:
  <tag id="${tag.id}"><!-- TODO: fill the tag description --></tag>
  % endif
% endfor
</tags>

<types>
% for typedef in metadata.types:
  <typedef name="${typedef.name}">
    % for (language, klass) in typedef.languages.iteritems():
      <language name="${language}">${klass | h}</language>
    % endfor
  </typedef>
% endfor
</types>

% for root in metadata.outer_namespaces:
<namespace name="${root.name}">
  % for section in root.sections:
  <section name="${section.name}">

    % if section.description is not None:
      <description>${section.description}</description>
    % endif

    % for kind in section.kinds: # dynamic,static,controls
      <${kind.name}>

        <%def name="insert_body(node)">
            % for nested in node.namespaces:
                ${insert_namespace(nested)}
            % endfor

            % for entry in node.entries:
                ${insert_entry(entry)}
            % endfor
        </%def>

        <%def name="insert_namespace(namespace)">
        <namespace name="${namespace.name}">
            ${insert_body(namespace)}
        </namespace>
        </%def>

        <%def name="insert_entry(prop)">
        % if prop.is_clone():
            <clone entry="${prop.name}" kind="${prop.target_kind}">

              % if prop.details is not None:
                <details>${prop.details}</details>
              % endif

              % if prop.hal_details is not None:
                <hal_details>${prop.hal_details}</hal_details>
              % endif

              % for tag in prop.tags:
                <tag id="${tag.id}" />
              % endfor

            </clone>
        % else:
            <entry name="${prop.name_short}" type="${prop.type}"
          % if prop.visibility:
                visibility="${prop.visibility}"
          % endif
          % if prop.synthetic:
                synthetic="true"
          % endif
          % if prop.deprecated:
                deprecated="true"
          % endif
          % if prop.optional:
                optional="${str(prop.optional).lower()}"
          % endif
          % if prop.enum:
                enum="true"
          % endif
          % if prop.type_notes is not None:
                type_notes="${prop.type_notes}"
          % endif
          % if prop.container is not None:
                container="${prop.container}"
          % endif

          % if prop.typedef is not None:
                typedef="${prop.typedef.name}"
          % endif

          % if prop.hwlevel:
                hwlevel="${prop.hwlevel}"
          % endif
            >

              % if prop.container == 'array':
                <array>
                  % for size in prop.container_sizes:
                    <size>${size}</size>
                  % endfor
                </array>
              % elif prop.container == 'tuple':
                <tuple>
                  % for size in prop.container_sizes:
                    <value /> <!-- intentionally generated empty. manually fix -->
                  % endfor
                </tuple>
              % endif
              % if prop.enum:
                <enum>
                  % for value in prop.enum.values:
                      <value
                    % if value.optional:
                             optional="true"
                    % endif:
                    % if value.hidden:
                             hidden="true"
                    % endif:
                    % if value.id is not None:
                             id="${value.id}"
                    % endif
                      >${value.name}
                    % if value.notes is not None:
                             <notes>${value.notes}</notes>
                    % endif
                      </value>
                  % endfor
                </enum>
              % endif

              % if prop.description is not None:
                <description>${prop.description | x}</description>
              % endif

              % if prop.units is not None:
                <units>${prop.units | x}</units>
              % endif

              % if prop.range is not None:
                <range>${prop.range | x}</range>
              % endif

              % if prop.details is not None:
                <details>${prop.details | x}</details>
              % endif

              % if prop.hal_details is not None:
                <hal_details>${prop.hal_details | x}</hal_details>
              % endif

              % for tag in prop.tags:
                <tag id="${tag.id}" />
              % endfor

            </entry>
        % endif
        </%def>

        ${insert_body(kind)}

      </${kind.name}>
    % endfor # for each kind

  </section>
  % endfor
</namespace>
% endfor

</metadata>
