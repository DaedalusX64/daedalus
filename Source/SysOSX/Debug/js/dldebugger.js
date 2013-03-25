/*jshint jquery:true browser:true */

(function (daedalus) {'use strict';
  var $dlistScrub;
  var debugBailAfter = -1;
  var debugNumOps = 0;

  daedalus.init = function () {

    var $d = $('<div />');

    var $img = $('<div class="row"><div class="span12"><img id="screen" src="dldebugger?screen"/></div></div>');
    $d.append($img);

    var $pad = $('<div class="span12"><div class="row" /></div>');
    $d.append($pad);

    $pad.find('div').html('&nbsp;');

    var $control_div = $('<div class="span12"><div class="row" /></div>');
    $d.append($control_div);

    var $stop = $('<button class="btn">Stop</button>');
    var $start = $('<button class="btn">Start</button>');
    var $controls = $('<div class="btn-group">');
    $controls.append($stop);
    $controls.append($start);


    $dlistScrub = $(
      '<div class="scrub" style="width:640px">' +
      '  <div class="scrub-text"></div>' +
      '  <div><input type="range" min="0" max="0" value="0" style="width:100%"/></div>' +
      '</div>');
    $dlistScrub.find('input').change(function () {
      setScrubTime($(this).val() | 0);
    });
    setScrubRange(0);

    $control_div.find('div').append($controls).append($dlistScrub);

    $stop.click(function () {
      $.post('/dldebugger?action=stop', function(ret) {
        var data = JSON.parse(ret);

        // Update the scrubber based on the new length of disassembly
        debugNumOps = data.num_ops > 0 ? (data.num_ops-1) : 0;
        setScrubRange(debugNumOps);

        // If debugBailAfter hasn't been set (e.g. by hleHalt), stop at the end of the list
        var time_to_show = (debugBailAfter == -1) ? debugNumOps : debugBailAfter;
        setScrubTime(time_to_show);
      });
    });

    $('body').find('.container').append($d);

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
        $('#screen').attr({src: '/dldebugger?screen'})
      });
  }

}(window.daedalus = window.daedalus || {}));

$(document).ready(function(){
  daedalus.init();
});
