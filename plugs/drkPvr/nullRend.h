#pragma once
#include "drkPvr.h"
#include "Renderer_if.h"

#if REND_API == REND_PSP
namespace NORenderer
{

	bool InitRenderer();
	void TermRenderer();
	void ResetRenderer(bool Manual);

	bool ThreadStart();
	void ThreadEnd();
	void VBlank();
	void StartRender();
	void EndRender();

	void ListCont();
	void ListInit();
	void SoftReset();

	void SetFpsText(char* text);

	void VramLockedWrite(vram_block* bl);
};
#define rend_init         NORenderer::InitRenderer
#define rend_term         NORenderer::TermRenderer
#define rend_reset        NORenderer::ResetRenderer

#define rend_thread_start NORenderer::ThreadStart
#define rend_thread_end	  NORenderer::ThreadEnd
#define rend_vblank       NORenderer::VBlank
#define rend_start_render NORenderer::StartRender
#define rend_end_render   NORenderer::EndRender

#define rend_list_cont NORenderer::ListCont
#define rend_list_init NORenderer::ListInit
#define rend_list_srst NORenderer::SoftReset

#define rend_text_invl NORenderer::VramLockedWrite
#define rend_set_fps_text NORenderer::SetFpsText
#define rend_set_render_rect(rect,sht)
#define rend_set_fb_scale(x,y)
#endif

