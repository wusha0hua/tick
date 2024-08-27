#include "MainThread.h"
#include <unistd.h>

using namespace std;

int main() {
	MainThread main_thread;
	main_thread.start();
	pause();
}
