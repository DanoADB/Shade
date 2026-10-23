/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <iostream>
#include <limits>
#include <map>
#include <memory>

#include <SDL3/SDL_messagebox.h>
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <SDL3/SDL.h>
#endif
#include <cxxopts.hpp>

#ifdef _WIN32
// clang-format off
// can't sort these includes
#include <wtypes.h>
#include <winreg.h>
// clang-format on
#endif

#include "EngineConfig.h"
#include "Game.h"

bool parseOptions(int argc, char** argv, openblack::Arguments& args, int& returnCode)
{
	cxxopts::Options options("openblack", "Open source reimplementation of the game Black & White (2001).");

	const std::string defaultLogFile =
#if defined(OPENBLACK_DEBUG) || defined(__EMSCRIPTEN__)
	    "stdout";
#else
	    "openblack.log";
#endif

	std::string loggingSubsystems = "all";
	for (const auto& system : openblack::k_LoggingSubsystemStrs)
	{
		loggingSubsystems += std::string(", ") + system.data();
	}

	// clang-format off
	options.add_options()
		("h,help", "Display this help message.")
		("g,game-path", "Path to the Data/ and Scripts/ directories of the original Black & White game. (Required)", cxxopts::value<std::string>())
		("W,width", "Window resolution in the x axis.", cxxopts::value<uint16_t>()->default_value("1280"))
		("H,height", "Window resolution in the y axis.", cxxopts::value<uint16_t>()->default_value("1024"))
		("u,ui-scale", "Scaling of the GUI", cxxopts::value<float>()->default_value("1.0"))
		("s,start-level", "Level that is loaded at start-up", cxxopts::value<std::string>()->default_value("Land1.txt"))
		("V,vsync", "Enable Vertical Sync.")
		("m,window-mode", "Which mode to run window.", cxxopts::value<std::string>()->default_value("windowed"))
		("b,backend-type", "Which backend to use for rendering.", cxxopts::value<std::string>())
		("n,num-frames-to-simulate", "Number of frames to simulate before quitting.", cxxopts::value<uint32_t>()->default_value("0"))
		("l,log-file", "Output file for logs, 'stdout'/'logcat' for terminal output.", cxxopts::value<std::string>()->default_value(defaultLogFile))
		("L,log-level", "Level (trace, debug, info, warning, error, critical, off) of logging per subsystem (" + loggingSubsystems + ").",
		    cxxopts::value<std::vector<std::string>>()->default_value("all=debug"))
		("screenshot-frame", "Request a screenshot of the backbuffer at a certain frame number.", cxxopts::value<uint32_t>())
		("screenshot-path", "Path of the request a screenshot of the backbuffer.", cxxopts::value<std::filesystem::path>()->default_value("screenshot.png"))
	;
	// clang-format on

	try
	{
		auto result = options.parse(argc, argv);
		if (result["help"].as<bool>())
		{
			std::cout << options.help() << std::endl;
			returnCode = EXIT_SUCCESS;
			return false;
		}

		// pick a sane renderer based on the user os
		openblack::GraphicsBackend graphicsBackend;
#ifdef _APPLE_
		graphicsBackend = openblack::GraphicsBackend::Metal;
#else
		graphicsBackend = openblack::GraphicsBackend::Vulkan;
#endif

		// allow user to specify a renderer
		if (result.count("backend-type") != 0)
		{
			auto rendererIter = openblack::k_GraphicsBackendStringLookup.find(result["backend-type"].as<std::string>());
			if (rendererIter != openblack::k_GraphicsBackendStringLookup.cend())
			{
				graphicsBackend = rendererIter->second;
			}
			else
			{
				throw cxxopts::exceptions::no_such_option(result["backend-type"].as<std::string>());
			}
		}

		static const std::map<std::string_view, openblack::windowing::DisplayMode> displayModeLookup = {
		    std::pair {"windowed", openblack::windowing::DisplayMode::Windowed},
		    std::pair {"fullscreen", openblack::windowing::DisplayMode::Fullscreen},
		    std::pair {"borderless", openblack::windowing::DisplayMode::Borderless},
		};

		openblack::windowing::DisplayMode displayMode;
		auto displayModeIter = displayModeLookup.find(result["window-mode"].as<std::string>());
		if (displayModeIter != displayModeLookup.cend())
		{
			displayMode = displayModeIter->second;
		}
		else
		{
			throw cxxopts::exceptions::no_such_option(result["window-mode"].as<std::string>());
		}

		std::array<spdlog::level::level_enum, openblack::k_LoggingSubsystemStrs.size()> logLevels;
		{
			std::map<std::string, spdlog::level::level_enum> logLevelMap;
			logLevelMap.insert_or_assign("all", spdlog::level::debug);
			for (const auto& levelStr : result["log-level"].as<std::vector<std::string>>())
			{
				const auto delim = levelStr.find_first_of('=');
				const auto key = delim == std::string::npos ? "all" : levelStr.substr(0, delim);
				const auto value = spdlog::level::from_str(delim == std::string::npos ? levelStr : levelStr.substr(delim + 1));
				logLevelMap.insert_or_assign(key, value);
			}

			const auto all = logLevelMap["all"];
			for (auto& level : logLevels)
			{
				level = all;
			}
			// TODO (#749) use std::views::enumerate
			for (size_t i = 0; const auto& str : openblack::k_LoggingSubsystemStrs)
			{
				const auto iter = logLevelMap.find(str.data());
				if (iter != logLevelMap.cend())
				{
					logLevels.at(i) = iter->second;
				}
				++i;
			}
		}

		args.executablePath = argv[0];
		if (result.count("game-path") == 0)
		{
#ifdef _WIN32
			// if we're on windows we can find the install path
			DWORD dataLen = 0;
			LSTATUS status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir",
			                             RRF_RT_REG_SZ, nullptr, nullptr, &dataLen);
			if (status == ERROR_SUCCESS)
			{
				char* path = new char[dataLen];
				status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir",
				                     RRF_RT_REG_SZ, nullptr, path, &dataLen);

				args.gamePath = std::string(path);
			}
			else
#endif
			{
				throw cxxopts::exceptions::option_has_no_value("game-path");
			}
		}
		else
		{
			args.gamePath = result["game-path"].as<std::string>();
		}

		if (result.count("screenshot-frame") != 0)
		{
			args.requestScreenshot = std::make_pair(result["screenshot-frame"].as<uint32_t>(),
			                                        result["screenshot-path"].as<std::filesystem::path>());
		}

		args.windowWidth = result["width"].as<uint16_t>();
		args.windowHeight = result["height"].as<uint16_t>();
		args.guiScale = result["ui-scale"].as<float>();
		args.vsync = result["vsync"].as<bool>();
		args.displayMode = displayMode;
		args.graphicsBackend = graphicsBackend;
		args.numFramesToSimulate = result["num-frames-to-simulate"].as<uint32_t>();
		args.logFile = result["log-file"].as<std::string>();
		args.logLevels = logLevels;
		args.startLevel = result["start-level"].as<std::string>();
	}
	catch (cxxopts::exceptions::parsing& err)
	{
		std::cerr << err.what() << std::endl;
		std::cerr << options.help() << std::endl;

		returnCode = EXIT_FAILURE;
		return false;
	}

	return true;
}

