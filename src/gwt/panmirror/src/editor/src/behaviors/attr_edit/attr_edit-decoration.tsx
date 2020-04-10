/*
 * attr_edit-decoration.tsx
 *
 * Copyright (C) 2019-20 by RStudio, PBC
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

import { Plugin, PluginKey, Transaction, EditorState } from 'prosemirror-state';
import { DecorationSet, EditorView, Decoration } from 'prosemirror-view';

import { findParentNodeOfType } from 'prosemirror-utils';

import * as React from 'react';

import { EditorUI } from '../../api/ui';
import { AttrEditOptions } from "../../api/attr_edit";
import { CommandFn } from '../../api/command';
import { AttrProps } from '../../api/ui';
import { WidgetProps, reactRenderForEditorView } from '../../api/widgets/react';
import { nodeDecorationPosition } from '../../api/widgets/decoration';
import { kResizeTransaction } from '../../api/transaction';

import { kEditAttrShortcut } from './attr_edit';
import { attrEditCommandFn } from './attr_edit-command';

import './attr_edit-decoration.css';

interface AttrEditDecorationProps extends WidgetProps {
  tag?: string;
  attrs: AttrProps;
  editFn: CommandFn;
  view: EditorView;
  ui: EditorUI;
}

const AttrEditDecoration: React.FC<AttrEditDecorationProps> = props => {
  
  const buttonTitle = `${props.ui.context.translateText('Edit Attributes')} (${kEditAttrShortcut})`;
  
  const onClick = (e: React.MouseEvent) => {
    props.editFn(props.view.state, props.view.dispatch, props.view);
  };

  return (
    <div className="pm-attr-edit-decoration pm-surface-widget-text-color " style={props.style}>
      {props.tag ? 
        <div className="attr-edit-tag attr-edit-widget pm-border-background-color">
          <div>
            {props.tag}
          </div>
        </div> 
        : null
      } 
      <div 
        className="attr-edit-button attr-edit-widget pm-border-background-color" 
        title={buttonTitle}
        onClick={onClick}
      >
        <div className="attr-edit-button-ellipses">...</div>
      </div>
    </div>
  );
};

const key = new PluginKey<DecorationSet>('attr_edit_decoration');

class AttrEditDecorationPlugin extends Plugin<DecorationSet> {
  constructor(ui: EditorUI, editors: AttrEditOptions[]) {
    let editorView: EditorView;
    super({
      key,
      view(view: EditorView) {
        editorView = view;
        return {};
      },
      state: {
        init: (_config: { [key: string]: any }) => {
          return DecorationSet.empty;
        },
        apply: (tr: Transaction, old: DecorationSet, _oldState: EditorState, newState: EditorState) => {
        
          // ignore resize transactions (view not yet updated)
          if (tr.getMeta(kResizeTransaction)) {
            return old.map(tr.mapping, tr.doc);
          }

          // node types
          const schema = newState.schema;
          const nodeTypes = editors.map(editor => editor.type(schema));

          // provide decoration if selection is contained within a heading, div, or code block
          const parentWithAttrs = findParentNodeOfType(nodeTypes)(tr.selection);
          if (parentWithAttrs) {

            // get editor options + provide defaults
            const editor = editors.find(ed=> ed.type(schema) === parentWithAttrs.node.type)!;
            editor.tags = editor.tags || ((editorNode) => {
              const attrTags = [];
              if (editorNode.attrs.id) {
                attrTags.push(`#${editorNode.attrs.id}`);
              }
              if (editorNode.attrs.classes && editorNode.attrs.classes.length) {
                attrTags.push(`.${editorNode.attrs.classes[0]}`);
              }
              return attrTags;
            });
            editor.editFn = editor.editFn || attrEditCommandFn;
            editor.offset = editor.offset || (() => 0);

            // get attrs
            const node = parentWithAttrs.node;
            const attrs = node.attrs;
  
            // create tag (if any)
            const tags = editor.tags(node);
            const tagText = tags.join(' ');
          
            // node decorator position
            const offset = editor.offset();
            const decorationPosition = nodeDecorationPosition(
              editorView, 
              parentWithAttrs,
              { // offsets
                top: -7 - offset,
                right: 5 - offset
              }
            );

            // no decorator if we couldn't get a position
            if (!decorationPosition) {
              return DecorationSet.empty;
            }

            // create a unique key to avoid recreating the decorator when the selection changes
            const specKey = `attr_edit_decoration_pos:${parentWithAttrs.pos};tag:${tagText};top:${decorationPosition.style.top}`;
          
            // if the old popup already has a decoration for this key then just use it
            if (old.find(undefined, undefined, spec => spec.key === specKey).length) {
              return old.map(tr.mapping, tr.doc);
            }

            // create attr edit react component
            const attrEdit = (
              <AttrEditDecoration
                tag={tagText}
                attrs={attrs}
                editFn={editor.editFn(ui)}
                view={editorView}
                ui={ui}
                style={decorationPosition.style}
              />
            );

            // create decorator and render attr editor into it
            const decoration = window.document.createElement('div');
            reactRenderForEditorView(attrEdit, decoration, editorView);

            // return decoration
            return DecorationSet.create(tr.doc, [Decoration.widget(decorationPosition.pos, decoration, 
              { 
                key: specKey,
                ignoreSelection: true,
                stopEvent: () => {
                  return true;
                }
              })]);

          } else {
            return DecorationSet.empty;
          }

          
         
        },
      },
      props: {
        decorations: (state: EditorState) => {
          return key.getState(state);
        },
      },
    });
  }
}


export function attrEditDecorationPlugin(ui: EditorUI, editors: AttrEditOptions[]) {
  return new AttrEditDecorationPlugin(ui, editors);
}




