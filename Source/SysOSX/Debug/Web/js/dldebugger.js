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

          // FIXME: setScrubTime here? At least need to set position.
        });
      });
    });
    $('#ctrl-resume').click(function () {
      $.post('/dldebugger?action=resume');
    });
    $('#ctrl-prev').click(function () {
      if (debugBailAfter > 0) {
        setScrubTime(debugBailAfter - 1);
      }
    });
    $('#ctrl-next').click(function () {
      if (debugBailAfter <  debugNumOps) {
        setScrubTime(debugBailAfter + 1);
      }
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
      $dlistOutput.find('.hle-detail').hide();
      $instr.find('.hle-detail').show();

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
    $dlistState.find('#dl-rdp-content').html(buildRDPTab());
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

 var renderModeFlags = {
    AA_EN:               0x0008,
    Z_CMP:               0x0010,
    Z_UPD:               0x0020,
    IM_RD:               0x0040,
    CLR_ON_CVG:          0x0080,
    CVG_DST_CLAMP:       0,
    CVG_DST_WRAP:        0x0100,
    CVG_DST_FULL:        0x0200,
    CVG_DST_SAVE:        0x0300,
    ZMODE_OPA:           0,
    ZMODE_INTER:         0x0400,
    ZMODE_XLU:           0x0800,
    ZMODE_DEC:           0x0c00,
    CVG_X_ALPHA:         0x1000,
    ALPHA_CVG_SEL:       0x2000,
    FORCE_BL:            0x4000,
    TEX_EDGE:            0x0000 /* used to be 0x8000 */
  };

  var blendColourSources = [
    'G_BL_CLR_IN',
    'G_BL_CLR_MEM',
    'G_BL_CLR_BL',
    'G_BL_CLR_FOG'
  ];

  var blendSourceFactors = [
    'G_BL_A_IN',
    'G_BL_A_FOG',
    'G_BL_A_SHADE',
    'G_BL_0'
  ];

  var blendDestFactors = [
    'G_BL_1MA',
    'G_BL_A_MEM',
    'G_BL_1',
    'G_BL_0'
  ];

  function blendOpText(v) {
    var m1a = (v>>>12)&0x3;
    var m1b = (v>>> 8)&0x3;
    var m2a = (v>>> 4)&0x3;
    var m2b = (v>>> 0)&0x3;

    return blendColourSources[m1a] + ',' + blendSourceFactors[m1b] + ',' + blendColourSources[m2a] + ',' + blendDestFactors[m2b];
  }

  function getRenderModeFlagsText(data) {
    var t = '';

    if (data & renderModeFlags.AA_EN)               t += '|AA_EN';
    if (data & renderModeFlags.Z_CMP)               t += '|Z_CMP';
    if (data & renderModeFlags.Z_UPD)               t += '|Z_UPD';
    if (data & renderModeFlags.IM_RD)               t += '|IM_RD';
    if (data & renderModeFlags.CLR_ON_CVG)          t += '|CLR_ON_CVG';

    var cvg = data & 0x0300;
         if (cvg === renderModeFlags.CVG_DST_CLAMP) t += '|CVG_DST_CLAMP';
    else if (cvg === renderModeFlags.CVG_DST_WRAP)  t += '|CVG_DST_WRAP';
    else if (cvg === renderModeFlags.CVG_DST_FULL)  t += '|CVG_DST_FULL';
    else if (cvg === renderModeFlags.CVG_DST_SAVE)  t += '|CVG_DST_SAVE';

    var zmode = data & 0x0c00;
         if (zmode === renderModeFlags.ZMODE_OPA)   t += '|ZMODE_OPA';
    else if (zmode === renderModeFlags.ZMODE_INTER) t += '|ZMODE_INTER';
    else if (zmode === renderModeFlags.ZMODE_XLU)   t += '|ZMODE_XLU';
    else if (zmode === renderModeFlags.ZMODE_DEC)   t += '|ZMODE_DEC';

    if (data & renderModeFlags.CVG_X_ALPHA)         t += '|CVG_X_ALPHA';
    if (data & renderModeFlags.ALPHA_CVG_SEL)       t += '|ALPHA_CVG_SEL';
    if (data & renderModeFlags.FORCE_BL)            t += '|FORCE_BL';

    var c0 = t.length > 0 ? t.substr(1) : '0';

    var blend = data >>> G_MDSFT_BLENDER;

    var c1 = 'GBL_c1(' + blendOpText(blend>>>2) + ') | GBL_c2(' + blendOpText(blend) + ') /*' + n64js.toString16(blend) + '*/';

    return c0 + ', ' + c1;
  }

  // G_SETOTHERMODE_L sft: shift count
  var G_MDSFT_ALPHACOMPARE    = 0;
  var G_MDSFT_ZSRCSEL         = 2;
  var G_MDSFT_RENDERMODE      = 3;
  var G_MDSFT_BLENDER         = 16;

  var G_AC_MASK     = 3 << G_MDSFT_ALPHACOMPARE;
  var G_ZS_MASK     = 1 << G_MDSFT_ZSRCSEL;

  function getAlphaCompareType() {
    return state.rdpOtherModeL & G_AC_MASK;
  }

  function getCoverageTimesAlpha() {
     return (state.rdpOtherModeL & renderModeFlags.CVG_X_ALPHA) !== 0;  // fragment coverage (0) or alpha (1)?
  }

  function getAlphaCoverageSelect() {
    return (state.rdpOtherModeL & renderModeFlags.ALPHA_CVG_SEL) !== 0;  // use fragment coverage * fragment alpha
  }

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

  var pipelineModeValues = {
    G_PM_1PRIMITIVE:   1 << G_MDSFT_PIPELINE,
    G_PM_NPRIMITIVE:   0 << G_MDSFT_PIPELINE
  };

  var cycleTypeValues = {
    G_CYC_1CYCLE:     0 << G_MDSFT_CYCLETYPE,
    G_CYC_2CYCLE:     1 << G_MDSFT_CYCLETYPE,
    G_CYC_COPY:       2 << G_MDSFT_CYCLETYPE,
    G_CYC_FILL:       3 << G_MDSFT_CYCLETYPE
  };

  var texturePerspValues = {
    G_TP_NONE:        0 << G_MDSFT_TEXTPERSP,
    G_TP_PERSP:       1 << G_MDSFT_TEXTPERSP
  };

  var textureDetailValues = {
    G_TD_CLAMP:       0 << G_MDSFT_TEXTDETAIL,
    G_TD_SHARPEN:     1 << G_MDSFT_TEXTDETAIL,
    G_TD_DETAIL:      2 << G_MDSFT_TEXTDETAIL
  };

  var textureLODValues = {
    G_TL_TILE:        0 << G_MDSFT_TEXTLOD,
    G_TL_LOD:         1 << G_MDSFT_TEXTLOD
  };

  var textureLUTValues = {
    G_TT_NONE:        0 << G_MDSFT_TEXTLUT,
    G_TT_RGBA16:      2 << G_MDSFT_TEXTLUT,
    G_TT_IA16:        3 << G_MDSFT_TEXTLUT
  };

  var textureFilterValues = {
    G_TF_POINT:       0 << G_MDSFT_TEXTFILT,
    G_TF_AVERAGE:     3 << G_MDSFT_TEXTFILT,
    G_TF_BILERP:      2 << G_MDSFT_TEXTFILT
  };

  var textureConvertValues = {
    G_TC_CONV:       0 << G_MDSFT_TEXTCONV,
    G_TC_FILTCONV:   5 << G_MDSFT_TEXTCONV,
    G_TC_FILT:       6 << G_MDSFT_TEXTCONV
  };

  var combineKeyValues = {
    G_CK_NONE:        0 << G_MDSFT_COMBKEY,
    G_CK_KEY:         1 << G_MDSFT_COMBKEY
  };

  var colorDitherValues = {
    G_CD_MAGICSQ:     0 << G_MDSFT_RGBDITHER,
    G_CD_BAYER:       1 << G_MDSFT_RGBDITHER,
    G_CD_NOISE:       2 << G_MDSFT_RGBDITHER,
    G_CD_DISABLE:     3 << G_MDSFT_RGBDITHER
  };

  var alphaDitherValues = {
    G_AD_PATTERN:     0 << G_MDSFT_ALPHADITHER,
    G_AD_NOTPATTERN:  1 << G_MDSFT_ALPHADITHER,
    G_AD_NOISE:       2 << G_MDSFT_ALPHADITHER,
    G_AD_DISABLE:     3 << G_MDSFT_ALPHADITHER
  };

  var alphaCompareValues = {
    G_AC_NONE:          0 << G_MDSFT_ALPHACOMPARE,
    G_AC_THRESHOLD:     1 << G_MDSFT_ALPHACOMPARE,
    G_AC_DITHER:        3 << G_MDSFT_ALPHACOMPARE
  };

  var depthSourceValues = {
    G_ZS_PIXEL:         0 << G_MDSFT_ZSRCSEL,
    G_ZS_PRIM:          1 << G_MDSFT_ZSRCSEL
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

  function buildTexture(tile_idx) {
    var tile = state.tiles[tile_idx];
    if (tile && tile.texture && tile.texture.data && tile.texture.width > 0 && tile.texture.height > 0) {
      // Copy + scale texture data.
      var scale = 8;
      var w = tile.texture.width  * scale;
      var h = tile.texture.height * scale;
      var $canvas = $( '<canvas width="' + w + '" height="' + h + '" style="background-color: black" />', {'width':w, 'height':h} );
      var dst_ctx = $canvas[0].getContext('2d');

      var dst_img_data = dst_ctx.createImageData(w, h);

      var src            = Base64.decodeArray(tile.texture.data);
      var dst            = dst_img_data.data;
      var src_row_stride = tile.texture.width*4;
      var dst_row_stride = dst_img_data.width*4;

      // Repeat last pixel across all lines
      var x;
      var y;
      for (y = 0; y < h; ++y) {

        var src_offset = src_row_stride * Math.floor(y/scale);
        var dst_offset = dst_row_stride * y;

        for (x = 0; x < w; ++x) {
          var o = src_offset + Math.floor(x/scale)*4;
          dst[dst_offset+0] = src[o+0];
          dst[dst_offset+1] = src[o+1];
          dst[dst_offset+2] = src[o+2];
          dst[dst_offset+3] = src[o+3];
          dst_offset += 4;
        }
      }

      dst_ctx.putImageData(dst_img_data, 0, 0);

      return $canvas;
    }
  }

  function buildTexturesTab() {
    var $d = $('<div />');

    $d.append(buildTilesTable());

    var i, $t;
    for (i = 0; i < 8; ++i) {
      $t = buildTexture(i);
      if ($t) {
        $d.append($t);
      }
    }

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

  function buildRDPTab() {

    var l = state.rdpOtherModeL;
    var h = state.rdpOtherModeH;
    var vals = {
      alphaCompare: getDefine(alphaCompareValues,   l & G_AC_MASK),
      depthSource:  getDefine(depthSourceValues,    l & G_ZS_MASK),
      renderMode:   getRenderModeFlagsText(l),

    //var G_MDSFT_BLENDMASK       = 0;
      alphaDither:    getDefine(alphaDitherValues,    h & G_AD_MASK),
      colorDither:    getDefine(colorDitherValues,    h & G_CD_MASK),
      combineKey:     getDefine(combineKeyValues,     h & G_CK_MASK),
      textureConvert: getDefine(textureConvertValues, h & G_TC_MASK),
      textureFilter:  getDefine(textureFilterValues,  h & G_TF_MASK),
      textureLUT:     getDefine(textureLUTValues,     h & G_TT_MASK),
      textureLOD:     getDefine(textureLODValues,     h & G_TL_MASK),
      texturePersp:   getDefine(texturePerspValues,   h & G_TP_MASK),
      textureDetail:  getDefine(textureDetailValues,  h & G_TD_MASK),
      cycleType:      getDefine(cycleTypeValues,      h & G_CYC_MASK),
      pipelineMode:   getDefine(pipelineModeValues,   h & G_PM_MASK)
    };

    var $table = $('<table class="table table-condensed" style="width: auto;"></table>');

    var $tr, i;
    for (i in vals) {
      if (vals.hasOwnProperty(i)) {
        $tr = $('<tr><td>' + i + '</td><td>' + vals[i] + '</td></tr>');
        $table.append($tr);
      }
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


  var Base64 = {
    lookup : "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=",
    decodeArray : function (str) {
      if ((str.length % 4) !== 0)
        throw 'Length should be multiple of 4 bytes';
      var outl = (str.length / 4) * 3;
      if (str.charAt(str.length-1) === '=') outl--;
      if (str.charAt(str.length-2) === '=') outl--;

      var arr = new Uint8Array(outl);
      var outi = 0;

      var i;
      for (i = 0; i < str.length; i += 4) {
        var a = this.lookup.indexOf(str.charAt(i+0));
        var b = this.lookup.indexOf(str.charAt(i+1));
        var c = this.lookup.indexOf(str.charAt(i+2));
        var d = this.lookup.indexOf(str.charAt(i+3));

        var c0 = (a << 2) | (b >>> 4);
        var c1 = ((b & 15) << 4) | (c >>> 2);
        var c2 = ((c & 3) << 6) | d;

        arr[outi++] = c0;
        if (c !== 64) {
          arr[outi++] = c1;
        }
        if (d !== 64) {
          arr[outi++] = c2;
        }
      }

      if (outi != outl)
        throw "Didn't decode the correct number of bytes";

      return arr;
    }
  };


  function padString(v,len) {
    var t = v.toString();
    while (t.length < len) {
      t = '0' + t;
    }
    return t;
  }

  function toHex(r, bits) {
    r = Number(r);
    if (r < 0) {
        r = 0xFFFFFFFF + r + 1;
    }

    var t = r.toString(16);

    if (bits) {
      var len = Math.floor(bits / 4); // 4 bits per hex char
      while (t.length < len) {
        t = '0' + t;
      }
    }

    return t;
  }

  function toString8(v) {
    return '0x' + toHex((v&0xff)>>>0, 8);
  }
  function toString16(v) {
    return '0x' + toHex((v&0xffff)>>>0, 16);
  }
  function toString32(v) {
    return '0x' + toHex(v, 32);
  }

  function toString64(hi, lo) {
    var t = toHex(lo, 32);
    var u = toHex(hi, 32);
    return '0x' + u + t;
  }

  var n64js = {};
  n64js.padString  = padString;
  n64js.toHex      = toHex;
  n64js.toString8  = toString8;
  n64js.toString16 = toString16;
  n64js.toString32 = toString32;
  n64js.toString64 = toString64;

}(window.daedalus = window.daedalus || {}));

$(document).ready(function(){
  daedalus.init();
});
