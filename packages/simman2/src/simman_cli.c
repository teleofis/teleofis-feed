#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdint.h>
#include <syslog.h>
#include <getopt.h>

#include "common.h"

static const char *device, *config;

int ModemStarted(char *atdevice)
{
	// 0 - OK, -1 - not found
	return access(atdevice, F_OK);
}

char *GetSimInfo(struct settings_entry *settings)
{
	char b[64] = {0};
	modem_sim_state(b,settings);
	return strdup(b);
}

int GetAllInfo(struct settings_entry *settings)
{
	char cmd[256];
	struct modems_ops *modem = NULL;
	modem = modems_backend(settings->atdevice);
	if(modem!=NULL){
		modem->init(settings);
		fprintf(stdout, "{\n");
		fprintf(stdout, "  \"modem\": \"%s\",\n", modem_summary(modem,INFO_MODEM, settings->atdevice));
		fprintf(stdout, "  \"firmware\": \"%s\",\n", modem_summary(modem,INFO_FW, settings->atdevice));
		fprintf(stdout, "  \"imei\": \"%s\",\n", modem_summary(modem,INFO_IMEI, settings->atdevice));
		fprintf(stdout, "  \"ccid\": \"%s\",\n", modem_summary(modem,INFO_CCID, settings->atdevice));
		fprintf(stdout, "  \"imsi\": \"%s\",\n", modem_summary(modem,INFO_IMSI, settings->atdevice));
		fprintf(stdout, "  \"sim_state\": \"%s\",\n", GetSimInfo(settings));
		fprintf(stdout, "  \"pin_state\": \"%s\",\n", modem_summary(modem,INFO_PINSTAT, settings->atdevice));
		fprintf(stdout, "  \"csq\": \"%s\",\n", modem_summary(modem,INFO_SIGLEV, settings->atdevice));
		fprintf(stdout, "  \"net_reg\": \"%s\",\n", modem_summary(modem,INFO_REGSTAT, settings->atdevice));
		fprintf(stdout, "  \"net_type\": \"%s\",\n", modem_summary(modem,INFO_NETTYPE, settings->atdevice));
		fprintf(stdout, "  \"data_reg\": \"%s\",\n", modem_summary(modem,INFO_GPRSSTAT, settings->atdevice));
		fprintf(stdout, "  \"data_type\": \"%s\",\n", modem_summary(modem,INFO_PACKTYPE, settings->atdevice));
		fprintf(stdout, "  \"bs_id\": \"%s\",\n", modem_summary(modem,INFO_BASESTID, settings->atdevice));
		fprintf(stdout, "  \"band\": \"%s\"\n", modem_summary(modem,INFO_BASESTBW, settings->atdevice));
		fprintf(stdout, "}\n");
	} else
	{
		LOG("modem does not respond to AT-commands or unknown modem\n");
	}
	return 0;
}

static const char *optString = "d:a:s:x:o:cprbnimfRBSDIh?";
static const struct option longOpts[] = {
		{"device",required_argument, NULL,'d'},		
		{"get-sim",required_argument, NULL,'s' },
		{"get-ccid",no_argument, NULL,'c'},
		{"get-pin-status",no_argument, NULL,'p'},
		{"get-rssi",no_argument, NULL,'r'},
		{"get-reg",no_argument, NULL,'R'},		
		{"get-bsid",no_argument, NULL,'B'},
		{"get-band",no_argument, NULL,'b'},
		{"get-net-type",no_argument, NULL,'n'},
		{"get-data-status",no_argument, NULL,'S'},
		{"get-data-type",no_argument, NULL,'D'},
		{"get-imsi",no_argument, NULL,'i'},
		{"get-imei",no_argument, NULL,'I'},
		{"get-modem",no_argument, NULL,'m'},
		{"get-fw",no_argument, NULL,'f'},
		{"all",required_argument, NULL,'a'},
		{"power-off",required_argument, NULL,'x'},
		{"power-up",required_argument, NULL,'o'},
		{"help", no_argument, NULL,'h'},
		{ NULL, no_argument, NULL, 0 }
};

