#pragma once
#include "imgui.h"
#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_sdl3.h"
#include "aZeroEngine/Engine.hpp"

#pragma once

/*
Color definitions in ImGui are a good starting point,
but do not cover all the intricacies of Spectrum's possible colors
in controls and widgets.

One big difference is that ImGui communicates widget activity
(hover, pressed) with their background, while spectrum uses a mix
of background and border, with border being the most common choice.

Because of this, we reference extra colors in spectrum from
imgui.cpp and imgui_widgets.cpp directly, and to make that work,
we need to have them defined at here at compile time.
*/

/// Pick one, or have one defined already.
#if !defined(SPECTRUM_USE_LIGHT_THEME) && !defined(SPECTRUM_USE_DARK_THEME)
#define SPECTRUM_USE_DARK_THEME
//#define SPECTRUM_USE_DARK_THEME
#endif

namespace ImGui {
    namespace Spectrum {
        // a list of changes introduced to change the look of the widgets. 
        // Collected here as const rather than being magic numbers spread 
        // around imgui.cpp and imgui_widgets.cpp.
        const float CHECKBOX_BORDER_SIZE = 2.0f;
        const float CHECKBOX_ROUNDING = 2.0f;

        // Load SourceSansProRegular and sets it as a default font.
        // You may want to call ImGui::GetIO().Fonts->Clear() before this
        void LoadFont(float size = 16.0f);

        // Sets the ImGui style to Spectrum
        void StyleColorsSpectrum();

        namespace { // Unnamed namespace, since we only use this here. 
            unsigned int Color(unsigned int c) {
                // add alpha.
                // also swap red and blue channel for some reason.
                // todo: figure out why, and fix it.
                const short a = 0xFF;
                const short r = (c >> 16) & 0xFF;
                const short g = (c >> 8) & 0xFF;
                const short b = (c >> 0) & 0xFF;
                return(a << 24)
                    | (r << 0)
                    | (g << 8)
                    | (b << 16);
            }
        }
        // all colors are from http://spectrum.corp.adobe.com/color.html

        inline unsigned int color_alpha(unsigned int alpha, unsigned int c) {
            return ((alpha & 0xFF) << 24) | (c & 0x00FFFFFF);
        }

