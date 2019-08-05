#include "sys.h"
#include "Host.h"

#include <stdlib.h>
#include <malloc.h>
#ifdef WIN32
#include <crtdbg.h>
#endif
#include "Sound.h"
#include "keyboard.h"

SYS sys;
Host host;
memPool pool;
memPool linear;
void new_game() {
	host.execute("map start");
}

void spectre_main() {
	double diff, new_time;
	double old_time = sys.seconds();

	int poolSize = (envGetHeapSize()<<3)/10;
	char *poolBase = (char *)memalign(0x10, poolSize);
	pool.initFromBase(poolBase, poolSize, 0x10);

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