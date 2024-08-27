#include "DataStruct.h"
#include <string>
#include <mutex>
#include <thread>
#include <vector>

class TaskThread {
public:
	TaskThread();
	Mysql *mysql;
	TickVector *tickvec;
	void start();
private:
	int interval;
	std::vector<std::thread> threads;
	void print_message(std::string message);	
	void create_thread();
	bool connect_mysql();
	void disconnect_mysql();
	void save_tick();
};
