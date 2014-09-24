function require(jspath) {
    document.write('<script type="text/javascript" src="'+jspath+'"><\/script>');
}

require("/fmu.js");
require("/fmu2.js");
require("/alist.js");
require("/alarms.js");
require("/slist.js");
document.write('<link rel="stylesheet" type="text/css" href="/style.css"/>');