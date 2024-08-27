#include "MainThread.h"
#include "DataStruct.h"
#include "Spi.h"
#include "ctp_api_linux/ThostFtdcMdApi.h"
#include "ctp_api_linux/ThostFtdcTraderApi.h"
#include "ctp_api_linux/ThostFtdcUserApiDataType.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mysql/mysql.h>
#include <string>

MainThread::MainThread(): account(new Account), tickvec(new TickVector), mysql(new Mysql) {
	account->is_md_api_connected = false;
	account->is_trade_api_connected = false;

	mysql->username = "localuser";
	mysql->password = "localuser";
	mysql->database = "test";
	mysql->hostname = "localhost";
	mysql->port = 0;
	mysql->tick_table = "tick";
	mysql->instrument_table = "instrument";
	mysql->handle = nullptr;
}

MainThread::~MainThread() {
	if(mysql->handle) {
		disconnect_mysql();
	}
}

void MainThread::start() {
	load_setting();
	create_mysql_table();
	connect_trade_api();
	while(!account->is_trade_api_connected.load());	
	print_message("connect to trade");
	connect_market_data_api();
	while(!account->is_md_api_connected.load());
	print_message("conncet to market");
	conncet_mysql();
	create_mysql_table();
	read_instrument();
	save_instrument();
	create_task_thread();
}

void MainThread::print_message(std::string s) {
	std::cout << s << std::endl;
}

void MainThread::load_setting() {
	std::ifstream ifs;
	ifs.open("account", std::ios::in);
	if(!ifs.is_open()) {
		print_message("load account fail");
		return;
	}
	ifs >> account->id;
	ifs >> account->password;
	ifs >> account->broker_id;
	ifs >> account->md_front_addr;
	ifs >> account->trade_front_addr;
	ifs >> account->app_id;
	ifs >> account->auth_code;

}

