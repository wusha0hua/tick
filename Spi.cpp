#include "Spi.h"
#include "DataStruct.h"
#include "ctp_api_linux/ThostFtdcUserApiDataType.h"
#include "ctp_api_linux/ThostFtdcUserApiStruct.h"
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <unordered_map>

bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo) {
	if(pRspInfo == nullptr) return false;
	if(pRspInfo->ErrorID != 0) {
		std::cout << __FUNCTION__ << std::endl;
		std::cout << pRspInfo->ErrorID << std::endl;
		std::cout << pRspInfo->ErrorMsg << std::endl;
		return true;
	} else return false;
}

std::string get_current_time() {
	std::time_t time_ = time(&time_);
	char buff[64];
	memset(buff, 0, sizeof(buff));
	strftime(buff, sizeof(buff), "%H:%M:%S", localtime(&time_));
	return std::string(buff);
}

std::string get_current_date() {
	std::time_t time_ = time(&time_);
	char buff[64];
	memset(buff, 0, sizeof(buff));
	strftime(buff, sizeof(buff), "%Y%m%d", localtime(&time_));
	return std::string(buff);

}

std::string get_current_datetime() {
	std::time_t time_ = time(&time_);
	char buff[64];
	memset(buff, 0, sizeof(buff));
	strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", localtime(&time_));
	return std::string(buff);
}

void MDSpi::OnFrontConnected() {
	id = std::string(account->id);
		ReqUserLogin();	
}

void MDSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if(pRspInfo && pRspInfo->ErrorID != 0) {
		std::cout << "User Login Fail" << std::endl;
		std::cout << pRspInfo->ErrorMsg << std::endl;
		std::cout << pRspInfo->ErrorID << std::endl;
	} else {
		std::cout << "User Login Sucess" << std::endl;
		SubscribeInstrument();
	}
}

void MDSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	std::string instrument_id = (std::string)pSpecificInstrument->InstrumentID;
	if(pRspInfo && pRspInfo->ErrorID != 0) {
		std::cout << "subscribe instrument fail" << std::endl;
		std::cout << pRspInfo->ErrorMsg << std::endl;
		std::cout << pRspInfo->ErrorID << std::endl;
	} else {
		//std::cout << instrument_id << " subscribe sucess" << std::endl;
	}
}

void MDSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData) {
	Tick tick;
	tick.instrument_id = pDepthMarketData->InstrumentID;
	tick.update_date = get_current_date();
	tick.update_time = get_current_time();
	tick.update_millisecond = std::to_string(pDepthMarketData->UpdateMillisec);
	tick.bid_price1 = pDepthMarketData->BidPrice1;
	tick.ask_price1 = pDepthMarketData->AskPrice1;
	tick.last_price = pDepthMarketData->LastPrice;
	tick.bid_volume1 = pDepthMarketData->BidVolume1;
	tick.ask_volume1 = pDepthMarketData->AskVolume1;
	tick.last_traded_volume = pDepthMarketData->Volume;
	tick.open_interest = pDepthMarketData->OpenInterest;

	tickvec->mutex.lock();
	tickvec->ticks.emplace_back(std::move(tick));
	tickvec->mutex.unlock();

	/*
	std::cout << "instrument id: " << pDepthMarketData->InstrumentID << " last price: " << pDepthMarketData->LastPrice << std::endl;
	std::unordered_map<std::string, double> m;
	m[pDepthMarketData->InstrumentID] = pDepthMarketData->LastPrice;
	*/
	//market->last_price[pDepthMarketData->InstrumentID] = (double)pDepthMarketData->LastPrice;
}
	
void MDSpi::ReqUserLogin() {
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strncpy(req.BrokerID, account->broker_id, sizeof(account->broker_id));
	strncpy(req.UserID, account->id, sizeof(account->id));
	strncpy(req.Password, account->password, sizeof(account->password));

	int err = account->md_api->ReqUserLogin(&req, 0);
	if(err != 0) {
		std::cout << "User Login Request Error" << std::endl;
	}
}
	
void MDSpi::SubscribeInstrument() {
	for(auto it = account->instruments.begin(); it != account->instruments.end(); it++) {
		char *instrument_id = (char*)(it->first).data();
		account->md_api->SubscribeMarketData(&instrument_id, 1);
	}
	account->is_md_api_connected.store(true);
}

