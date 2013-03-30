/*jshint jquery:true browser:true */

(function (daedalus) {'use strict';
  var $dlistOutput;
  var $dlistState;
  var $dlistScrub;
  var debugBailAfter = -1;
  var debugNumOps = 0;

  var state = {};

  daedalus.init = function () {

    $dlistOutput = $('#dlist');
    $dlistState = $('.dl-state');

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

        $.post('/dldebugger?dump', function (ret) {
          var $pre = $('<pre />');
          $pre.html(ret);
          $dlistOutput.html($pre);
        });
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

      var $instr = $dlistOutput.find('#I' + debugBailAfter );
      if ($instr.length) {
        $dlistOutput.scrollTop($dlistOutput.scrollTop() + $instr.position().top -
                               $dlistOutput.height()/2 + $instr.height()/2);
      }
      $dlistOutput.find('.hle-instr').removeAttr('style');
      $instr.css('background-color', 'rgb(255,255,204)');

      $.post('/dldebugger?scrub=' + t, function(data) {
        $('#screen').attr({src: '/dldebugger?screen'});

        state               = data;
        state.rdpOtherModeH = parseInt(data.rdpOtherModeH, 16);
        state.rdpOtherModeL = parseInt(data.rdpOtherModeL, 16);
        state.combine.hi    = parseInt(data.combine.hi, 16);
        state.combine.lo    = parseInt(data.combine.lo, 16);
        state.fillColor     = parseInt(data.fillColor, 16);
        state.envColor      = parseInt(data.envColor, 16);
        state.primColor     = parseInt(data.primColor, 16);
        state.blendColor    = parseInt(data.blendColor, 16);
        state.fogColor      = parseInt(data.fogColor, 16);

        updateStateUI();
      });
  }


  function makeColourText(r,g,b,a) {
    var t = r + ', ' + g + ', ' + b + ', ' + a;

    if ((r<128 && g<128) ||
        (g<128 && b<128) ||
        (b<128 && r<128)) {
      return '<span style="color: white; background-color: rgb(' + r + ',' + g + ',' + b + ')">' + t + '</span>';
    }
    return '<span style="background-color: rgb(' + r + ',' + g + ',' + b + ')">' + t + '</span>';
  }

  function makeColorTextRGBA(col) {
    var r = (col >>> 24) & 0xff;
    var g = (col >>> 16) & 0xff;
    var b = (col >>>  8) & 0xff;
    var a = (col       ) & 0xff;

    return makeColourText(r,g,b,a);
  }

  function makeColorTextABGR(col) {
    var r = (col       ) & 0xff;
    var g = (col >>>  8) & 0xff;
    var b = (col >>> 16) & 0xff;
    var a = (col >>> 24) & 0xff;

    return makeColourText(r,g,b,a);
  }

  function updateStateUI() {

    //$dlistState.find('#dl-geometrymode-content').html(buildStateTab());
    //$dlistState.find('#dl-vertices-content').html(buildVerticesTab());
    $dlistState.find('#dl-textures-content').html(buildTexturesTab());
    $dlistState.find('#dl-combiner-content').html(buildCombinerTab());
    //$dlistState.find('#dl-rdp-content').html(buildRDPTab());
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

  //G_SETOTHERMODE_H shift count
  var G_MDSFT_BLENDMASK       = 0;
  var G_MDSFT_ALPHADITHER     = 4;
  var G_MDSFT_RGBDITHER       = 6;
  var G_MDSFT_COMBKEY         = 8;
  var G_MDSFT_TEXTCONV        = 9;
  var G_MDSFT_TEXTFILT        = 12;
  var G_MDSFT_TEXTLUT         = 14;
  var G_MDSFT_TEXTLOD         = 16;
  var G_MDSFT_TEXTDETAIL      = 17;
  var G_MDSFT_TEXTPERSP       = 19;
  var G_MDSFT_CYCLETYPE       = 20;
  var G_MDSFT_COLORDITHER     = 22;
  var G_MDSFT_PIPELINE        = 23;

  var G_PM_MASK     = 1 << G_MDSFT_PIPELINE;
  var G_CYC_MASK    = 3 << G_MDSFT_CYCLETYPE;
  var G_TP_MASK     = 1 << G_MDSFT_TEXTPERSP;
  var G_TD_MASK     = 3 << G_MDSFT_TEXTDETAIL;
  var G_TL_MASK     = 1 << G_MDSFT_TEXTLOD;
  var G_TT_MASK     = 3 << G_MDSFT_TEXTLUT;
  var G_TF_MASK     = 3 << G_MDSFT_TEXTFILT;
  var G_TC_MASK     = 7 << G_MDSFT_TEXTCONV;
  var G_CK_MASK     = 1 << G_MDSFT_COMBKEY;
  var G_CD_MASK     = 3 << G_MDSFT_RGBDITHER;
  var G_AD_MASK     = 3 << G_MDSFT_ALPHADITHER;

  var cycleTypeValues = {
    G_CYC_1CYCLE:     0 << G_MDSFT_CYCLETYPE,
    G_CYC_2CYCLE:     1 << G_MDSFT_CYCLETYPE,
    G_CYC_COPY:       2 << G_MDSFT_CYCLETYPE,
    G_CYC_FILL:       3 << G_MDSFT_CYCLETYPE
  };

  function getCycleType() {
    return state.rdpOtherModeH & G_CYC_MASK;
  }


  function getClampMirrorWrapText(clamp, mirror) {
    if (clamp && mirror) return 'G_TX_MIRROR|G_TX_CLAMP';
    if (clamp)           return 'G_TX_CLAMP';
    if (mirror)          return 'G_TX_MIRROR';

    return 'G_TX_WRAP';
  }

  function buildTexturesTab() {
    var $d = $('<div />');

    $d.append(buildTilesTable());

    // var i, $t;
    // for (i = 0; i < 8; ++i) {
    //   $t = buildTexture(i);
    //   if ($t) {
    //     $d.append($t);
    //   }
    // }

    return $d;
  }

  function buildTilesTable() {
    var tiles = state.tiles;
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

  function buildColorsTable() {
    var $table = $('<table class="table table-condensed" style="width: auto;"></table>');

    var colors =[
      'fillColor',
      'envColor',
      'primColor',
      'blendColor',
      'fogColor'
    ];

    var i;
    for (i = 0; i < colors.length; ++i) {
      var col = state[colors[i]];
      $('<tr><td>' + colors[i] + '</td><td>' + makeColorTextABGR( col ) + '</td></tr>').appendTo($table);
    }

    return $table;
  }

  function buildCombinerTab() {

    var $p = $('<pre class="combine"></pre>');

    $p.append(getDefine(cycleTypeValues, getCycleType()) + '\n');
    $p.append(buildColorsTable());
    $p.append(decodeSetCombine(state.combine.hi, state.combine.lo));

    return $p;
  }

  var kMulInputRGB = [
    'Combined    ', 'Texel0      ',
    'Texel1      ', 'Primitive   ',
    'Shade       ', 'Env         ',
    'KeyScale    ', 'CombinedAlph',
    'Texel0_Alpha', 'Texel1_Alpha',
    'Prim_Alpha  ', 'Shade_Alpha ',
    'Env_Alpha   ', 'LOD_Frac    ',
    'PrimLODFrac ', 'K5          ',
    '0           ', '0           ',
    '0           ', '0           ',
    '0           ', '0           ',
    '0           ', '0           ',
    '0           ', '0           ',
    '0           ', '0           ',
    '0           ', '0           ',
    '0           ', '0           '
  ];
  var kSubAInputRGB = [
    'Combined    ', 'Texel0      ',
    'Texel1      ', 'Primitive   ',
    'Shade       ', 'Env         ',
    '1           ', 'Noise       ',
    '0           ', '0           ',
    '0           ', '0           ',
    '0           ', '0           ',
    '0           ', '0           '
  ];
  var kSubBInputRGB = [
    'Combined    ', 'Texel0      ',
    'Texel1      ', 'Primitive   ',
    'Shade       ', 'Env         ',
    'KeyCenter   ', 'K4          ',
    '0           ', '0           ',
    '0           ', '0           ',
    '0           ', '0           ',
    '0           ', '0           '
  ];
  var kAddInputRGB = [
    'Combined    ', 'Texel0      ',
    'Texel1      ', 'Primitive   ',
    'Shade       ', 'Env         ',
    '1           ', '0           '
  ];


  var kSubInputA = [
    'Combined    ', 'Texel0      ',
    'Texel1      ', 'Primitive   ',
    'Shade       ', 'Env         ',
    'PrimLODFrac ', '0           '
  ];
  var kMulInputA = [
    'Combined    ', 'Texel0      ',
    'Texel1      ', 'Primitive   ',
    'Shade       ', 'Env         ',
    '1           ', '0           '
  ];
  var kAddInputA = [
    'Combined    ', 'Texel0      ',
    'Texel1      ', 'Primitive   ',
    'Shade       ', 'Env         ',
    '1           ', '0           '
  ];

  function decodeSetCombine(mux0, mux1) {
      //
      var aRGB0  = (mux0>>>20)&0x0F; // c1 c1    // a0
      var bRGB0  = (mux1>>>28)&0x0F; // c1 c2    // b0
      var cRGB0  = (mux0>>>15)&0x1F; // c1 c3    // c0
      var dRGB0  = (mux1>>>15)&0x07; // c1 c4    // d0

      var aA0    = (mux0>>>12)&0x07; // c1 a1    // Aa0
      var bA0    = (mux1>>>12)&0x07; // c1 a2    // Ab0
      var cA0    = (mux0>>> 9)&0x07; // c1 a3    // Ac0
      var dA0    = (mux1>>> 9)&0x07; // c1 a4    // Ad0

      var aRGB1  = (mux0>>> 5)&0x0F; // c2 c1    // a1
      var bRGB1  = (mux1>>>24)&0x0F; // c2 c2    // b1
      var cRGB1  = (mux0>>> 0)&0x1F; // c2 c3    // c1
      var dRGB1  = (mux1>>> 6)&0x07; // c2 c4    // d1

      var aA1    = (mux1>>>21)&0x07; // c2 a1    // Aa1
      var bA1    = (mux1>>> 3)&0x07; // c2 a2    // Ab1
      var cA1    = (mux1>>>18)&0x07; // c2 a3    // Ac1
      var dA1    = (mux1>>> 0)&0x07; // c2 a4    // Ad1

      var decoded = '';

      decoded += 'RGB0 = (' + kSubAInputRGB[aRGB0] + ' - ' + kSubBInputRGB[bRGB0] + ') * ' + kMulInputRGB[cRGB0] + ' + ' + kAddInputRGB[dRGB0] + '\n';
      decoded += '  A0 = (' + kSubInputA   [  aA0] + ' - ' + kSubInputA   [  bA0] + ') * ' + kMulInputA  [  cA0] + ' + ' + kAddInputA  [  dA0] + '\n';
      decoded += 'RGB1 = (' + kSubAInputRGB[aRGB1] + ' - ' + kSubBInputRGB[bRGB1] + ') * ' + kMulInputRGB[cRGB1] + ' + ' + kAddInputRGB[dRGB1] + '\n';
      decoded += '  A1 = (' + kSubInputA   [  aA1] + ' - ' + kSubInputA   [  bA1] + ') * ' + kMulInputA  [  cA1] + ' + ' + kAddInputA  [  dA1] + '\n';

      return decoded;
  }


}(window.daedalus = window.daedalus || {}));

$(document).ready(function(){
  daedalus.init();
});
