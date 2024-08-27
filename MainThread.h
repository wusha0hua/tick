#pragma once
#include "DataStruct.h"
#include "TaskThread.h"
#include "Spi.h"

#define DATABUFFSIZE 1024
class MainThread {
public:
	MainThread();
	~MainThread();
	void start();
private:
	Account *account;
	TickVector *tickvec;
	Mysql *mysql;
	TaskThread *task_thread;
	void print_message(std::string);
	void load_setting();
	bool conncet_mysql();
	void disconnect_mysql();
	void create_mysql_table();
	void connect_trade_api();
	void connect_market_data_api();
	void read_instrument();
	void save_instrument();
	void create_task_thread();

	/*
	AccountData *account;
	MarketData *market;

	void init_account();
	void connect_to_market();
	void connect_to_trade();
	*/
};