        namespace Static { // static colors
            const unsigned int NONE = 0x00000000; // transparent
            const unsigned int WHITE = Color(0xFFFFFF);
            const unsigned int BLACK = Color(0x000000);
            const unsigned int GRAY200 = Color(0xF4F4F4);
            const unsigned int GRAY300 = Color(0xEAEAEA);
            const unsigned int GRAY400 = Color(0xD3D3D3);
            const unsigned int GRAY500 = Color(0xBCBCBC);
            const unsigned int GRAY600 = Color(0x959595);
            const unsigned int GRAY700 = Color(0x767676);
            const unsigned int GRAY800 = Color(0x505050);
            const unsigned int GRAY900 = Color(0x323232);
            const unsigned int BLUE400 = Color(0x378EF0);
            const unsigned int BLUE500 = Color(0x2680EB);
            const unsigned int BLUE600 = Color(0x1473E6);
            const unsigned int BLUE700 = Color(0x0D66D0);
            const unsigned int RED400 = Color(0xEC5B62);
            const unsigned int RED500 = Color(0xE34850);
            const unsigned int RED600 = Color(0xD7373F);
            const unsigned int RED700 = Color(0xC9252D);
            const unsigned int ORANGE400 = Color(0xF29423);
            const unsigned int ORANGE500 = Color(0xE68619);
            const unsigned int ORANGE600 = Color(0xDA7B11);
            const unsigned int ORANGE700 = Color(0xCB6F10);
            const unsigned int GREEN400 = Color(0x33AB84);
            const unsigned int GREEN500 = Color(0x2D9D78);
            const unsigned int GREEN600 = Color(0x268E6C);
            const unsigned int GREEN700 = Color(0x12805C);
        }

#ifdef SPECTRUM_USE_LIGHT_THEME
        const unsigned int GRAY50 = Color(0xFFFFFF);
        const unsigned int GRAY75 = Color(0xFAFAFA);
        const unsigned int GRAY100 = Color(0xF5F5F5);
        const unsigned int GRAY200 = Color(0xEAEAEA);
        const unsigned int GRAY300 = Color(0xE1E1E1);
        const unsigned int GRAY400 = Color(0xCACACA);
        const unsigned int GRAY500 = Color(0xB3B3B3);
        const unsigned int GRAY600 = Color(0x8E8E8E);
        const unsigned int GRAY700 = Color(0x707070);
        const unsigned int GRAY800 = Color(0x4B4B4B);
        const unsigned int GRAY900 = Color(0x2C2C2C);
        const unsigned int BLUE400 = Color(0x2680EB);
        const unsigned int BLUE500 = Color(0x1473E6);
        const unsigned int BLUE600 = Color(0x0D66D0);
        const unsigned int BLUE700 = Color(0x095ABA);
        const unsigned int RED400 = Color(0xE34850);
        const unsigned int RED500 = Color(0xD7373F);
        const unsigned int RED600 = Color(0xC9252D);
        const unsigned int RED700 = Color(0xBB121A);
        const unsigned int ORANGE400 = Color(0xE68619);
        const unsigned int ORANGE500 = Color(0xDA7B11);
        const unsigned int ORANGE600 = Color(0xCB6F10);
        const unsigned int ORANGE700 = Color(0xBD640D);
        const unsigned int GREEN400 = Color(0x2D9D78);
        const unsigned int GREEN500 = Color(0x268E6C);
        const unsigned int GREEN600 = Color(0x12805C);
        const unsigned int GREEN700 = Color(0x107154);
        const unsigned int INDIGO400 = Color(0x6767EC);
        const unsigned int INDIGO500 = Color(0x5C5CE0);
        const unsigned int INDIGO600 = Color(0x5151D3);
        const unsigned int INDIGO700 = Color(0x4646C6);
        const unsigned int CELERY400 = Color(0x44B556);
        const unsigned int CELERY500 = Color(0x3DA74E);
        const unsigned int CELERY600 = Color(0x379947);
        const unsigned int CELERY700 = Color(0x318B40);
        const unsigned int MAGENTA400 = Color(0xD83790);
        const unsigned int MAGENTA500 = Color(0xCE2783);
        const unsigned int MAGENTA600 = Color(0xBC1C74);
        const unsigned int MAGENTA700 = Color(0xAE0E66);
        const unsigned int YELLOW400 = Color(0xDFBF00);
        const unsigned int YELLOW500 = Color(0xD2B200);
        const unsigned int YELLOW600 = Color(0xC4A600);
        const unsigned int YELLOW700 = Color(0xB79900);
        const unsigned int FUCHSIA400 = Color(0xC038CC);
        const unsigned int FUCHSIA500 = Color(0xB130BD);
        const unsigned int FUCHSIA600 = Color(0xA228AD);
        const unsigned int FUCHSIA700 = Color(0x93219E);
        const unsigned int SEAFOAM400 = Color(0x1B959A);
        const unsigned int SEAFOAM500 = Color(0x16878C);
        const unsigned int SEAFOAM600 = Color(0x0F797D);
        const unsigned int SEAFOAM700 = Color(0x096C6F);
        const unsigned int CHARTREUSE400 = Color(0x85D044);
        const unsigned int CHARTREUSE500 = Color(0x7CC33F);
        const unsigned int CHARTREUSE600 = Color(0x73B53A);
        const unsigned int CHARTREUSE700 = Color(0x6AA834);
        const unsigned int PURPLE400 = Color(0x9256D9);
        const unsigned int PURPLE500 = Color(0x864CCC);
        const unsigned int PURPLE600 = Color(0x7A42BF);
        const unsigned int PURPLE700 = Color(0x6F38B1);
#endif
#ifdef SPECTRUM_USE_DARK_THEME
        const unsigned int GRAY50 = Color(0x252525);
        const unsigned int GRAY75 = Color(0x2F2F2F);
        const unsigned int GRAY100 = Color(0x323232);
        const unsigned int GRAY200 = Color(0x393939);
        const unsigned int GRAY300 = Color(0x3E3E3E);
        const unsigned int GRAY400 = Color(0x4D4D4D);
        const unsigned int GRAY500 = Color(0x5C5C5C);
        const unsigned int GRAY600 = Color(0x7B7B7B);
        const unsigned int GRAY700 = Color(0x999999);
        const unsigned int GRAY800 = Color(0xCDCDCD);
        const unsigned int GRAY900 = Color(0xFFFFFF);
        const unsigned int BLUE400 = Color(0x2680EB);
        const unsigned int BLUE500 = Color(0x378EF0);
        const unsigned int BLUE600 = Color(0x4B9CF5);
        const unsigned int BLUE700 = Color(0x5AA9FA);
        const unsigned int RED400 = Color(0xE34850);
        const unsigned int RED500 = Color(0xEC5B62);
        const unsigned int RED600 = Color(0xF76D74);
        const unsigned int RED700 = Color(0xFF7B82);
        const unsigned int ORANGE400 = Color(0xE68619);
        const unsigned int ORANGE500 = Color(0xF29423);
        const unsigned int ORANGE600 = Color(0xF9A43F);
        const unsigned int ORANGE700 = Color(0xFFB55B);
        const unsigned int GREEN400 = Color(0x2D9D78);
        const unsigned int GREEN500 = Color(0x33AB84);
        const unsigned int GREEN600 = Color(0x39B990);
        const unsigned int GREEN700 = Color(0x3FC89C);
        const unsigned int INDIGO400 = Color(0x6767EC);
        const unsigned int INDIGO500 = Color(0x7575F1);
        const unsigned int INDIGO600 = Color(0x8282F6);
        const unsigned int INDIGO700 = Color(0x9090FA);
        const unsigned int CELERY400 = Color(0x44B556);
        const unsigned int CELERY500 = Color(0x4BC35F);
        const unsigned int CELERY600 = Color(0x51D267);
        const unsigned int CELERY700 = Color(0x58E06F);
        const unsigned int MAGENTA400 = Color(0xD83790);
        const unsigned int MAGENTA500 = Color(0xE2499D);
        const unsigned int MAGENTA600 = Color(0xEC5AAA);
        const unsigned int MAGENTA700 = Color(0xF56BB7);
        const unsigned int YELLOW400 = Color(0xDFBF00);
        const unsigned int YELLOW500 = Color(0xEDCC00);
        const unsigned int YELLOW600 = Color(0xFAD900);
        const unsigned int YELLOW700 = Color(0xFFE22E);
        const unsigned int FUCHSIA400 = Color(0xC038CC);
        const unsigned int FUCHSIA500 = Color(0xCF3EDC);
        const unsigned int FUCHSIA600 = Color(0xD951E5);
        const unsigned int FUCHSIA700 = Color(0xE366EF);
        const unsigned int SEAFOAM400 = Color(0x1B959A);
        const unsigned int SEAFOAM500 = Color(0x20A3A8);
        const unsigned int SEAFOAM600 = Color(0x23B2B8);
        const unsigned int SEAFOAM700 = Color(0x26C0C7);
        const unsigned int CHARTREUSE400 = Color(0x85D044);
        const unsigned int CHARTREUSE500 = Color(0x8EDE49);
        const unsigned int CHARTREUSE600 = Color(0x9BEC54);
        const unsigned int CHARTREUSE700 = Color(0xA3F858);
        const unsigned int PURPLE400 = Color(0x9256D9);
        const unsigned int PURPLE500 = Color(0x9D64E1);
        const unsigned int PURPLE600 = Color(0xA873E9);
        const unsigned int PURPLE700 = Color(0xB483F0);
#endif
    }
}


