function dw(a){document.write(a)}
function f(t,n,v,s){dw('<input type="'+t+'" name="'+n+'" id="'+n+'" value="'+v+'" size='+s+">")}
function tf(a,b,c){f('text', a, b, c)}
function tfh(a,b,c){f('hidden', a, b, c)}
function formend(){dw('<br>');f('submit', '', 'Save', '');dw('</form>')}
function opt(a,b,c){dw('<option value="'+a+'" '+isSelected(a,b)+">"+c+"</option>")}
function isChecked(a){return a>0?" checked ":""}
function isCheckedR(a,b){return a==b?" checked ":""}
function isSelected(a,b){return a==b?" selected ":""}
function sels(name){dw('<select name="'+name+'" id="'+name+'">')}
function sele(name){dw('</select>')}
function isChecked(a){return a>0?" checked ":""}
function isCheckedR(a,b){return a==b?" checked ":""}
function cb(a,c,v){dw('<input type="checkbox" name="'+a+'" id="'+a+'"'+isChecked(c)+' value="'+v+'">')}
function adcset(a,b,c,cn,p){dw("<tr><td>Analog "+a+"</td><td>");dw('<select name="'+b+'">');opt(1,c,"ADC");opt(2,c,"Digital");opt(3,c,"Counter");dw("</select></td><td>");cb(cn,p,1);dw("</td></tr>")}
function dport(a,b,c,cn,p){dw("<tr><td>Digital "+a+"</td><td>");dw('<select name="'+b+'">');opt(0,c,"Output");opt(2,c,"Input");opt(3,c,"Counter");opt(4,c,"DHT22");opt(5,c,"DHT11");dw("</select></td><td>");cb(cn,p,1);dw("</td></tr>")}
function formstart(url){dw('<form method="GET" action="'+url+'">')}
function sselect(name, selected, choose)
{
	sels(name);

	if (choose == 1) opt('', '-', 'Choose one');

	for (var i=0; i<sensors.length; i++)
		opt(sensors[i][0], selected, sensors[i][1]);

	sele();
}
