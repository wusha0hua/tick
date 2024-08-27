main: *.cpp
	g++ -o main *.cpp -lmysqlclient -L ./ctp_api_linux -lthostmduserapi_se -lthosttraderapi_se -Wl,-rpath,./ctp_api_linux -g
