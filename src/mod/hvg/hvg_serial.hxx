#ifndef _HVG_SERIAL_H_
#define _HVG_SERIAL_H_
#include <future>
#include <functional>
#include <iostream>
#include "rs232.h"
#include"spdlog/spdlog.h"
namespace mod::hvg::control {
	constexpr size_t BUF_SIZE = 1024 * 4;
	struct hvg_serial_t {
		int port;
		int baud;
		char mode[4];
		int pos;
		int len;
		char buf[BUF_SIZE];
		enum status_t {
			PENDING,
			SENDING,
			SENT,
			RECIEVING,
			RECIEVED,      // not used
			TIMEOUT
		} status;

		using process_data_f = std::function<void(char* p, int n)>;
		process_data_f process_data;
	};
	bool init(hvg_serial_t* p, int port, int baud, const char* mode);
	void drop(hvg_serial_t* p);
	bool open(hvg_serial_t* p);
	bool close(hvg_serial_t* p);
	int send(hvg_serial_t* p, char* buf, int len);
	int recv(hvg_serial_t* p, char* buf, int len);
	int get_line(hvg_serial_t* p, char* buf, int len, double timeout);
	int get_line(hvg_serial_t* p, char* buf, int len);
}
#endif // _HVG_SERIAL_H_
#ifdef HVG_SERIAL_IMPLEMENTATION
#ifndef _HVG_SERIAL_IMPLEMENTED_
#define _HVG_SERIAL_IMPLEMENTED_
namespace mod::hvg::control {
	bool init(hvg_serial_t* p, int port, int baud, const char* mode)
	{
		if (p == nullptr) {
			return false;
		}
		p->len = 0;
		p->port = port;
		p->baud = baud;
		strncpy(p->mode, mode, 4);
		return true;
	}
	void drop(hvg_serial_t* p)
	{

	}
	bool open(hvg_serial_t* p)
	{
		if (p == nullptr) {
			return false;
		}
		if (RS232_OpenComport(p->port, p->baud, p->mode)) {
			spdlog::error("can not open server port:{:d}", p->port);
			return false;
		}
		spdlog::info("open the server comport:{:d}", p->port);
		return true;
	}
	bool close(hvg_serial_t* p)
	{
		if (p == nullptr) {
			return false;
		}
		RS232_CloseComport(p->port);
		spdlog::info("server port status is closed");
		return true;
	}
	int send(hvg_serial_t* p, char* buf, int len)
	{
		spdlog::debug("send->port:{:d}", p->port);
		if (p == nullptr) {
			return -1;
		}
		if (*buf == 0) {
			spdlog::info("No data need send!");
			return 0;
		}
		strcat(buf, "\r\n");
		int ret = RS232_SendBuf(p->port, (unsigned char*)buf, strlen(buf));
		return ret;
	}
	static int feedback(char result)
	{
		unsigned char feedback_res[10] = {};
		feedback_res[0] = result;
		int ret = RS232_SendBuf(2, feedback_res, 10);
		//spdlog::debug("send feedback byte:{:d}", ret);
		return 0;
	}
	static void judge_result(char* out)
	{
		char test1_end_result[] = "vbn123\r\n";
		char test2_end_result[] = ">ase34 \r\n";
		char test3_end_result[] = "fge123\r\n";
		if (strcmp(out, test1_end_result) == 0) {
			feedback('A');
		}
		else if (strcmp(out, test2_end_result) == 0) {
			feedback('B');
		}
		else if (strcmp(out, test3_end_result) == 0) {
			feedback('C');
		}

	}
	static int scan_data(char* start, int len, char* out)
	{
		int i = 0;
		spdlog::debug("length:{:d};available space:{:d}", start, BUF_SIZE - len);
		while (i < len) {
			if (start[i] == '\r' && start[i + 1] == '\n') {
				memcpy(out, start, i + 2);
				//judge_result(out);
				memmove(start, start + i + 2, len - i - 2);
				return i + 2;
			}
			i++;
			spdlog::debug("i:{:d}", i);
		}
		return 0;
	}
	int recv(hvg_serial_t* p, char* buf, int len)
	{
		if (p == nullptr) {
			return -1;
		}
		char* ptr = p->buf;
		int n = RS232_PollComport(p->port, (unsigned char*)ptr + p->len, BUF_SIZE - p->len);
		p->len = p->len + n;
		p->buf[p->len] = '\0';
		//spdlog::debug("receive_buf:{:s}", p->buf);
		n = scan_data(p->buf, p->len, buf);
		//spdlog::debug("p->len:{:d}", p->len);
		if (n > 0) {
			p->len = p->len - n;
			return n;
		}
		return 0;
	}
	//static bool is_stop(char* p, char* stop_chars)
	//{
	//
	//}
	int get_line(hvg_serial_t* p, char* buf, int len)
	{
		if (p == nullptr) {
			return -1;
		}
		char* ptr = p->buf;
		int i = 0;
		while (1) {
			int n = RS232_PollComport(p->port, (unsigned char*)ptr + p->len, BUF_SIZE - p->len);
			p->len = p->len + n;
			p->buf[p->len] = '\0';
			n = scan_data(p->buf, p->len, buf);
			if (n > 0) {
				p->len = p->len - n;
				return n;
			}
		}
		return 0;
	}
	int get_line(hvg_serial_t* p, char* buf, int len, double timeout)
	{
		if (p == nullptr) {
			return -1;
		}
		char* ptr = p->buf;
		int i = 0;
		clock_t start;
		clock_t finish;
		double run_time = 0;
		start = clock();
		while (run_time < timeout) {
			finish = clock();
			run_time = ((double)(finish - start) / CLOCKS_PER_SEC) * 1000.0;
			//spdlog::debug("run_time:{:f}", run_time);
			int n = RS232_PollComport(p->port, (unsigned char*)ptr + p->len, BUF_SIZE - p->len);
			p->len = p->len + n;
			//p->buf[p->len] = '\0';
			//spdlog::debug("receive_buf:{:s}", p->buf);
			n = scan_data(p->buf, p->len, buf);
			//spdlog::debug("p->len:{:d}", p->len);
			if (n > 0) {
				p->len = p->len - n;
				return n;
			}
		}
		return 0;
	}
}
#endif // _HVG_SERIAL_IMPLEMENTED_
#endif // HVG_SERIAL_IMPLEMENTATION
