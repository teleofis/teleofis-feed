'use strict';
'require form';
'require view';
'require uci';
'require rpc';
'require ui';
'require poll';

var power_status = rpc.declare({
	object: 'powersupply',
	method: 'info',
	params: [ 'info' ],
	expect: { '': {} }
});

var DummyValueExt = form.DummyValue.extend({
	renderWidget: function(section_id, option_index, cfgvalue) {
		return E([], [
			E('input', {
				'id': this.cbid(section_id),
				'type': 'text',
				'readonly': false
			})
		]);
	}
});

return view.extend({
	render: function() {
		var m, s, o;

		m = new form.Map('powersupply', _('Power Supply'));

		s = m.section(form.NamedSection, 'system', _('System Properties'));

		o = s.option(DummyValueExt, 'input_voltage', _('Input Voltage'));
		o = s.option(DummyValueExt, 'input_voltage_threshold', _('Input Voltage Threshold'));

		o = s.option(form.ListValue, 'poe_out_power', _('PoE OUT power'));
		o.default = 'disabled';
		o.value(0,_('disabled'));
		o.value(1,_('enabled'));

		o = s.option(form.ListValue, 'poe_out_control', _('PoE OUT control'));
		o.default = 'auto';
		o.value(0,_('disabled'));
		o.value(1,_('auto'));

		o = s.option(form.ListValue, 'usb_power', _('USB power'));
		o.default = 'enabled';
		o.value(0,_('disabled'));
		o.value(1,_('enabled'));

		o = s.option(form.ListValue, 'usb_control', _('USB control'));
		o.default = 'auto';
		o.value(0,_('disabled'));
		o.value(1,_('auto'));

		o = s.option(form.ListValue, 'ext_power', _('7.5V power'));
		o.default = 'disabled';
		o.value(0,_('disabled'));
		o.value(1,_('enabled'));

		return m.render().then(function(mapEl) {
			poll.add(function() {
				return power_status().then(function(t) {
					document.getElementById('cbid.powersupply.system.input_voltage').value = t.input_voltage || 'n/a';
					document.getElementById('cbid.powersupply.system.input_voltage_threshold').value = t.voltage_threshold || 'n/a';
				});
			},1);

			return mapEl;
		});
	}
});