void TradeSpi::OnFrontConnected() {
	id = std::string(account->id);
	OnReqAuthenticate();	
}

void TradeSpi::OnReqAuthenticate() {
	CThostFtdcReqAuthenticateField req;
	memset(&req, 0, sizeof(req));
	strncpy(req.BrokerID, account->broker_id, sizeof(account->broker_id));
	strncpy(req.UserID, account->id, sizeof(account->id));
	strncpy(req.AppID, account->app_id, sizeof(account->app_id));
	strncpy(req.AuthCode, account->auth_code, sizeof(account->auth_code));
	account->trade_api->ReqAuthenticate(&req, 0);
}

void TradeSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if(pRspInfo && pRspInfo->ErrorID != 0) {
		std::cout << "Authenticate fail" << std::endl;
		std::cout << pRspInfo->ErrorMsg << std::endl;
		std::cout << pRspInfo->ErrorID << std::endl;
	} else {
		std::cout << "Authenticate sucess" << std::endl;
		ReqUserLogin();
	}
}

void TradeSpi::ReqUserLogin() {
	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));
	strncpy(req.BrokerID, account->broker_id, sizeof(account->broker_id));
	strncpy(req.UserID, account->id, sizeof(account->id));
	strncpy(req.Password, account->password, sizeof(account->password));
	account->trade_api->ReqUserLogin(&req, 0);
}


void TradeSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if(pRspInfo && pRspInfo->ErrorID != 0) {
		std::cout << "trade login fail" << std::endl;
		std::cout << pRspInfo->ErrorMsg << std::endl;
		std::cout << pRspInfo->ErrorID << std::endl;
	} else {
		std::cout << "trade login sucess" << std::endl;
		/*
		account->front_id = pRspUserLogin->FrontID;
		account->session_id = pRspUserLogin->SessionID;
		account->order_ref = atoi(pRspUserLogin->MaxOrderRef) + 1;
		*/
		ReqSettlementInfoConfirm();	
	}
}
	
void TradeSpi::ReqSettlementInfoConfirm() {
	CThostFtdcSettlementInfoConfirmField req;
	memset(&req, 0, sizeof(req));
	strncpy(req.BrokerID, account->broker_id, sizeof(account->broker_id));
	strncpy(req.InvestorID, account->id, sizeof(account->id));
	account->trade_api->ReqSettlementInfoConfirm(&req, 0);
}

void TradeSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if(bIsLast && !IsErrorRspInfo(pRspInfo)) {
		std::cout << "settlement confirm sucess" << std::endl;
		ReqQryInstrument();
	} else {
		std::cout << "settlement confirm fail" << std::endl;
		std::cout << "is last: " << bIsLast << std::endl;
		std::cout << pRspInfo->ErrorID << std::endl;
		std::cout << std::string(pRspInfo->ErrorMsg) << std::endl;
	}
}

void TradeSpi::ReqQryInstrument() {
	CThostFtdcQryInstrumentField req;
	memset(&req, 0, sizeof(req));
	strcpy(req.InstrumentID, "");
	account->trade_api->ReqQryInstrument(&req, 0);
}


void TradeSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
	if(!IsErrorRspInfo(pRspInfo) && pInstrument != nullptr) {
		std::string instrument_id = pInstrument->InstrumentID;
		if(pInstrument->ProductClass == THOST_FTDC_PC_Futures && pInstrument->IsTrading) {
			Instrument *instrument = new Instrument;
			instrument->id = pInstrument->InstrumentID;
			instrument->exchange_id = pInstrument->ExchangeID;
			instrument->volume_multiplier = pInstrument->VolumeMultiple;
			instrument->long_margin_ratio = pInstrument->LongMarginRatio;
			instrument->short_margin_ratio = pInstrument->ShortMarginRatio;
			instrument->price_tick = pInstrument->PriceTick;
			account->instruments[instrument_id] = instrument;
		}
	}
	///////////////////////////////////////////////////////////////
	account->is_trade_api_connected.store(true);

	if(bIsLast) {
		account->is_trade_api_connected.store(true);
	}
}

void TradeSpi::OnRtnTrade(CThostFtdcTradeField *pTrade) {
	std::cout << "trade: " << pTrade->InstrumentID << " price: " << pTrade->Price << " volume: " << pTrade->Volume << std::endl;
}

