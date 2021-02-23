#include <iostream>
#include <thread>
#define WINDOW_IMPLEMENTATION
#define LOG_IMPLEMENTATION
#define HVG_SERIAL_IMPLEMENTATION
#include "imgui_log.hxx"
#include "hvg_serial.hxx"
#include "window.hxx"
#include "spdlog/spdlog.h"
bool hvg_serial_init(mod::hvg::control::hvg_serial_t* ptr, char port_buff[], char bdrate_buff[])
{
	int port = atoi(port_buff);
	int bdrate = atoi(bdrate_buff);
	char mode[] = { '8','N','2',0 };
	bool ret = mod::hvg::control::init(ptr, port, bdrate, mode);
	if (ret == false) {
		spdlog::error("init failed!\n");
		return false;
	}
	spdlog::debug("init successfully!\n");
	return true;
}
void recv_data(mod::hvg::control::hvg_serial_t* hvg_ptr, ui::log::Display_Log* log_ptr)
{
	double timeout = 1000;
	while (1)
	{
		char recv_buff[1024] = {};
		//mod::hvg::control::get_line(hvg_ptr, recv_buff, 1024);
		printf("before complete command is %s\n", recv_buff);
		int n = mod::hvg::control::get_line(hvg_ptr, recv_buff, 1024);
		printf("after complete command is %s\n", recv_buff);
		if (strlen(recv_buff) != 0) {
			add_log(log_ptr, recv_buff, n);
		}
		//	printf("recv_buff length is ->%d\n", strlen(recv_buff));
		/*if (strlen(recv_buff) == 0) {
			break;
		}*/
		spdlog::debug("p->buf is :{:s}", hvg_ptr->buf);
	}
	printf("break while\n");
}
bool rs232_imgui_interface(ui::window_t*, mod::hvg::control::hvg_serial_t* hvg_ptr, ui::log::Display_Log* log_ptr)
{
	static char port_buff[20] = "2";
	static char bdrate_buff[20] = "9600";
	char command_buff[100] = {};
	static int receive_flag;
	ImGui::Begin("RS-232 Communication");
	ImGui::Text("Please input Port");
	ImGui::InputText("Port", port_buff, IM_ARRAYSIZE(port_buff));
	ImGui::Text("Please input Bdrate");
	ImGui::InputText("Bdrate", bdrate_buff, IM_ARRAYSIZE(bdrate_buff));
	if (ImGui::Button("init")) {
		std::thread fifth(hvg_serial_init, hvg_ptr, port_buff, bdrate_buff);
		fifth.detach();
	}
	//hvg_serial_init(hvg_ptr, port_buff, bdrate_buff);
	ImGui::Text("Please input command");
	ImGui::InputText("Command", command_buff, IM_ARRAYSIZE(command_buff));
	if (ImGui::Button("Open Port")) {
		std::thread first(mod::hvg::control::open, hvg_ptr);
		first.detach();
	}
	ImGui::SameLine();
	if (ImGui::Button("Close Port")) {
		std::thread second(mod::hvg::control::close, hvg_ptr);
		second.detach();
	}
	ImGui::SameLine();
	if (ImGui::Button("Send Command")) {
		std::thread third(mod::hvg::control::send, hvg_ptr, command_buff, 100);
		third.join();
		//memset(command_buff, 0x00, 100);
	}
	if (ImGui::Button("Receive Command")) {
		std::thread fourth(recv_data, hvg_ptr, log_ptr);
		fourth.detach();
		receive_flag = 1;
	}
	ImGui::SameLine();
	if (receive_flag == 1) {
		ImGui::Text("Receive status is Yes");
	}
	else {
		ImGui::Text("Receive status is No");
	}
	log_ptr->Draw("Receive data log");
	ImGui::End();
	return true;
}
int imgui_init(ui::window_t* win_ptr)
{
	std::array<float, 4> background_color = { 0.45f, 0.55f, 0.60f, 0.95f };
	int x = 0;
	int y = 0;
	int w = 1280;
	int h = 720;
	char title[50] = "Port Communication";
	bool ret = ui::init(win_ptr, x, y, w, h, background_color, title, rs232_imgui_interface);
	if (ret == false) {
		spdlog::error("init failed");
		return -1;
	}
	spdlog::debug("init successfully");
	return 0;
}
int main()
{
	//spdlog::set_level(spdlog::level::debug);
	mod::hvg::control::hvg_serial_t* hvg_ptr = (mod::hvg::control::hvg_serial_t*)malloc(sizeof(mod::hvg::control::hvg_serial_t));
	ui::window_t* win_ptr = new ui::window_t;
	ui::log::Display_Log* log_ptr = new ui::log::Display_Log;
	imgui_init(win_ptr);
	while (!is_close(win_ptr)) {
		new_frame(win_ptr);
		draw(win_ptr, hvg_ptr, log_ptr);
		render(win_ptr);
	}
	clean_up(win_ptr);
	free(hvg_ptr);
	delete win_ptr;
	delete log_ptr;
	return 0;
}
