/*
 * Copyright 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

$(document).ready(function() {
  prettyPrint();
  preventParentScrolls('nav');

  var sluggify_ = function(s) {
    return (s || '').replace(/ /g, '-').replace(/[^\w-]/g, '').toLowerCase();
  };

  $('h2, h3, h4.includetoc').each(function() {
    $(this).attr('id', 'toc_' + sluggify_($(this).data('tocid') || $(this).data('toctitle') || $(this).text()));
    $(this).click(function() {
      smoothScrollToId($(this).attr('id'));
    });
  });

  var buildNav_ = function(queries, $contentRoot, $navRoot) {
    if (!queries || !queries.length) {
      return;
    }

    $contentRoot.find(queries[0]).each(function() {
      var $navNode = $('<div>')
          .text($(this).html())
          .appendTo($navRoot);
      buildNav_(queries.splice(1), $(this), $navNode);
    });
  };

  buildNav();
});

function buildNav() {
  var currentLevel = 2;
  var $currentParent = $('nav');
  var $currentNode = null;

  $('#page-content').find('h2, h3, h4.includetoc').each(function() {
    var level = $(this).get(0).tagName.substring(1);

    if (level < currentLevel) {
      // ascend
      for (var i = 0; i < (currentLevel - level); i++) {
        $currentParent = $currentParent.parents('div.children, nav').first();
      }

    } else if (level > currentLevel) {
      // descend
      $currentParent = $('<div>')
          .addClass('children')
          .appendTo($currentNode);
    }

    var tocId = $(this).attr('id');
    var navId = tocId.replace(/toc_/, 'nav_');

    $interactionNode = $('<span>')
        .html($(this).data('toctitle') || $(this).html())
        .data('target', tocId)
        .click(function() {
          smoothScrollToId($(this).data('target'));
        });

    $currentNode = $('<div>')
        .attr('id', navId)
        .addClass('item')
        .append($interactionNode)
        .appendTo($currentParent);

    currentLevel = level;
  });

  var headerPositionCache = [];
  var rebuildHeaderPositionCache_ = function() {
    headerPositionCache = [];
    $('#page-content').find('h2, h3, h4.includetoc').each(function() {
      headerPositionCache.push({
        id: $(this).attr('id').replace(/toc_/, 'nav_'),
        top: $(this).offset().top
      });
    });
  };

  var updateSelectedNavPosition_ = function() {
    $('nav .item').removeClass('selected');
    var scrollTop = $(window).scrollTop();
    for (var i = headerPositionCache.length - 1; i >= 0; i--) {
      if (scrollTop >= headerPositionCache[i].top) {
        $('#' + headerPositionCache[i].id).addClass('selected');
        break;
      }
    }
  };

  rebuildHeaderPositionCache_();
  $(window).resize(function() {
    rebuildHeaderPositionCache_();
    updateSelectedNavPosition_();
  });

  $(window).scroll(function() {
    updateSelectedNavPosition_();
  });
}

function smoothScrollToId(id) {
  var $target = $('#' + id);
  $('body').animate({ scrollTop: $target.offset().top }, 200, 'swing', function() {
    document.location.hash = id;
  });
}

// Based on http://stackoverflow.com/questions/5802467/prevent-scrolling-of-parent-element
function preventParentScrolls($el) {
  $($el).on('DOMMouseScroll mousewheel', function(ev) {
    var $this = $(this),
        scrollTop = this.scrollTop,
        scrollHeight = this.scrollHeight,
        height = $this.height(),
        delta = (ev.type == 'DOMMouseScroll' ?
            ev.originalEvent.detail * -40 :
            ev.originalEvent.wheelDelta),
        up = delta > 0;

    if (!up && -delta > scrollHeight - height - scrollTop) {
      // Scrolling down, but this will take us past the bottom.
      $this.scrollTop(scrollHeight);
    } else if (up && delta > scrollTop) {
      // Scrolling up, but this will take us past the top.
      $this.scrollTop(0);
    } else {
      $this.scrollTop(scrollTop - delta);
    }

    ev.stopPropagation();
    ev.preventDefault();
    ev.returnValue = false;
    return false;
  });
}