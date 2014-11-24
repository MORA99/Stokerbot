function showAlarm(id)
{
  formstart('/settings/alarm');
	dw('<tr><td>');
	sselect("S"+id, alarms[id][0], 1)
	dw('</td><td>');
	sels('T'+id);
	opt('1', alarms[id][1], '<');
	opt('2', alarms[id][1], '=');
	opt('3', alarms[id][1], '>');
	sele();
	dw('</td><td>');
	tf('V'+id, alarms[id][2], 2);
	dw('</td><td>');
	sels('D'+id);
	opt(1, alarms[id][3], 1);
	opt(2, alarms[id][3], 2);
	opt(3, alarms[id][3], 3);
	opt(4, alarms[id][3], 4);
	sele();
	dw('</td><td>');
	sels('P'+id);
	opt(1, alarms[id][4], 'HIGH');
	opt(0, alarms[id][4], 'LOW');
	sele();
	dw('</td><td>');
	cb('R'+id, alarms[id][5], 1);
	dw('</td><td>');
	cb('E'+id, alarms[id][6], 1);
	dw('</td><td>');
    dw('<input type="button" id="btn'+id+'" onClick="save('+id+');" value="Save"></form>');
	dw('</td></tr>');              
}
