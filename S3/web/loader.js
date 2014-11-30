function require(jspath) {
    document.write('<script type="text/javascript" src="'+jspath+'"><\/script>');
}

require("/fmu.js");
require("/alist.js");
require("/alarms.js");
require("/slist.js");
document.write('<link rel="stylesheet" type="text/css" href="/style.css"/>');

function menu(major,minor) {
dw('<div id="header">Stokerbot S3 - Firmware 2.14<br><a href="/">Home</a>&nbsp;&nbsp;&nbsp;<a href="/settings/general/">Settings<a>&nbsp;&nbsp;&nbsp;<a href="/settings/net/">Network</a>&nbsp;&nbsp;&nbsp;<a href="/settings/webclient/">Uploader</a>&nbsp;&nbsp;&nbsp;<a href="/settings/io/">I/O</a>&nbsp;&nbsp;&nbsp;<a href="/settings/alarm/">Alarms</a></div><br><center>'); 
}