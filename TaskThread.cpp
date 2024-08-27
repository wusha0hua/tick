#include "TaskThread.h"
#include "DataStruct.h"
#include <chrono>
#include <iostream>
#include <mysql/mysql.h>
#include <string>
#include <thread>
#include <vector>

TaskThread::TaskThread() {

}

void TaskThread::start() {
	threads.emplace_back(std::thread(&TaskThread::create_thread, this));
	print_message("create task thread");
}

void TaskThread::print_message(std::string message) {
	std::cout << message << std::endl;
}

void TaskThread::create_thread() {
	interval = 5;
	while(true) {
		std::this_thread::sleep_for(std::chrono::seconds(interval));
		if(connect_mysql()) {
			save_tick();
			disconnect_mysql();
			print_message("save tick");
		}
	}
}

bool TaskThread::connect_mysql() {
	if(mysql->handle != nullptr) return true;
	mysql->handle = mysql_init(nullptr);
	if(mysql_real_connect(mysql->handle, mysql->hostname.c_str(), mysql->username.c_str(), mysql->password.c_str(), mysql->database.c_str(), mysql->port, nullptr, 0 ) != nullptr) {
		mysql_autocommit(mysql->handle, 0);
		mysql_query(mysql->handle, "SET NAMES UTF8");
		mysql_query(mysql->handle, "SET SQL_SAFE_UPDATES = 0");
		mysql_query(mysql->handle, "SET INTERACTIVE_TIMEOUT = 28800");
		mysql_query(mysql->handle, "SET GLOBAL MAX_ALLOWED_PACKET = 1024");
		print_message("mysql conncet sucess");
		return true;
	}
	print_message("mysql conncet fail");
	print_message(mysql_error(mysql->handle));
	mysql_close(mysql->handle);
	mysql->handle = nullptr;
	return false;
}

void TaskThread::disconnect_mysql() {
	mysql_close(mysql->handle);
	mysql->handle = nullptr;
}

void TaskThread::save_tick() {
	mysql_query(mysql->handle, "start transaction");
	tickvec->mutex.lock();
	std::vector<Tick> ticks = std::move(tickvec->ticks);	
	//tickvec->ticks.clear();
	tickvec->mutex.unlock();
	for(Tick tick: ticks) {
		std::string sql;
		sql = "insert into " + mysql->tick_table + " values(" + 
			"'" + tick.instrument_id + "'" + "," +
			tick.update_date + "," + 
			"'" + tick.update_time + "." + tick.update_millisecond + "'" + "," +
			std::to_string(tick.bid_price1) + "," +
			std::to_string(tick.ask_price1) + "," +
			std::to_string(tick.last_price) + "," +
			std::to_string(tick.bid_volume1) + "," + 
			std::to_string(tick.ask_volume1) + "," + 
			std::to_string(tick.last_traded_volume) + "," +
			std::to_string(tick.open_interest) + 
			+ ")";
		if(mysql_query(mysql->handle, sql.c_str())) {
			print_message(mysql_error(mysql->handle));
			continue;
		}
	}
	mysql_commit(mysql->handle);
}

