#include "sys.h"
#include "Host.h"

#include <stdlib.h>
#ifdef WIN32
#include <crtdbg.h>
#endif
#include "Sound.h"
#include "keyboard.h"

SYS sys;
Host host;
memPool pool(16 * 1024 * 1024);
memPool linear;
void new_game() {
	host.execute("map start");
}

void spectre_main() {
	int frame = 0;
	double diff, new_time;
	double old_time = sys.seconds();

	sys.init();
	host.init();
	host.execute("music track 3");
	keyboard_bind(K_F2, new_game);

	do {
		new_time = sys.seconds();
		diff = new_time - old_time;
		host.frame(diff);
		old_time = new_time;

	} while (!sys.quiting());

	host.shutdown();
	sys.shutdown();
}

int main(int argc,char **argv) {

#ifdef WIN32
	//prevent safe string functions from filling buffers with garbage
	_CrtSetDebugFillThreshold(0);
#endif

	spectre_main();


	return 0;
}