int main(int argc, char* argv[]) noexcept
{
	// clang-format off
	std::cout <<
	    "==============================================================================\n"
	    "   openblack - A modern reimplementation of Lionhead's Black & White (2001)   \n"
	    "==============================================================================\n"
	    "\n";
	// clang-format on

	try
	{
		openblack::Arguments args;
		int returnCode = EXIT_FAILURE;
		if (!parseOptions(argc, argv, args, returnCode))
		{
			return returnCode;
		}
		auto game = std::make_unique<openblack::Game>(std::move(args));
		if (!game->Initialize())
		{
			return EXIT_FAILURE;
		}
		if (!game->Run())
		{
			return EXIT_FAILURE;
		}
#ifdef __EMSCRIPTEN__
		// Emscripten owns the animation frame scheduling. Keep Game alive until
		// the user quits or the frame loop terminates.
		emscripten_set_main_loop_arg(
		    [](void* context) {
			    auto* runningGame = static_cast<openblack::Game*>(context);
			    if (!runningGame->Frame())
			    {
				    EM_ASM({if(Module.shadeStop)Module.shadeStop('Engine stopped. Reload to start again.');});
				    emscripten_cancel_main_loop();
				    delete runningGame;
			    }
		    },
		    game.release(), 0, 1);
#endif
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal error", e.what(), nullptr);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

#if defined(_WIN32) && !defined(_CONSOLE)
int WINAPI WinMain([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance,
                   [[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nShowCmd)
{
	return main(__argc, __argv);
}
#endif

// Development navigation for checking the engine's loaded original scene.
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include "Camera/Camera.h"
#include "Locator.h"
extern "C" EMSCRIPTEN_KEEPALIVE int shade_village_view(int closeup)
{
    if (!openblack::Locator::camera::has_value()) return 0;
    auto& camera=openblack::Locator::camera::value();
    camera.SetFocus({1920.0f,48.0f,2700.0f});
    camera.SetOrigin(closeup ? glm::vec3(1960.0f,110.0f,2600.0f) : glm::vec3(2120.0f,450.0f,2400.0f));
    return 1;
}

#include "Windowing/WindowingInterface.h"
extern "C" EMSCRIPTEN_KEEPALIVE int shade_control(int action, int pressed)
{
    if (!openblack::Locator::gameActionSystem::has_value()) return 0;
    static constexpr SDL_Scancode keys[] = {SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN,
                                            SDL_SCANCODE_Z, SDL_SCANCODE_U, SDL_SCANCODE_A, SDL_SCANCODE_Q};
    SDL_Event event {};
    if (action >= 0 && action < 8)
    {
        event.type = pressed ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        event.key.scancode = keys[action];
        event.key.key = SDL_GetKeyFromScancode(keys[action], SDL_KMOD_NONE, false);
        event.key.down = pressed != 0;
    }
    else if ((action == 8 || action == 9) && pressed)
    {
        event.type = SDL_EVENT_MOUSE_WHEEL;
        event.wheel.y = action == 8 ? 1.0f : -1.0f;
    }
    else return 0;
    return SDL_PushEvent(&event) ? 1 : 0;
}
extern "C" EMSCRIPTEN_KEEPALIVE int shade_resize(int width, int height)
{
    if (!openblack::Locator::windowing::has_value() || width < 1 || height < 1 || width > 4096 || height > 4096) return 0;
    auto& window = openblack::Locator::windowing::value();
    if (window.GetSize() == glm::ivec2(width, height)) return 1;
    return SDL_SetWindowSize(static_cast<SDL_Window*>(window.GetHandle()), width, height) ? 1 : 0;
}

#include "ECS/Registry.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/Transform.h"
#include <glm/geometric.hpp>
#include "BrowserGameplay.h"
#include "Debug/DebugGuiInterface.h"
extern "C" EMSCRIPTEN_KEEPALIVE void shade_pause_loop(){emscripten_pause_main_loop();}
static openblack::browser::Gameplay& BrowserGameplay()
{
    static openblack::browser::Gameplay gameplay;
    return gameplay;
}
extern "C" EMSCRIPTEN_KEEPALIVE void shade_gameplay(int action,float x,float y)
{
    if(action==1 && openblack::Locator::debugGui::has_value() && openblack::Locator::debugGui::value().StealsFocus())return;
    BrowserGameplay().Run(action,x,y);
}
extern "C" EMSCRIPTEN_KEEPALIVE int shade_gameplay_restore(int kind,uint32_t id,float x,float y,float z,uint32_t amount)
{
    return BrowserGameplay().Restore(kind,id,x,y,z,amount);
}
extern "C" EMSCRIPTEN_KEEPALIVE int shade_villager_view()
{
    using namespace openblack;
    if (!Locator::camera::has_value() || !Locator::entitiesRegistry::has_value()) return 0;
    float nearest=std::numeric_limits<float>::max();
    glm::vec3 target{};
    Locator::entitiesRegistry::value().Each<const ecs::components::Villager,const ecs::components::Transform>(
        [&](const auto&,const auto& transform) {
            const auto delta=transform.position-glm::vec3(1920.0f,48.0f,2700.0f);
            const float distance=glm::dot(delta,delta);
            if(distance<nearest) {nearest=distance;target=transform.position;}
        });
    if(nearest==std::numeric_limits<float>::max())return 0;
    Locator::camera::value().SetFocus(target+glm::vec3(0,1,0)).SetOrigin(target+glm::vec3(3,3,-5));
    return 1;
}
#endif

#ifdef __EMSCRIPTEN__
#include "BrowserDrought.h"
extern "C" EMSCRIPTEN_KEEPALIVE int shade_drought_routes(){return openblack::browser::DroughtRoutes();}
extern "C" EMSCRIPTEN_KEEPALIVE void shade_drought_weather(float dry,float rain){openblack::browser::droughtDryness=std::clamp(dry,0.f,1.f);openblack::browser::droughtRain=std::clamp(rain,0.f,1.f);}
extern "C" EMSCRIPTEN_KEEPALIVE void shade_drought_restore(int deliveries){openblack::browser::RestoreDroughtCounters(deliveries);}
extern "C" EMSCRIPTEN_KEEPALIVE int shade_drought_god(int selection){return openblack::browser::SwapDroughtGod(selection);}
extern "C" EMSCRIPTEN_KEEPALIVE void shade_drought_animate(int phase,float progress,float time,int paused,int deliveries){openblack::browser::AnimateDroughtCreature(phase,progress,time,paused,deliveries);}
extern "C" EMSCRIPTEN_KEEPALIVE void shade_drought_world(int discovered,int channel,float progress,float time){openblack::browser::UpdateDroughtWorld(discovered,channel,progress,time);}
extern "C" EMSCRIPTEN_KEEPALIVE void shade_drought_worker(int index,int job,float progress){openblack::browser::RunDroughtWorker(index,job,progress);}
extern "C" EMSCRIPTEN_KEEPALIVE int shade_drought(int phase,float progress,int deliveries,int focus,int reset)
{
    return openblack::browser::RunDrought(phase,progress,deliveries,focus,reset);
}
#endif
