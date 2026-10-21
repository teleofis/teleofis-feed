IOLINESD_PROG=/sbin/iolinesd
IOLINESD_CONF=/var/etc/iolinesd.conf

iolinesd_add_line() {
	local name poll threshold hysteresis edge command precommand source scale interval hotplug
	local mode enabled oc_on oc_off dc_on dc_off

	config_get_bool poll "$1" poll 0
	config_get_bool enabled "$1" enabled 0

	config_get name "$1" name "$1"
	config_get threshold "$1" threshold
	config_get hysteresis "$1" hysteresis
	config_get edge "$1" edge
	config_get command "$1" command
	config_get precommand "$1" precommand
	config_get source "$1" source
	config_get scale "$1" scale
	config_get interval "$1" interval
	config_get hotplug "$1" hotplug
	config_get mode "$1" mode
	config_get oc_on "$1" oc_on
	config_get oc_off "$1" oc_off
	config_get dc_on "$1" dc_on
	config_get dc_off "$1" dc_off

	case "$mode" in
		mode1) mode=adc ;;
		mode2) mode=dry_contact ;;
		mode3) mode=oc ;;
		*) mode= ;;
	esac

	if [ -z "$source" ]; then
		logger -t iolinesd "$name: no source, line skipped"
		return 0
	fi

	{
		echo ""
		echo "[$name]"
		echo "source    = $source"
		echo "poll      = $poll"
		[ -n "$scale" ] && echo "scale     = $scale"
		[ -n "$threshold" ] && echo "threshold = $threshold"
		[ -n "$hysteresis" ] && echo "hysteresis = $hysteresis"
		[ -n "$interval" ] && echo "interval  = $interval"
		[ -n "$precommand" ] && echo "precommand = $precommand"
		[ -n "$command" ] && echo "command   = $command"
		[ -n "$edge" ] && echo "edge      = $edge"
		[ -n "$hotplug" ] && echo "hotplug   = $hotplug"
		[ -n "$mode" ] && echo "mode      = $mode"
		echo "enabled   = $enabled"
		[ -n "$oc_on" ] && echo "oc_on     = $oc_on"
		[ -n "$oc_off" ] && echo "oc_off    = $oc_off"
		[ -n "$dc_on" ] && echo "dc_on     = $dc_on"
		[ -n "$dc_off" ] && echo "dc_off    = $dc_off"
	} >> "$IOLINESD_CONF"
}

iolinesd_generate_conf() {
	mkdir -p /var/etc
	echo "# Generated from /etc/config/iolines by /etc/init.d/iolines" > "$IOLINESD_CONF"

	config_foreach iolinesd_add_line io
}
