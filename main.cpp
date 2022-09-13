#define INK_UPDATE_INTERVAL 6.5f

#include <SDL2/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_sdlrenderer.h>
#include <Windows.h>
#include <winspool.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <atomic>
#include <thread>
#include "json.hpp"

struct {
	float blackValue;
	float colorValue;
} ink;

using namespace std;

class CInkLevelUpdater {
private:
	std::string m_path;
	std::string m_working_path;
	std::string m_arguments;
	std::atomic_bool m_locked;
	std::string m_json_file;
	volatile bool m_errored;
	float m_time;

	float m_triColorValue;
	float m_blackValue;

public:
	CInkLevelUpdater() :
		m_working_path("C:\\Program Files\\HP\\HP Deskjet 1510 series\\Bin"),
		m_arguments("BR43O1D70B05XJ:USB /inkLevels"),
		m_json_file("C:\\ProgramData\\HP\\HP Deskjet 1510 series\\HPUDC\\HP Deskjet 1510 series\\UDC_inklevel.json"),
		m_locked(false),
		m_blackValue(0.f),
		m_triColorValue(0.f),
		m_time(INK_UPDATE_INTERVAL + 0.1f)
	{
		m_path = m_working_path + "\\Toolbox.exe";
	}

	void Update(float dt) {
		m_time += dt;

		if (m_time >= INK_UPDATE_INTERVAL && !m_locked.load()) {
			m_locked.exchange(true);

			std::thread th([this]() {
				this->Refresh();
				});

			th.detach();
		}
	}

	float GetBlackValue() {
		return m_blackValue;
	}

	float GetTriColorValue() {
		return m_triColorValue;
	}

private:
	void Refresh()
	{
		std::stringstream ss;

		{
			fstream fs;
			fs.open(m_json_file, ios::in);
			char buff[1024];
			fs.read(buff, 1024);
			ss << buff;
			ss.seekg(0);
		}

		nlohmann::json j;
		ss >> j;

#define BIND_VALUE(K, P) do {\
				auto& p = j.at(K);\
				P = p["INK_LEVEL"].get<int>() / 100.f;\
				printf("<INK LEVEL CHECK> Update '" #P "' to '%.2f'\n", P);\
			} while (0);

		if (j.is_array()) {
			BIND_VALUE(0, m_triColorValue);
			BIND_VALUE(1, m_blackValue);

#undef BIND_VALUE
		}

		m_time = 0.f;
		m_locked.store(false);
	}
};

struct SInkLevelColorizer {
public:
	float minLevel;
	float maxLevel;
	UINT color;

	bool IsInRange(float value) {
		return value >= minLevel && value <= maxLevel;
	}
};

#define NUM_STYLES 5

static SInkLevelColorizer colorizers[NUM_STYLES] =
{
	{   0.f, 0.2f, IM_COL32(255,   0, 0, 255) },
	{ 0.21f, 0.5f, IM_COL32(255,  60, 0, 255) },
	{ 0.51f, 0.6f, IM_COL32(255, 166, 0, 255) },
	{ 0.61f, 0.8f, IM_COL32(191, 255, 0, 255) },
	{ 0.81f, 1.f , IM_COL32(  0, 255, 0, 255) }
};

void ProgressBarStyled(float percent) {
	bool popStyle = false;

	for (size_t i = 0; i < NUM_STYLES; i++) {
		auto& it = colorizers[i];

		if (it.IsInRange(percent)) {
			popStyle = true;
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, it.color);
		}
	}

	ImGui::ProgressBar(percent);

	if (popStyle)
		ImGui::PopStyleColor();
}

int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	CInkLevelUpdater inkLevel;

#if defined(_DEBUG) && defined(_LOGGING)
	AllocConsole();
	freopen("conout$", "w", stdout);
#endif

	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window* window = SDL_CreateWindow("HP 1516 - Niveis de Tinta", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 62, SDL_WINDOW_UTILITY);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	ImGui::CreateContext();
	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer_Init(renderer);
	auto& io = ImGui::GetIO();

	SDL_Event evt;

	while (true) {
		if (SDL_PollEvent(&evt)) {
			if (evt.type == SDL_QUIT) {
				break;
			}

			ImGui_ImplSDL2_ProcessEvent(&evt);
		}

		ImGui_ImplSDLRenderer_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(io.DisplaySize);

		ImGui::Begin("##", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
		{
			ImGui::BeginGroup();
			{
				ImGui::Indent();
				ImGui::Text("Cartucho Preto: ");
				ImGui::SameLine();
				ProgressBarStyled(inkLevel.GetBlackValue());
				
			}
			ImGui::EndGroup();

			ImGui::BeginGroup();
			{
				ImGui::Text("Cartucho Colorido: ");
				ImGui::SameLine();
				ProgressBarStyled(inkLevel.GetTriColorValue());
			}
			ImGui::EndGroup();
		}
		ImGui::End();

		inkLevel.Update(io.DeltaTime);

		ImGui::Render();
		SDL_SetRenderDrawColor(renderer, 25, 25, 25, 255);
		SDL_RenderClear(renderer);
		ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
		SDL_RenderPresent(renderer);
	}
}