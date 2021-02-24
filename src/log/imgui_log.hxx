#ifndef _IMGUI_LOG_H_
#define _IMGUI_LOG_H_
#include <sys/timeb.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
namespace ui::log {
	struct Display_Log {
		ImGuiTextBuffer     Buf;
		ImGuiTextFilter     Filter;
		ImVector<int>       LineOffsets; // Index to lines offset. We maintain this with AddLog() calls.
		bool                AutoScroll;  // Keep scrolling if already at the bottom.
		Display_Log()
		{
			AutoScroll = true;
			Clear();
		}
		void    Clear() {
			Buf.clear();
			LineOffsets.clear();
			LineOffsets.push_back(0);
		}
		void    AddLog(const char* fmt, ...) IM_FMTARGS(2) {
			int old_size = Buf.size();
			va_list args;
			va_start(args, fmt);
			Buf.appendfv(fmt, args);
			va_end(args);
			for (int new_size = Buf.size(); old_size < new_size; old_size++)
				if (Buf[old_size] == '\n')
					LineOffsets.push_back(old_size + 1);
		}
		void    Draw(const char* title, bool* p_open = NULL) {
			if (!ImGui::Begin(title, p_open)) {
				ImGui::End();
				return;
			}
			// Options menu
			if (ImGui::BeginPopup("Options")) {
				ImGui::Checkbox("Auto-scroll", &AutoScroll);
				ImGui::EndPopup();
			}
			// Main window
			if (ImGui::Button("Options"))
				ImGui::OpenPopup("Options");
			ImGui::SameLine();
			bool clear = ImGui::Button("Clear");
			ImGui::SameLine();
			bool copy = ImGui::Button("Copy");
			ImGui::SameLine();
			Filter.Draw("Filter", -100.0f);
			ImGui::Separator();
			ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
			if (clear)
				Clear();
			if (copy)
				ImGui::LogToClipboard();
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
			const char* buf = Buf.begin();
			const char* buf_end = Buf.end();
			if (Filter.IsActive()) {
				for (int line_no = 0; line_no < LineOffsets.Size; line_no++) {
					const char* line_start = buf + LineOffsets[line_no];
					const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
					if (Filter.PassFilter(line_start, line_end))
						ImGui::TextUnformatted(line_start, line_end);
				}
			}
			else
			{
				ImGuiListClipper clipper;
				clipper.Begin(LineOffsets.Size);
				while (clipper.Step())
				{
					for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
					{
						const char* line_start = buf + LineOffsets[line_no];
						const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
						ImGui::TextUnformatted(line_start, line_end);
					}
				}
				clipper.End();
			}
			ImGui::PopStyleVar();
			if (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);
			ImGui::EndChild();
			ImGui::End();
		}
	};
	int add_log(struct Display_Log* log_ptr, char* ptr, int n);
}
#endif//_IMGUI_LOG_H_
#ifdef  LOG_IMPLEMENTATION
#ifndef LOG_IMPLEMENTED
#define LOG_IMPLEMENTED
//#include<time.h>
namespace ui::log {
	int add_log(struct Display_Log* log_ptr, char* ptr, int n)
	{
		struct timeb tb;
		ftime(&tb);
		struct tm t;
		time_t now;
		time(&now);
		localtime_s(&t, &now);
		char* add_log_buff = (char*)malloc(500);
		memset(add_log_buff, 0x00, 500);
		sprintf(add_log_buff, "[%d-%d-%d %d:%d:%d:%03d] # RECV DATA:", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, tb.millitm);
		memcpy(add_log_buff + strlen(add_log_buff), ptr, n);
		add_log_buff[strlen(add_log_buff)] = '\0';
		log_ptr->AddLog("%s", add_log_buff);
		return 0;
	}
}
#endif  //!LOG_IMPLEMENTED
#endif //LOG_IMPLEMENTATION
