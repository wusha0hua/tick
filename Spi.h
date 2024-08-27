#pragma once
#include "api.h"
#include "DataStruct.h"
#include "ctp_api_linux/ThostFtdcTraderApi.h"
#include "ctp_api_linux/ThostFtdcUserApiStruct.h"
	
bool IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo);
std::string get_current_time();
std::string get_current_date();
std::string get_current_datetime();

class MDSpi: public CThostFtdcMdSpi {
public:
	Account *account;
	TickVector *tickvec;
	virtual void OnFrontConnected();
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);
private:
	std::string id;
	void ReqUserLogin();
	void SubscribeInstrument();
};

class TradeSpi: public CThostFtdcTraderSpi {
public:
	Account *account;
	virtual void OnFrontConnected();
	virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	//virtual int ReqUserLogin(CThostFtdcReqUserLoginField *pReqUserLoginField, int nRequestID);
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
	virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);
private:
	std::string id;
	void OnReqAuthenticate();
	void ReqUserLogin();
	void ReqSettlementInfoConfirm();
	void ReqQryInstrument();
};
