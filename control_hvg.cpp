#include <iostream>
#include <atomic>
#include <thread>
#define WINDOW_IMPLEMENTATION
#define LOG_IMPLEMENTATION
#define HVG_SERIAL_IMPLEMENTATION
#include "imgui_log.hxx"
#include "hvg_serial.hxx"
#include "window.hxx"
#include "spdlog/spdlog.h"
std::atomic<bool> breakflag(false);
std::atomic<bool> completeflag(false);
bool hvg_serial_init(mod::hvg::control::hvg_serial_t* ptr, const int port, const int bdrate)
{
	char mode[] = { '8','N','2',0 };
	bool ret = mod::hvg::control::init(ptr, port, bdrate, mode);
	printf("portis %d bdrate is %d\n", port, bdrate);
	if (ret == false) {
		spdlog::error("init failed!\n");
		return false;
	}
	spdlog::debug("init successfully!\n");
	return true;
}
void recv_data(mod::hvg::control::hvg_serial_t* hvg_ptr, ui::log::display_log_t* log_ptr)
{
	static double timeout = 1000;
	while (!breakflag.load())
		//while (1)
	{
		char recv_buff[1024] = {};
		//mod::hvg::control::get_line(hvg_ptr, recv_buff, 1024);
		printf("before complete command is %s,and length is %d\n", recv_buff, strlen(recv_buff));
		//int n = mod::hvg::control::get_line(hvg_ptr, recv_buff, 1024);
		int n = mod::hvg::control::get_line(hvg_ptr, recv_buff, 1024, timeout);
		printf("after complete command is %s,and length is %d\n", recv_buff, strlen(recv_buff));
		if (strlen(recv_buff) != 0) {
			add_log(log_ptr, recv_buff, n);
		}
		//	printf("recv_buff length is ->%d\n", strlen(recv_buff));
		/*if (strlen(recv_buff) == 0) {
			break;
		}*/
		spdlog::debug("p->buf is :{:s}", hvg_ptr->buf);
	}
	completeflag.store(true);
	printf("break while\n");
}
bool ERRORHANDLE(mod::hvg::control::hvg_serial_t* hvg_ptr, const int errorflag)
{
	static char handshake_buff[50] = "<IFV\r\n";
	int ret;
	printf("enter into errorhandle function!\n");
	switch (errorflag) {
	case 1:
		ret = RS232_SendBuf(hvg_ptr->port, (unsigned char*)handshake_buff, strlen(handshake_buff));
		printf("send_buff is %s,send_byte is %d\n", handshake_buff, ret);
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	case 5:
		break;
	default:
		printf("unkown error!\n");
		return false;
	}
	return true;
}
bool HANDSHAKE(mod::hvg::control::hvg_serial_t* hvg_ptr)
{
	static char handshake_buff[50] = "<IFV\r\n";
	static char compare_command_version1[50] = ">IFV 1\r\n";
	static char compare_command_version2[50] = ">IFV 2\r\n";
	static char compare_handshake_unkown[50] = ">UNK\r\n";
	int ret = RS232_SendBuf(hvg_ptr->port, (unsigned char*)handshake_buff, strlen(handshake_buff));
	printf("send_buff is %s,send_byte is %d\n", handshake_buff, ret);
	static double timeout = 1000;
	//while (!breakflag.load())
	while (1)
	{
		char recv_buff[1024] = {};
		//mod::hvg::control::get_line(hvg_ptr, recv_buff, 1024);
		printf("before complete command is %s,and length is %d\n", recv_buff, strlen(recv_buff));
		//int n = mod::hvg::control::get_line(hvg_ptr, recv_buff, 1024);
		int n = mod::hvg::control::get_line(hvg_ptr, recv_buff, 1024, timeout);
		if (n > 0) {
			printf("recv_buff is %s,compare_handshake_buff is %s\n", recv_buff, compare_command_version2);
			if (strcmp(recv_buff, compare_command_version1) == 0 || strcmp(recv_buff, compare_command_version2) == 0) {
				printf("true\n");
				return true;
			}
			else if (strcmp(recv_buff, compare_handshake_unkown) == 0) {
				printf("false\n");
				ERRORHANDLE(hvg_ptr, 1);
				return false;
			}
		}
		printf("after complete command is %s,and length is %d\n", recv_buff, strlen(recv_buff));
		/*	if (strlen(recv_buff) != 0) {
				add_log(log_ptr, recv_buff, n);
			}*/
			//	printf("recv_buff length is ->%d\n", strlen(recv_buff));
			/*if (strlen(recv_buff) == 0) {
				break;
			}*/
		spdlog::debug("p->buf is :{:s}", hvg_ptr->buf);
	}

}
bool rs232_imgui_interface(ui::window_t*, mod::hvg::control::hvg_serial_t* hvg_ptr, ui::log::display_log_t* log_ptr)
{
	static int i = 0;
	static char command_history_buff[100][100];
	static char current_command_buff[100];
	static int receive_flag;
	const char* items[] = { "<IFV\r\n", "<S_SDRSSL 60000\r\n", "<ES3 0 100 0.5 10 0 L 6\r\n", "<SXP 1 0 1\r\n", "<SXP 0 0 0\r\n", "<GST\r\n", "<ERQ\r\n" };
	static int item_current = -1;
	/*static char port_buff[20] = "2";
	 static char bdrate_buff[20] = "9600";*/
	 //static char kv_buff[20];
	 //static char mAs_buff[20];
	static int kv;
	static int port = 2;
	static int bdrate = 19200;
	static float mAs;
	ImGui::Begin("RS-232 Communication");
	//ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.5f, 0.7f), "Please input Port");
	ImGui::Text("Please input Port");
	ImGui::InputInt("Port", &port);
	//ImGui::InputText("Port", port_buff, IM_ARRAYSIZE(port_buff));
	ImGui::Text("Please input Bdrate");
	//ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.5f, 0.7f), "Please input Bdrate");
	ImGui::InputInt("Bdrate", &bdrate);
	//ImGui::InputText("Bdrate", bdrate_buff, IM_ARRAYSIZE(bdrate_buff));
	//ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.5f, 0.7f), "Please input KV");
	ImGui::Text("Please input KV");
	ImGui::InputInt("KV", &kv);
	ImGui::Text("Please input mAs");
	//ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.5f, 0.7f), "Please input mAs");
	ImGui::InputFloat("mAs", &mAs);
	if (ImGui::Button("init")) {
		std::thread init(hvg_serial_init, hvg_ptr, port, bdrate);
		init.detach();
	}
	ImGui::SameLine();
	if (ImGui::Button("Open Port")) {
		std::thread open(mod::hvg::control::open, hvg_ptr);
		open.detach();
	}
	ImGui::SameLine();
	if (ImGui::Button("Close Port")) {
		std::thread close(mod::hvg::control::close, hvg_ptr);
		close.detach();
	}
	if (ImGui::Button("Handshake")) {
		std::thread HandShake(HANDSHAKE, hvg_ptr);
		HandShake.detach();
	}
	if (ImGui::TreeNode("Send && Receive"))
	{
		struct Funcs
		{
			static int MyCallback(ImGuiInputTextCallbackData* data)
			{
				static int i = 0;
				static int j = 0;
				if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
				{
					data->InsertChars(data->CursorPos, "..");
				}
				else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
				{
					if (data->EventKey == ImGuiKey_UpArrow)
					{
						data->DeleteChars(0, data->BufTextLen);
						if (i == 0 && strlen(command_history_buff[0]) == 0) {
							data->InsertChars(0, command_history_buff[0]);
						}
						else if (i >= 0 && i < 100 && strlen(command_history_buff[i + 1]) == 0) {
							data->InsertChars(0, command_history_buff[i]);
						}
						else if (i >= 0 && i < 100 && strlen(command_history_buff[i + 1]) != 0) {
							data->InsertChars(0, command_history_buff[++i]);
						}
						else if (i == 99) {
							data->InsertChars(0, command_history_buff[99]);
						}
						data->SelectAll();
					}
					else if (data->EventKey == ImGuiKey_DownArrow)
					{
						data->DeleteChars(0, data->BufTextLen);
						if (i == 0) {
							data->InsertChars(0, command_history_buff[0]);
						}
						else if (i >= 1) {
							data->InsertChars(0, command_history_buff[--i]);
						}
						data->SelectAll();
					}
					/*		else if (data->EventKey == ImGuiKey_LeftArrow) {
								data->DeleteChars(0, data->BufTextLen);
								data->InsertChars(0, command_history_buff[0]);
								data->SelectAll();
							}
							else if (data->EventKey == ImGuiKey_RightArrow) {
								data->DeleteChars(0, data->BufTextLen);
								data->InsertChars(0, command_history_buff[0]);
								data->SelectAll();
							}*/
				}
				else if (data->EventFlag == ImGuiInputTextFlags_CallbackEdit)
				{
					// Toggle casing of first character
					char c = data->Buf[0];
					if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) data->Buf[0] ^= 32;
					data->BufDirty = true;
					// Increment a counter
					int* p_int = (int*)data->UserData;
					*p_int = *p_int + 1;
				}
				return 0;
			}
		};
		//ImGui::Text("Please input command");//default color
		//ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.5f, 0.7f), "Please input command");//custom color
		ImGui::Text("Please input command");
		ImGui::InputText("Command", current_command_buff, IM_ARRAYSIZE(current_command_buff), ImGuiInputTextFlags_CallbackHistory, Funcs::MyCallback);
		ImGui::Combo("current command", &item_current, items, IM_ARRAYSIZE(items));
		//printf("current command is%s\n", items[item_current]);
		if (item_current != -1) {
			strcpy(current_command_buff, items[item_current]);
		}
		if (ImGui::Button("Send Command")) {
			std::thread send(mod::hvg::control::send, hvg_ptr, current_command_buff, strlen(current_command_buff));
			//std::thread send(mod::hvg::control::send, hvg_ptr, current_command_buff, 100);
			send.join();
			strcpy(command_history_buff[i++], current_command_buff);
			memset(current_command_buff, 0x00, 100);
		}
		if (ImGui::Button("Receive Command")) {
			std::thread receive(recv_data, hvg_ptr, log_ptr);
			receive.detach();
			receive_flag = 1;
		}
		ImGui::SameLine();
		if (receive_flag == 1) {
			ImGui::Text("Receive status is Yes");
		}
		else {
			ImGui::Text("Receive status is No");
		}
		ImGui::TreePop();
	}
	Draw(log_ptr, "Receive data log");
	ImGui::End();
	return true;
}
ui::window_t* imgui_init()
{
	ui::window_t* win_ptr = (ui::window_t*)malloc(sizeof(ui::window_t));
	std::array<float, 4> background_color = { 0.45f, 0.55f, 0.60f, 0.95f };
	int x = 0;
	int y = 0;
	int w = 1280;
	int h = 720;
	char title[50] = "Port Communication";
	bool ret = ui::init(win_ptr, x, y, w, h, background_color, title, rs232_imgui_interface);
	if (ret == false) {
		spdlog::error("init failed");
		return nullptr;
	}
	spdlog::debug("init successfully");
	return win_ptr;
}
int main()
{
	//spdlog::set_level(spdlog::level::debug);
	mod::hvg::control::hvg_serial_t* hvg_ptr = (mod::hvg::control::hvg_serial_t*)malloc(sizeof(mod::hvg::control::hvg_serial_t));
	ui::log::display_log_t* log_ptr = new ui::log::display_log_t;
	ui::window_t* win_ptr = imgui_init();
	if (win_ptr == nullptr) {
		spdlog::debug("imgui init failed!");
		return -1;
	}
	while (!is_close(win_ptr)) {
		new_frame(win_ptr);
		draw(win_ptr, hvg_ptr, log_ptr);
		render(win_ptr);
	}
	breakflag.store(true);
	clean_up(win_ptr);
	drop(win_ptr);
	while (!completeflag.load()) {

	}
	free(hvg_ptr);
	delete log_ptr;
	delete win_ptr;
	hvg_ptr = nullptr;
	win_ptr = nullptr;
	log_ptr = nullptr;
	return 0;
}
