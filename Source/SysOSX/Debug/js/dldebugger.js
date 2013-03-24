/*jshint jquery:true browser:true */

(function (daedalus) {'use strict';
  daedalus.init = function () {

    var $d = $('<div />');
    var $stop = $('<button>Stop</button>');
    $d.append($stop);

    $stop.click(function () {
      $.post('/dldebugger?action=stop');
    });

    $('body').append($d);

  };

}(window.daedalus = window.daedalus || {}));

$(document).ready(function(){
  daedalus.init();
});