void display_usage(void)
{
	printf("Usage: simman2_cli -d [PORT] [-cprRBbnSDiImf]\n"
			"    or simman2_cli [-asxo[CONFIG]]\n"
			"Options:\n"
			"  --device=PORT, -d PORT:          Set device name to PORT\n"
			"  --get-ccid, -c:                  Get CCID SIM card\n"
			"  --get-pin-status, -p:            Get PIN status\n"
			"  --get-rssi, -r:                  Get RSSI level\n"
			"  --get-reg, -R:                   Get registration status\n"
			"  --get-bsid, -B:                  Get base station id\n"
			"  --get-band, -b:                  Get mobile band\n"
			"  --get-net-type, -n:              Get network type\n"
			"  --get-data-status, -S:           Get data connection status\n"
			"  --get-data-type, -D:             Get data connection type\n"
			"  --get-imsi, -i:                  Get IMSI SIM card\n"
			"  --get-imei, -I:                  Get IMEI modem\n"
			"  --get-modem, -m:                 Get modem type\n"
			"  --get-fw, -f:                    Get firmware version\n"
			"       OR\n"
			"  --all=CONFIG, -a CONFIG:         Get all information about the requested CONFIG\n"
			"  --get-sim=CONFIG, -s CONFIG:     Get SIM information\n"
			"  --power-off=CONFIG, -x CONFIG:   Modem shutdown\n"
			"  --power-up=CONFIG, -o CONFIG:    Modem power up\n"
			);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int ch,cmd;
	struct modems_ops *modem = NULL;

	while((ch = getopt_long(argc, argv, optString, longOpts, NULL)) != -1) {
		switch(ch){
			case 'd':
				device=optarg;
				break;
			case 's':
				cmd=INFO_SIM;
				config=optarg;
				break;
			case 'c':
				cmd=INFO_CCID;
				break;
			case 'p':
				cmd=INFO_PINSTAT;
				break;
			case 'r':
				cmd=INFO_SIGLEV;
				break;
			case 'R':
				cmd=INFO_REGSTAT;
				break;
			case 'B':
				cmd=INFO_BASESTID;
				break;
			case 'b':
				cmd=INFO_BASESTBW;
				break;
			case 'n':
				cmd=INFO_NETTYPE;
				break;
			case 'S':
				cmd=INFO_GPRSSTAT;
				break;
			case 'D':
				cmd=INFO_PACKTYPE;
				break;
			case 'i':
				cmd=INFO_IMSI;
				break;
			case 'I':
				cmd=INFO_IMEI;
				break;
			case 'm':
				cmd=INFO_MODEM;
				break;
			case 'f':
				cmd=INFO_FW;
				break;
			case 'a':
				cmd=INFO_ALL;
				config=optarg;
				break;
			case 'x':
				cmd=POWER_OFF;
				config=optarg;
				break;
			case 'o':
				cmd=POWER_UP;
				config=optarg;
				break;
			case 'h':
			case '?':
			default:
				display_usage();
				break;
		}
	}
	if ((!device) && (!config)) {
		LOG("No device or config given\n");
		display_usage();
	}
	if (!config) {
		if ( ModemStarted(device) < 0 )
		{
			LOG("Modem not found\n");
			return 1;
		}

		modem = modems_backend(device);
		if(modem!=NULL && cmd!=NULL){
			fprintf(stdout, "%s\n", modem_summary(modem,cmd,device));
		} else
		{
			LOG("modem does not respond to AT-commands or unknown modem\n");
		}
	} else {
		struct settings_entry settings;
		if (uci_read_configuration(&settings,config)==0)
		{
			if(cmd==INFO_SIM){
				fprintf(stdout, "%s\n", GetSimInfo(&settings));
				return 0;
			}

			if(cmd==INFO_ALL) {
				if(ModemStarted(settings.atdevice) < 0 ) {
					LOG("modem not found\n");
					return 1;
				}
				if(GetAllInfo(&settings)) {
					LOG("Error while reading SIM info\n");
					return 1;
				}
				return 0;
			}

			if(ModemStarted(settings.atdevice) < 0) {
				if(cmd==POWER_UP) {
					LOG("modem power up\n");
					modem_common_power_reset(&settings,NULL);
					return 0;
				}
				if(cmd==POWER_OFF) {
					LOG("modem powered off\n");
					return 0;
				}
			} else {
				modem = modems_backend(settings.atdevice);
				if(modem!=NULL) {
					if(cmd==POWER_UP) {
						LOG("modem powered on\n");
						return 0;
					}
					if(cmd==POWER_OFF) {
						LOG("modem power off\n");
						modem->power_down(&settings);
						return 0;
					}
				} else {
					LOG("modem does not respond to AT-commands or unknown modem\n");
					return 1;
				}
			}
		}
	}
	return 0;
}
