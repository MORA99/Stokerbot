function formstart(url){dw('<form method="GET" action="'+url+'">')}

function sselect(name, selected, choose)
{
	sels(name);

	if (choose == 1) opt('', '-', 'Choose one');

	for (var i=0; i<sensors.length; i++)
		opt(sensors[i][0], selected, sensors[i][1]);

	sele();
}

function menu(major,minor) {
dw('<div id="header">Stokerbot S3 - Firmware 2.14<br><a href="/">Home</a>&nbsp;&nbsp;&nbsp;<a href="/settings/general/">Settings<a>&nbsp;&nbsp;&nbsp;<a href="/settings/net/">Network</a>&nbsp;&nbsp;&nbsp;<a href="/settings/io/">I/O</a>&nbsp;&nbsp;&nbsp;<a href="/settings/alarm/">Alarms</a></div><br><center>'); 
}
