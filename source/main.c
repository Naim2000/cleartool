#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gccore.h>

#include "video.h"
#include "pad.h"
#include "libpatcher/libpatcher.h"

typedef const struct option {
	const char* name;
	const char* description;
	uint32_t titleID;
	const char* const fileNames[5];
} Option;

#define NBR_OPTIONS 4
static Option ClearToolOptions[NBR_OPTIONS] =
{
	{
		.name = "Forecast Channel data",
		.description =
		"This will delete the data last downloaded by the Forecast Channel.\n"
		"Generally does not fix FORE000006.\n\n"

		"Deleting this data usually resolves:\n"
		"	* FORE000001\n"
		"	* FORE000003\n"
		"	* FORE000005 (..?)\n"
		"	* FORE000099\n"
		"	* Discontinuation message/Similar custom text messages\n",

		.titleID = 0x48414600
	},
	{
		.name = "News Channel data",
		.description =
		"This will delete the data last downloaded by the News Channel.\n"
		"Generally does not fix NEWS000006.\n\n"

		"Deleting this data usually resolves:\n"
		"	* NEWS000001\n"
		"	* NEWS000003\n"
		"	* NEWS000005 (..?)\n"
		"	* NEWS000099\n"
		"	* Discontinuation message/Similar custom text messages\n",

		.titleID = 0x48414700
	},
	{
		.name = "WiiConnect24 mailboxes",
		.description =
		"This will delete the incoming/outcoming mailboxes for WiiConnect24 Messages.\n"
		"(This does not mean messages on the Wii Message Board itself!)\n\n"

		"Solves 109106, and may solve issues with sending/receiving mail.\n\n"

		"The Wii Menu will automatically recreate these files, then state that\n"
		"the Wii Message Board Data is corrupted. This is perfectly normal.\n",

		.fileNames = {
			"/shared2/wc24/mbox/wc24recv.ctl",
			"/shared2/wc24/mbox/wc24recv.mbx",
			"/shared2/wc24/mbox/wc24send.ctl",
			"/shared2/wc24/mbox/wc24send.mbx",
			NULL
		}
	},
	{
		.name = "WiiConnect24 Message server config",
		.description =
		"This will delete the WiiConnect24 message server config file (nwc24msg.cfg),\n"
		"Used to store the URLs and login credentials for the mail server. (And your Wii number.)\n"

		"The Wii Menu will automatically recreate this file, then state that\n"
		"the Wii Message Board Data is corrupted. This is perfectly normal.\n\n"

		"Please note that you will have to run the Mail patcher again.\n",

		.fileNames = {
			"/shared2/wc24/nwc24msg.cbk",
			"/shared2/wc24/nwc24msg.cfg",
			NULL
		}
	}
};

static void DeleteFile(const char* filepath) {
	printf("	>> %s... ", filepath);
	int ret = ISFS_Delete(filepath);

	if (ret == -106) puts("Not found.");
	else if (ret < 0) printf("Failed! (%i)\n", ret);
	else puts("OK!");
}

static void DeleteWC24ChannelDownload(uint32_t titleID) {
	char filepath[ISFS_MAXPATH];

	sprintf(filepath, "/title/00010002/%08x/data/wc24dl.vff", titleID);
	DeleteFile(filepath);
}

/* Why is this guy making 2 morbillion subroutines? */
static void DeleteWC24ChannelDownloadA(uint32_t titleID) {
	titleID &= ~0xFF;

	DeleteWC24ChannelDownload(titleID | 'J');
	DeleteWC24ChannelDownload(titleID | 'E');
	DeleteWC24ChannelDownload(titleID | 'P');
}

static void DeleteWC24Data(Option* opt) {
	if (opt->titleID) DeleteWC24ChannelDownloadA(opt->titleID);
	else
		for (const char* const* file = opt->fileNames; *file; file++)
			DeleteFile(*file);

}

static void DeleteWC24DataMenu() {
	int index = 0, curX = 0, curY = 0;
	bool ready = false;
	Option* selected = NULL;

	CON_GetPosition(&curX, &curY);

	for (int i = 0; i < NBR_OPTIONS; i++)
		printf("  	%s\n", ClearToolOptions[i].name);


	while (!ready) {
		printf("\x1b[%i;0H", curY);
		for (int i = 0; i < NBR_OPTIONS; i++)
			printf("%s	%s\n", (index == i) ? ">>" : "  ", ClearToolOptions[i].name);

		uint32_t button = wait_button(WPAD_BUTTON_A | WPAD_BUTTON_B | WPAD_BUTTON_DOWN | WPAD_BUTTON_UP | WPAD_BUTTON_HOME);
		switch (button) {
			case WPAD_BUTTON_DOWN:
				if (++index == NBR_OPTIONS) index = 0;
				break;

			case WPAD_BUTTON_UP:
				if (--index < 0) index = NBR_OPTIONS - 1;
				break;

			case WPAD_BUTTON_A:
				selected = ClearToolOptions + index;

			case WPAD_BUTTON_B:
			case WPAD_BUTTON_HOME:
				ready = true;
				break;

		}
	}

	if (selected) {
		putchar('\n');
		puts(selected->description);
		usleep(5000000);
		puts("Are you sure you want to delete this?\n\n"

			 "Press + to confirm.\n"
			 "Press any other button to cancel.");

		if (wait_button(0) & WPAD_BUTTON_PLUS) DeleteWC24Data(selected);
	}
}

int main(void) {
	puts("WiiConnect24 cleartool v1.0");

	puts("Reloading & patching IOS, please wait...\n");
	bool patch_ok = (patch_ahbprot_reset() && patch_isfs_permissions());

	// Still want the user's Wii remote to connect
	initpads();
	if (!patch_ok) // libpatcher kindly prints messages on screen
		goto exit;

	ISFS_Initialize();


	puts("Select what you would like to delete.\n");

	DeleteWC24DataMenu(ClearToolOptions);

exit:
	puts("\nPress HOME to exit.");
	wait_button(WPAD_BUTTON_HOME);

//	printf("Failed to launch the Wii menu..!? (%i)\n", WII_LaunchTitle(0x100000002LL));
//	usleep(3000000);
	return 0;
}
