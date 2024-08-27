#pragma once
#include "api.h"
#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <mysql/mysql.h>

struct Instrument {
	std::string id;
	std::string exchange_id;
	double volume_multiplier;
	double long_margin_ratio;
	double short_margin_ratio;
	double price_tick;
};

struct Account {
	TThostFtdcInvestorIDType id;
	TThostFtdcPasswordType password;		
	TThostFtdcBrokerIDType broker_id;
	TThostFtdcAppIDType app_id;
	TThostFtdcAuthCodeType auth_code;

	std::string md_front_addr;
	std::string trade_front_addr;

	CThostFtdcTraderApi *trade_api;
	CThostFtdcMdApi *md_api;

	std::atomic<bool> is_trade_api_connected;
	std::atomic<bool> is_md_api_connected;

	std::map<std::string, Instrument*> instruments;
	//TThostFtdcFrontIDType front_id;
	//TThostFtdcSessionIDType session_id;

};

struct Mysql {
	MYSQL *handle;
	std::string username;
	std::string password;
	std::string hostname;
	std::string database;
	unsigned int port;
	std::string instrument_table;
	std::string tick_table;
};

struct Tick {
	std::string instrument_id;
	std::string update_date;
	std::string update_time;
	std::string update_millisecond;
	double bid_price1;
	double ask_price1;
	double last_price;
	int bid_volume1;
	int ask_volume1;
	int last_traded_volume;
	double open_interest;
};

struct TickVector {
	std::mutex mutex;
	std::vector<Tick> ticks;
};
