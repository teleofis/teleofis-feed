'use strict';
'require form';
'require view';
'require uci';
'require rpc';
'require tools.widgets as widgets';

return view.extend({
	render: function() {
		var m, s, o;

		m = new form.Map('hotplug', _('Interface Rules'));
		m.description = _('Running commands when the hotplug trigger occurs. <br/>\
			<br/>\
			ACTION - ifdown, ifup, ifup-failed, ifupdate, free, reload, iflink, create <br/>\
			INTERFACE - Name of the logical interface which went up or down (e.g. wan or lan) <br/>\
			DEVICE - Name of the physical device which went up or down (e.g. eth0 or br-lan or pppoe-wan), when applicable <br/>\
			<br/>\
			More information about iface events and variables here: <a href="https://openwrt.org/docs/guide-user/base-system/hotplug#iface">hotplug#iface</a>');

		s = m.section(form.GridSection, 'hotplug');
		s.addremove = true;
		s.nodescriptions = true;

		o = s.option(form.TextValue, 'description', _('Description'));

		o = s.option(form.Flag, 'enabled', _('Enabled'));
		o.rmempty = false;
		o.editable = true;

		o = s.option(form.ListValue, 'action', _('$ACTION'), _('Hotplug event'));
		o.default = "mode1";
		o.value("ifup", _("iface up (ifup)"));
		o.value("ifdown", _("iface down (ifdown)"));
		o.value("ifup-failed", _("iface up failed (ifup-failed)"));
		o.value("ifupdate", _("iface changed (ifupdate)"));
		o.value("free", _("iface removed (free)"));
		o.value("reload", _("iface reload (reload)"));
		o.value("iflink", _("Iface received link (iflink)"));
		o.value("create", _("iface created (create)"));

		o = s.option(widgets.NetworkSelect, 'interface', _('$INTERFACE'), _('Interface'));
		o.rmempty = false;
		o.textvalue = function(section_id) {
			return uci.get('hotplug', section_id, 'interface');
		}

		o = s.option(form.Value, 'command', _('Command to execute'));

		return m.render();
	}
});
