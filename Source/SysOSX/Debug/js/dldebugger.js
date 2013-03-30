/*jshint jquery:true browser:true */

(function (daedalus) {'use strict';
  var $dlistScrub;
  var debugBailAfter = -1;
  var debugNumOps = 0;

  daedalus.init = function () {
    var $tabs = $('#tiles');

    $dlistScrub = $('.scrub');
    $dlistScrub.find('input').change(function () {
      setScrubTime($(this).val() | 0);
    });
    setScrubRange(0);

    $('#ctrl-break').click(function () {
      $.post('/dldebugger?action=break', function(data) {
        // Update the scrubber based on the new length of disassembly
        debugNumOps = data.num_ops > 0 ? (data.num_ops-1) : 0;
        setScrubRange(debugNumOps);

        // If debugBailAfter hasn't been set (e.g. by hleHalt), stop at the end of the list
        var time_to_show = (debugBailAfter == -1) ? debugNumOps : debugBailAfter;
        setScrubTime(time_to_show);

        $.post('/dldebugger?action=dump');
      });
    });
    $('#ctrl-resume').click(function () {
      $.post('/dldebugger?action=resume');
    });
  };

  // FIXME: we should reuse n64js/hle.js :)
  function setScrubText(x, max) {
    $dlistScrub.find('.scrub-text').html('uCode op ' + x + '/' + max + '.');
  }

  function setScrubRange(max) {
    var $slider = $dlistScrub.find('input');
    $slider.attr({
      min:   0,
      max:   max,
      value: max
    });
    $slider.val(max);
    setScrubText(max, max);
  }

  function setScrubTime(t) {
      debugBailAfter = t;
      setScrubText(debugBailAfter, debugNumOps);

      // var $instr = $dlistOutput.find('#I' + debugBailAfter );

      // $dlistOutput.scrollTop($dlistOutput.scrollTop() + $instr.position().top -
      //                        $dlistOutput.height()/2 + $instr.height()/2);

      // $dlistOutput.find('.hle-instr').removeAttr('style');
      // $instr.css('background-color', 'rgb(255,255,204)');

      $.post('/dldebugger?scrub=' + t, function(ret) {
        $('#screen').attr({src: '/dldebugger?screen'});

        $('#tiles').html(buildTilesTable(ret.tiles));
      });
  }


  function getDefine(m, v) {
    for (var d in m) {
      if (m[d] === v)
        return d;
    }
    return n64js.toString32(v);
  }
  var imageFormatTypes = {
    G_IM_FMT_RGBA:    0,
    G_IM_FMT_YUV:     1,
    G_IM_FMT_CI:      2,
    G_IM_FMT_IA:      3,
    G_IM_FMT_I:       4
  };

  var imageSizeTypes = {
    G_IM_SIZ_4b:      0,
    G_IM_SIZ_8b:      1,
    G_IM_SIZ_16b:     2,
    G_IM_SIZ_32b:     3
  };
  function getClampMirrorWrapText(clamp, mirror) {
    if (clamp && mirror) return 'G_TX_MIRROR|G_TX_CLAMP';
    if (clamp)           return 'G_TX_CLAMP';
    if (mirror)          return 'G_TX_MIRROR';

    return 'G_TX_WRAP';
  }

  function buildTilesTable(tiles) {
    var tile_fields = [
      'tile #',
      'format',
      'size',
      'line',
      'tmem',
      'palette',
      'cm_s',
      'mask_s',
      'shift_s',
      'cm_t',
      'mask_t',
      'shift_t',
      'left',
      'top',
      'right',
      'bottom'
    ];

    var $table = $('<table class="table table-condensed" style="width: auto"></table>');
    var $tr = $('<tr><th>' + tile_fields.join('</th><th>') + '</th></tr>');
    $table.append($tr);

    var i;
    for (i = 0; i < tiles.length; ++i) {
      var tile = tiles[i];

      // Ignore any tiles that haven't been set up.
      if (tile.format === -1 || (tile.format === 0 && tile.size === 0)) {
        continue;
      }

      var vals = [];
      vals.push(i);
      vals.push(getDefine(imageFormatTypes, tile.format));
      vals.push(getDefine(imageSizeTypes, tile.size));
      vals.push(tile.line);
      vals.push(tile.tmem);
      vals.push(tile.palette);
      vals.push(getClampMirrorWrapText(tile.clamp_s, tile.mirror_s));
      vals.push(tile.mask_s);
      vals.push(tile.shift_s);
      vals.push(getClampMirrorWrapText(tile.clamp_t, tile.mirror_t));
      vals.push(tile.mask_t);
      vals.push(tile.shift_t);
      vals.push(tile.left   / 4.0);
      vals.push(tile.top    / 4.0);
      vals.push(tile.right  / 4.0);
      vals.push(tile.bottom / 4.0);

      $tr = $('<tr><td>' + vals.join('</td><td>') + '</td></tr>');
      $table.append($tr);
    }

    return $table;
  }

}(window.daedalus = window.daedalus || {}));

$(document).ready(function(){
  daedalus.init();
});