bool MainThread::conncet_mysql() {
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

void MainThread::disconnect_mysql() {
	mysql_close(mysql->handle);
	mysql->handle = nullptr;
}

void MainThread::create_mysql_table() {
	mysql->instrument_table.clear();
	mysql->tick_table.clear();
	mysql->instrument_table = "instrument" + get_current_date();
	mysql->tick_table = "tick" + get_current_date();
	std::string sql;
	if(mysql->handle == nullptr) conncet_mysql();
	sql = "select * from " + mysql->instrument_table;
	if(mysql_query(mysql->handle, sql.c_str())) {
		sql.clear();
		sql = "create table " + mysql->instrument_table + " (instrument_id varchar(30), exchange_id varchar(12), volume_multiplier double, long_margin_ratio double, short_margin_ratio double, price_tick double)";
		if(!mysql_query(mysql->handle, sql.c_str())) {
			print_message(mysql->instrument_table + " create sucess");
		} else {
			print_message(mysql->instrument_table + " create fail");
			print_message(mysql_error(mysql->handle));
		}
	} else {
		MYSQL_RES *result = mysql_store_result(mysql->handle);
		mysql_free_result(result);
	}
	sql.clear();
	sql = "select * from " + mysql->tick_table;
	if(mysql_query(mysql->handle, sql.c_str())) {
		sql.clear();
		sql = "create table " + mysql->tick_table + " (instrument_id varchar(30), update_date date, update_time varchar(12), bid_price1 double, ask_price1 double, last_price double, bid_volume1 int, ask_volume1 int, last_traded_volume int, open_interest double)";
		if(!mysql_query(mysql->handle, sql.c_str())) {
			print_message(mysql->tick_table + " create sucess");
		} else {
			print_message(mysql->tick_table + " create fail");
			print_message(mysql_error(mysql->handle));
		}

	} else {
		MYSQL_RES *result = mysql_store_result(mysql->handle);
		mysql_free_result(result);
	}
	mysql_commit(mysql->handle);
	disconnect_mysql();
}

void MainThread::connect_trade_api() {
	TradeSpi *trade_spi = new TradeSpi;
	account->trade_api = CThostFtdcTraderApi::CreateFtdcTraderApi();	
	trade_spi->account = account;
	account->trade_api->RegisterSpi(trade_spi);
	account->trade_api->RegisterFront(account->trade_front_addr.data());
	account->trade_api->SubscribePublicTopic(THOST_TERT_QUICK);
	account->trade_api->SubscribePrivateTopic(THOST_TERT_QUICK);
	account->trade_api->Init();
}

void MainThread::connect_market_data_api() {
	MDSpi *md_spi = new MDSpi;		
	account->md_api = CThostFtdcMdApi::CreateFtdcMdApi("", true);
	md_spi->account = account;
	md_spi->tickvec = tickvec;
	account->md_api->RegisterSpi(md_spi);
	account->md_api->RegisterFront(account->md_front_addr.data());
	account->md_api->Init();	
}
	
void MainThread::read_instrument() {

}

void MainThread::save_instrument() {
	if(conncet_mysql()) {
		std::string sql;
		mysql_query(mysql->handle, "START TRANSCTION");	
		for(auto it = account->instruments.begin(); it != account->instruments.end(); it++) {
			sql.clear();
			sql = "insert into " + mysql->instrument_table + " values('" + it->second->id + "','" + it->second->exchange_id + "'," + std::to_string(it->second->volume_multiplier) + "," + std::to_string(it->second->long_margin_ratio) + "," + std::to_string(it->second->short_margin_ratio) + "," + std::to_string(it->second->price_tick) + ");"	;
			if(mysql_query(mysql->handle, sql.c_str())) {
				print_message(mysql_error(mysql->handle));
				continue;
			}
		}
		mysql_commit(mysql->handle);
		print_message("instrument save sucess");
		disconnect_mysql();
	}
}

void MainThread::create_task_thread() {
	task_thread = new TaskThread;		
	task_thread->mysql = mysql;
	task_thread->tickvec = tickvec;
	task_thread->start();
}

/*
MainThread::MainThread(): account(new AccountData), market(new MarketData) {
	memset(account, 0, sizeof(AccountData));
	//memset(market, 0, sizeof(MarketData));
}

void MainThread::start() {
	init_account();
	connect_to_market();
	connect_to_trade();
}
	
void MainThread::init_account() {
	std::ifstream ifs;
	ifs.open("account", std::ios::in);
	if(!ifs.is_open()) {
		std::cout << "open account file error" << std::endl;
		exit(-1);
	}
	std::string id, passward;
	ifs >> id;
	ifs >> passward;
	ifs.close();	
	std::string default_broker_id = "9999";
	std::string md_front_addr = "tcp://180.168.146.187:10212";
	std::string trade_front_addr = "tcp://180.168.146.187:10202";
	std::string default_app_id = "simnow_client_test";
	std::string default_auth_code = "0000000000000000";
	strncpy(account->id, id.c_str(), id.size());
	strncpy(account->password, passward.c_str(), passward.size());
	strncpy(account->broker_id, default_broker_id.c_str(), default_broker_id.size());
	account->md_front_addr = md_front_addr;
	account->trade_front_addr = trade_front_addr;
	strncpy(account->app_id, default_app_id.c_str(), default_app_id.size());
	strncpy(account->auth_code, default_auth_code.c_str(), default_auth_code.size());
}
	
void MainThread::connect_to_market() {
	MDSpi *md_spi = new MDSpi;		
	account->md_api = CThostFtdcMdApi::CreateFtdcMdApi("", true);
	md_spi->account = account;
	md_spi->market = market;
	account->md_api->RegisterSpi(md_spi);
	account->md_api->RegisterFront(account->md_front_addr.data());
	account->md_api->Init();	
}


void MainThread::connect_to_trade() {
	TradeSpi *trade_spi = new TradeSpi;
	account->trade_api = CThostFtdcTraderApi::CreateFtdcTraderApi();	
	trade_spi->account = account;
	account->trade_api->RegisterSpi(trade_spi);
	account->trade_api->RegisterFront(account->trade_front_addr.data());
	account->trade_api->SubscribePublicTopic(THOST_TERT_QUICK);
	account->trade_api->SubscribePrivateTopic(THOST_TERT_QUICK);
	account->trade_api->Init();
}
*/
