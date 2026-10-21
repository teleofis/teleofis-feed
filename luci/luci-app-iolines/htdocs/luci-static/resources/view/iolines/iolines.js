'use strict';
'require fs';
'require uci';
'require ui';
'require view';
'require form';
'require poll';

var DummyValueExt = form.DummyValue.extend({
	renderWidget: function(section_id, option_index, cfgvalue) {
		return E([], [
			E('div', {
				'class': 'cbi-value-field',
				'id': this.cbid(uci.get('iolines', section_id, 'dev')),
				'style': 'color:#c73d3d;margin-left:0'
			})
		]);
	}
});

return view.extend({
	render: function() {
		var m, s, o;
		
		m = new form.Map('iolines', _('Universal I/O lines'));
		m.description = _('ADC input: voltage measurement.<br />Dry contact input: when turned on, the analog input of the router is connected to +5V voltage via a pull-up resistor. The pull-up is required to ensure the correct operation of the dry contact input.<br />Open collector: open collector output is set into the active state.') +
			'<br />' +
			_('Monitoring: the command is run every time the measured value crosses the threshold, with the variables IO_NAME, IO_LINE, IO_PATH, IO_EDGE, IO_MODE, IO_VALUE, IO_RAW, IO_THRESHOLD and IO_HYSTERESIS. The line goes to the above state only once the value rises over the threshold plus the hysteresis, and back to the below state only once it drops under the threshold minus the hysteresis, so noise around the threshold does not produce an event on every sample. The edge selects which crossings run the command: rising (the line goes above), falling (the line goes back below) or both.');

		s = m.section(form.TableSection, 'io');
		s.anonymous = true;

		o = s.option(form.DummyValue, 'name', _('I/O port'));
		o.width = '6%';

		o = s.option(form.ListValue, 'mode', _('Mode'));
		o.width = '10%';
		o.default = "mode1";
  		o.value("mode1", _("ADC"));
  		o.value("mode2", _("Dry contact"));
  		o.value("mode3", _("Open collector (OC)"));

		o = s.option(form.Flag, 'enabled', _('Save configuration before reboot'));
		o.width = '10%';

		o = s.option(DummyValueExt, 'voltage', _('Measured voltage, mV'));
		o.width = '8%';

		o = s.option(form.Flag, 'poll', _('Monitoring'));
		o.width = '8%';

		o = s.option(form.Value, 'interval', _('Poll interval, ms'));
		o.width = '8%';
		o.datatype = 'uinteger';
		o.placeholder = '1000';
		o.depends('poll', '1');

		o = s.option(form.Value, 'threshold', _('Threshold, mV'));
		o.width = '8%';
		o.datatype = 'integer';
		o.depends('poll', '1');

		o = s.option(form.Value, 'hysteresis', _('Hysteresis, mV'));
		o.width = '8%';
		o.datatype = 'uinteger';
		o.placeholder = '0';
		o.depends('poll', '1');

		o = s.option(form.ListValue, 'edge', _('Edge'));
		o.width = '8%';
		o.default = 'both';
		o.value('both', _('Both'));
		o.value('rising', _('Rising'));
		o.value('falling', _('Falling'));
		o.depends('poll', '1');

		o = s.option(form.Value, 'command', _('Command'));
		o.width = '26%';
		o.depends('poll', '1');

		return m.render().then(function(mapEl) {
			mapEl.querySelector('.cbi-section-table').style.tableLayout = 'fixed';

			poll.add(function() {
				return fs.exec('/sbin/iolinesd', [ '-c', '/var/etc/iolinesd.conf', '-t' ]).then(function(res) {
					var t = {};
					try { t = JSON.parse(res.stdout || '{}'); } catch(e) {}
					var sections = uci.sections('iolines','io');
					for (var i = 0; i < sections.length; i++) {
						var key = sections[i].name || sections[i]['.name'];
						var el = document.getElementById('cbid.iolines.%s.voltage'.format(sections[i].dev));
						if (el)
							el.textContent = (t[key] && t[key].value != null) ? t[key].value : 'n/a';
					}
				});
			},1);

			return mapEl;
		});
	}
});
