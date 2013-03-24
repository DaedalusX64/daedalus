/*jshint jquery:true browser:true */

(function (daedalus) {'use strict';
  daedalus.init = function () {
    alert('hi');
  };

}(window.daedalus = window.daedalus || {}));

$(document).ready(function(){
  daedalus.init();
});
