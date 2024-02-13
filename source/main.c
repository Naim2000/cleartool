#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>

#include "video.h"
#include "pad.h"
#include "libpatcher/libpatcher.h"

struct option {
	const char* name;
	bool selected;
};

static bool SelectOptionsMenu(struct option options[]) {
	int cnt = 0, index = 0, curX = 0, curY = 0;
	while (options[++cnt].name)
		;

	CON_GetPosition(&curX, &curY);

	for (;;) {
		struct option* opt = options + index;

		printf("\x1b[%i;%iH", curY, curX);
		for (int i = 0; i < cnt; i++)
			printf("%s%s	%s\x1b[40m\x1b[39m\n", i == index? ">>" : "  ",
												   options[i].selected? "\x1b[47;1m\x1b[30m" : "", options[i].name);

		for (;;) {
			scanpads();
			uint32_t buttons = buttons_down(0);

			if (buttons & WPAD_BUTTON_DOWN) {
				if (++index == cnt) index = 0;
				break;
			}
			else if (buttons & WPAD_BUTTON_UP) {
				if (--index < 0) index = cnt - 1;
				break;
			}

			else if (buttons & WPAD_BUTTON_A) { opt->selected ^= true; break; }
			else if (buttons & WPAD_BUTTON_PLUS) return true;
			else if (buttons & (WPAD_BUTTON_B | WPAD_BUTTON_HOME)) return false;
		}
	}
}

static void DeleteFile(const char* filepath) {
	printf("	>> %s... ", filepath);
	int ret = ISFS_Delete(filepath);

	if (ret < 0) printf("Failed! (%i)\n", ret);
	else puts("OK!");
}

static void DeleteWC24ChannelDownload(uint64_t titleID) {
	static char filepath[ISFS_MAXPATH];

	sprintf(filepath, "/title/%08x/%08x/data/wc24dl.vff", (uint32_t)(titleID >> 32), (uint32_t)(titleID & 0xFFFFFFFF));
	DeleteFile(filepath);
}

/* Why is this guy making 2 morbillion subroutines? */
static void DeleteWC24ChannelDownloadA(uint64_t titleID) {
	titleID &= ~0xFF;

	DeleteWC24ChannelDownload(titleID | 'E');
	DeleteWC24ChannelDownload(titleID | 'P');
	DeleteWC24ChannelDownload(titleID | 'J');
}

int main(void) {
	puts(
		"WiiConnect24 cleartool v1.0\n" );

	puts("Reloading & patching IOS, please wait...\n");
	bool patch_ok = patch_ahbprot_reset() && patch_isfs_permissions();

	// Still want the user's Wii remote to connect
	initpads();
	if (!patch_ok) // libpatcher kindly prints messages on screen
		goto exit;

	ISFS_Initialize();

	static struct option ClearToolOptions[5] = {
		{ "Forecast Channel data" },
		{ "News Channel data" },
		{ "WiiConnect24 mailboxes" },
		{ "WiiConnect24 Message server config" }
	};

	puts(
		"Select what you would like to delete.\n"
		"Press A to select an item.\n"
		"Press + to begin deleting.\n" );

	if (!SelectOptionsMenu(ClearToolOptions)) goto exit;

	if (ClearToolOptions[0].selected) {
		puts("Deleting Forecast Channel data...");
		DeleteWC24ChannelDownloadA(0x0001000248414600LL); // I forgot LL here and it got past somehow. gcc is wonderful
	}

	if (ClearToolOptions[1].selected) {
		puts("Deleting News Channel data...");
		DeleteWC24ChannelDownloadA(0x0001000248414700LL);
	}

	if (ClearToolOptions[2].selected) {
		puts("Deleting WiiConnect24 mailboxes...");
		DeleteFile("/shared2/wc24/mbox/wc24recv.ctl");
		DeleteFile("/shared2/wc24/mbox/wc24recv.mbx");
		DeleteFile("/shared2/wc24/mbox/wc24send.ctl");
		DeleteFile("/shared2/wc24/mbox/wc24send.mbx");
	}

	if (ClearToolOptions[3].selected) {
		puts("Deleting WiiConnect24 Message server config...");
		DeleteFile("/shared2/wc24/nwc24msg.cbk");
		DeleteFile("/shared2/wc24/nwc24msg.cfg");
	}

exit:
	puts("Press HOME to exit.");
	wait_button(WPAD_BUTTON_HOME);

	printf("Failed to launch the Wii menu..!? (%i)\n", WII_LaunchTitle(0x100000002LL));
	sleep(3);
	return 0;
}