namespace aZero
{
	void StyleColorsSpectrum() 
    {
		ImGuiStyle* style = &ImGui::GetStyle();
		style->GrabRounding = 4.0f;

        using namespace ImGui;
		ImVec4* colors = style->Colors;
		colors[ImGuiCol_Text] = ColorConvertU32ToFloat4(Spectrum::GRAY800); // text on hovered controls is gray900
		colors[ImGuiCol_TextDisabled] = ColorConvertU32ToFloat4(Spectrum::GRAY500);
		colors[ImGuiCol_WindowBg] = ColorConvertU32ToFloat4(Spectrum::GRAY100);
		colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_PopupBg] = ColorConvertU32ToFloat4(Spectrum::GRAY50); // not sure about this. Note: applies to tooltips too.
		colors[ImGuiCol_Border] = ColorConvertU32ToFloat4(Spectrum::GRAY300);
		colors[ImGuiCol_BorderShadow] = ColorConvertU32ToFloat4(Spectrum::Static::NONE); // We don't want shadows. Ever.
		colors[ImGuiCol_FrameBg] = ColorConvertU32ToFloat4(Spectrum::GRAY75); // this isnt right, spectrum does not do this, but it's a good fallback
		colors[ImGuiCol_FrameBgHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY50);
		colors[ImGuiCol_FrameBgActive] = ColorConvertU32ToFloat4(Spectrum::GRAY200);
		colors[ImGuiCol_TitleBg] = ColorConvertU32ToFloat4(Spectrum::GRAY300); // those titlebar values are totally made up, spectrum does not have this.
		colors[ImGuiCol_TitleBgActive] = ColorConvertU32ToFloat4(Spectrum::GRAY200);
		colors[ImGuiCol_TitleBgCollapsed] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
		colors[ImGuiCol_MenuBarBg] = ColorConvertU32ToFloat4(Spectrum::GRAY100);
		colors[ImGuiCol_ScrollbarBg] = ColorConvertU32ToFloat4(Spectrum::GRAY100); // same as regular background
		colors[ImGuiCol_ScrollbarGrab] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
		colors[ImGuiCol_ScrollbarGrabHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY600);
		colors[ImGuiCol_ScrollbarGrabActive] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
		colors[ImGuiCol_CheckMark] = ColorConvertU32ToFloat4(Spectrum::BLUE500);
		colors[ImGuiCol_SliderGrab] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
		colors[ImGuiCol_SliderGrabActive] = ColorConvertU32ToFloat4(Spectrum::GRAY800);
		colors[ImGuiCol_Button] = ColorConvertU32ToFloat4(Spectrum::GRAY75); // match default button to Spectrum's 'Action Button'.
		colors[ImGuiCol_ButtonHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY50);
		colors[ImGuiCol_ButtonActive] = ColorConvertU32ToFloat4(Spectrum::GRAY200);
		colors[ImGuiCol_Header] = ColorConvertU32ToFloat4(Spectrum::BLUE400);
		colors[ImGuiCol_HeaderHovered] = ColorConvertU32ToFloat4(Spectrum::BLUE500);
		colors[ImGuiCol_HeaderActive] = ColorConvertU32ToFloat4(Spectrum::BLUE600);
		colors[ImGuiCol_Separator] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
		colors[ImGuiCol_SeparatorHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY600);
		colors[ImGuiCol_SeparatorActive] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
		colors[ImGuiCol_ResizeGrip] = ColorConvertU32ToFloat4(Spectrum::GRAY400);
		colors[ImGuiCol_ResizeGripHovered] = ColorConvertU32ToFloat4(Spectrum::GRAY600);
		colors[ImGuiCol_ResizeGripActive] = ColorConvertU32ToFloat4(Spectrum::GRAY700);
		colors[ImGuiCol_PlotLines] = ColorConvertU32ToFloat4(Spectrum::BLUE400);
		colors[ImGuiCol_PlotLinesHovered] = ColorConvertU32ToFloat4(Spectrum::BLUE600);
		colors[ImGuiCol_PlotHistogram] = ColorConvertU32ToFloat4(Spectrum::BLUE400);
		colors[ImGuiCol_PlotHistogramHovered] = ColorConvertU32ToFloat4(Spectrum::BLUE600);
		colors[ImGuiCol_TextSelectedBg] = ColorConvertU32ToFloat4((Spectrum::BLUE400 & 0x00FFFFFF) | 0x33000000);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_NavHighlight] = ColorConvertU32ToFloat4((Spectrum::GRAY900 & 0x00FFFFFF) | 0x0A000000);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
	}

    void StyleTransparent()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        // ---- Layout / Shape
        style.WindowRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding = 4.0f;

        style.Alpha = 0.9;

        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.PopupBorderSize = 1.0f;

        style.WindowPadding = ImVec2(10, 10);
        style.FramePadding = ImVec2(8, 4);
        style.ItemSpacing = ImVec2(8, 6);

        // ---- Colors ----
        ImVec4* colors = style.Colors;

        ImVec4 purple = ImVec4(0.50f, 0.35f, 0.67f, 1.00f); // main accent
        ImVec4 purple_hover = ImVec4(0.60f, 0.45f, 0.77f, 1.00f);
        ImVec4 purple_active = ImVec4(0.45f, 0.30f, 0.60f, 1.00f);

        ImVec4 bg_dark = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
        ImVec4 bg_mid = ImVec4(0.22f, 0.22f, 0.25f, 1.00f);
        ImVec4 bg_light = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);

        // Text
        colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.60f, 1.00f);

        // Backgrounds
        colors[ImGuiCol_WindowBg] = bg_dark;
        colors[ImGuiCol_ChildBg] = bg_dark;
        colors[ImGuiCol_PopupBg] = bg_mid;

        // Borders
        colors[ImGuiCol_Border] = ImVec4(0.35f, 0.35f, 0.40f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

        // Frames
        colors[ImGuiCol_FrameBg] = bg_mid;
        colors[ImGuiCol_FrameBgHovered] = bg_light;
        colors[ImGuiCol_FrameBgActive] = bg_light;

        // Title bar
        colors[ImGuiCol_TitleBg] = purple_active;
        colors[ImGuiCol_TitleBgActive] = purple;
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(purple.x, purple.y, purple.z, 0.5f);

        // Buttons
        colors[ImGuiCol_Button] = purple_active;
        colors[ImGuiCol_ButtonHovered] = purple_hover;
        colors[ImGuiCol_ButtonActive] = purple;

        // Headers
        colors[ImGuiCol_Header] = purple_active;
        colors[ImGuiCol_HeaderHovered] = purple_hover;
        colors[ImGuiCol_HeaderActive] = purple;

        // Tabs
        colors[ImGuiCol_Tab] = bg_mid;
        colors[ImGuiCol_TabHovered] = purple_hover;
        colors[ImGuiCol_TabActive] = purple;
        colors[ImGuiCol_TabUnfocused] = bg_mid;
        colors[ImGuiCol_TabUnfocusedActive] = purple_active;

        // Sliders / checkmarks
        colors[ImGuiCol_SliderGrab] = purple_hover;
        colors[ImGuiCol_SliderGrabActive] = purple;
        colors[ImGuiCol_CheckMark] = purple_hover;

        // Scrollbar
        colors[ImGuiCol_ScrollbarBg] = bg_dark;
        colors[ImGuiCol_ScrollbarGrab] = bg_light;
        colors[ImGuiCol_ScrollbarGrabHovered] = purple_hover;
        colors[ImGuiCol_ScrollbarGrabActive] = purple;

        // Resize grip
        colors[ImGuiCol_ResizeGrip] = purple_active;
        colors[ImGuiCol_ResizeGripHovered] = purple_hover;
        colors[ImGuiCol_ResizeGripActive] = purple;

        // Separator
        colors[ImGuiCol_Separator] = bg_light;
        colors[ImGuiCol_SeparatorHovered] = purple_hover;
        colors[ImGuiCol_SeparatorActive] = purple;
    }

	class ImGui_Wrapper
	{
	public:
		static void Init(Rendering::Renderer& renderer, SDL_Window* window)
		{
			float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
			io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

			// Setup Dear ImGui style
			//ImGui::StyleColorsDark();
			//ImGui::StyleColorsLight();


			ImGui_ImplSDL3_InitForD3D(window);

            // Setup scaling
            ImGuiStyle& style = ImGui::GetStyle();
            style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
            style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

			ImGui_ImplDX12_InitInfo init_info = {};
			init_info.Device = renderer.GetResourceHeap().GetDevice();
			init_info.CommandQueue = renderer.GetGraphicsCommandQueue().Get();
			init_info.NumFramesInFlight = renderer.GetBufferingCount();
			init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
			init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
			init_info.UserData = &renderer.GetResourceHeap();

			// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
			// (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
			init_info.SrvDescriptorHeap = renderer.GetResourceHeap().Get();

			init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) {
				RenderAPI::DescriptorHeap* heap = static_cast<RenderAPI::DescriptorHeap*>(info->UserData);
				auto descriptor = heap->CreateDescriptor(false); // Leak
				*out_cpu_handle = descriptor.GetCpuHandle();
				*out_gpu_handle = descriptor.GetGpuHandle();
				};

			init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) {};

			ImGui_ImplDX12_Init(&init_info);

           // StyleColorsSpectrum();
            StyleTransparent();
		}

		static void BeginFrame()
		{
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplSDL3_NewFrame();
			ImGui::NewFrame();
		}

		static void HandleMultiViewport()
		{
			ImGuiIO& io = ImGui::GetIO();
			// Update and Render additional Platform Windows
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault();
			}
		}
	};
}