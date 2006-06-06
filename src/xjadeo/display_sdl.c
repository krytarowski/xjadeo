/* xjadeo - jack video monitor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  
 *
 * (c) 2006 
 *  Robin Gareus <robin@gareus.org>
 *  Luis Garrido <luisgarrido@users.sourceforge.net>
 *
 * this file was inspired by playdv source code of http://libdv.sourceforge.net/.
 *  - (c) 2000 Charles 'Buck' Krasic 
 *  - (c) 2000 Erik Walthinsen 
 *
 */

#include "xjadeo.h"
#include "display.h"


/*******************************************************************************
 * SDL
 */

#if HAVE_SDL

  SDL_Surface* sdl_screen;
  SDL_Overlay *sdl_overlay;
  SDL_Rect sdl_rect;
  int sdl_pic_format= SDL_YV12_OVERLAY; // fourcc

void close_window_sdl(void) {
	// TODO: free sdl stuff (sdl_overlay)
	SDL_Quit();
}


#if 0
#ifdef USE_SDLTTF
// OSD - experimental 
//
#include <SDL_ttf.h>

TTF_Font *font;

#define FONT_PTSIZE     18
#define SDL_FONTFILE	"arial.ttf"


SDL_Color white = { 0xFF, 0xFF, 0xFF, 0 };

int displaybox (SDL_Color *col, int yperc) {
	SDL_Rect dstrect;
	dstrect.x = (sdl_screen->w)*.2;
	dstrect.w = (sdl_screen->w)*.6;
	dstrect.y = (sdl_screen->h)*yperc/100; 
	dstrect.h = TTF_FontHeight(font);
	SDL_FillRect(sdl_screen, &dstrect, SDL_MapRGB(sdl_screen->format, col->r, col->g, col->b));
	SDL_UpdateRect(sdl_screen, dstrect.x, dstrect.y, dstrect.w, dstrect.h);
	return(0);
}

int InitTTF (void) {
	if(TTF_Init()==-1) {
		fprintf(stderr, "Error: unable to initialize TTF_SDL, %s\n", TTF_GetError());
		return (-1);
	}
//	atexit(TTF_Quit);
	font = TTF_OpenFont(SDL_FONTFILE, FONT_PTSIZE);
	if ( font == NULL ) {
		fprintf(stderr, "Error: Couldn't load %d pt font from %s: %s\n", FONT_PTSIZE, SDL_FONTFILE, SDL_GetError());
		return(-2);
	}
	TTF_SetFontStyle(font, TTF_STYLE_NORMAL);

	return (0);
}

int displaytext (char *string, SDL_Color *col, int yperc) {
        SDL_Surface *text;
        SDL_Rect dstrect;

        text = TTF_RenderText_Solid(font, string, *col);
        if ( text == NULL ) {
                fprintf(stderr, "Error: Couldn't render text: %s\n", SDL_GetError());
                return(-2);
        }

        dstrect.x = (sdl_screen->w - text->w)/2;
        dstrect.y = (sdl_screen->h)*yperc/100; 
	dstrect.w = text->w;
	dstrect.h = text->h;

        SDL_BlitSurface(text, NULL, sdl_screen, &dstrect);
	SDL_UpdateRect(sdl_screen, dstrect.x, dstrect.y, dstrect.w, dstrect.h);
        SDL_FreeSurface(text);
        return(0);

}
#endif /* USE_SDLTTF */
#endif /* 0 */

int open_window_sdl (int *argc, char ***argv) {
	const SDL_VideoInfo *video_info;
	int video_bpp;

	if(SDL_Init(SDL_INIT_VIDEO) < 0) goto no_sdl;
#if 0
	InitTTF(); // FIXME: exit if retval !=0..
#endif

	/* Get the "native" video mode */
	video_info = SDL_GetVideoInfo();
	switch (video_info->vfmt->BitsPerPixel) {
		case 16:
		case 32:
			video_bpp = video_info->vfmt->BitsPerPixel;
			break;
		default:
			video_bpp = 16;
			break;
	} 
// TODO : if no HWSURFACE overlay -> use RGB software rendering 
// - SDL_CreateRGBSurfaceFrom..
// - vidoutmodes "SDL RGB" and "SDL XV"!?
	sdl_screen = SDL_SetVideoMode(movie_width,movie_height, video_bpp,SDL_HWSURFACE | SDL_RESIZABLE);
	SDL_WM_SetCaption("xjadeo", "xjadeo");

// FIXME: on linux the SDL_*_OVERLAY are defined as FOURCC numbers rather than beeing abstract 
// so we could try other ffmpeg/lqt compatible 420P formats as I420 (0x30323449)
	sdl_overlay = SDL_CreateYUVOverlay(movie_width, movie_height, 0x30323449, sdl_screen);
	sdl_pic_format=0x30323449;
	if(!sdl_overlay || (!sdl_overlay->hw_overlay)) {
		sdl_overlay = SDL_CreateYUVOverlay(movie_width, movie_height, SDL_YV12_OVERLAY, sdl_screen);
		sdl_pic_format=SDL_YV12_OVERLAY;
	}
	if((!sdl_overlay || (!sdl_overlay->hw_overlay) || SDL_LockYUVOverlay(sdl_overlay)<0)) {
		goto no_overlay;
	}
	sdl_rect.x = 0;
	sdl_rect.y = 0;
	sdl_rect.w = sdl_overlay->w;
	sdl_rect.h = sdl_overlay->h;

	if ( sdl_overlay->pitches[0] != movie_width ||
			sdl_overlay->pitches[1] != sdl_overlay->pitches[2] ) {
		fprintf(stderr,"unsupported SDL YV12.\n"); 
		goto no_overlay;
	}  
#if 0  // verify YUV alignment
	if ((sdl_overlay->pixels[1] - sdl_overlay->pixels[0]) != ( movie_width * movie_height) ||
			(sdl_overlay->pixels[2] - sdl_overlay->pixels[1]) != ( movie_width * movie_height /4) ) {
		printf("unsupported SDL YV12 pixel buffer alignment.\n"); 
		goto no_overlay;
	}  
#endif
	return(0);
 
no_overlay: 
	if(sdl_overlay) SDL_FreeYUVOverlay(sdl_overlay); 
	SDL_Quit();
no_sdl:
	return 1;
}

void resize_sdl (unsigned int x, unsigned int y) { 
	sdl_screen = SDL_SetVideoMode(x, y, 0, SDL_RESIZABLE | SDL_HWSURFACE);
	sdl_rect.w=x;
	sdl_rect.h=y;
}

void getsize_sdl (unsigned int *x, unsigned int *y) {
	if(x) *x = sdl_rect.w;
	if(y) *y = sdl_rect.h;
}

void position_sdl(int x, int y) {
	SDL_SysWMinfo info;

	SDL_VERSION(&info.version);
	if ( SDL_GetWMInfo(&info) > 0 ) {
	if ( info.subsystem == SDL_SYSWM_X11 ) {
			info.info.x11.lock_func();
	/* get root window size  - center window 
			int x, y;
			int w, h;
			w = DisplayWidth(info.info.x11.display,
			DefaultScreen(info.info.x11.display));

			h = DisplayHeight(info.info.x11.display,
			DefaultScreen(info.info.x11.display));
			x = (w - screen->w)/2;
			y = (h - screen->h)/2;
	*/
			XMoveWindow(info.info.x11.display, info.info.x11.wmwindow, x, y);
			info.info.x11.unlock_func();
		} 
	} 
} 



void render_sdl (uint8_t *mybuffer) {
	/* http://www.fourcc.org/indexyuv.htm */

	size_t Ylen= movie_width * movie_height;
	size_t UVlen= movie_width/2 * movie_height/2; 

	// decode ffmpeg - YUV 
	uint8_t *Yptr=mybuffer;
	uint8_t *Uptr=Yptr + Ylen;
	uint8_t *Vptr=Uptr + UVlen;

	if (sdl_pic_format == SDL_YV12_OVERLAY) { 
	// encode SDL YV12
		stride_memcpy(sdl_overlay->pixels[0],Yptr,movie_width,movie_height,sdl_overlay->pitches[0],movie_width);//Y
		stride_memcpy(sdl_overlay->pixels[1],Vptr,movie_width/2,movie_height/2,sdl_overlay->pitches[1],movie_width/2);//V
		stride_memcpy(sdl_overlay->pixels[2],Uptr,movie_width/2,movie_height/2,sdl_overlay->pitches[2],movie_width/2);//U
	} else {
	// encode SDL YUV
		stride_memcpy(sdl_overlay->pixels[0],Yptr,movie_width,movie_height,sdl_overlay->pitches[0],movie_width);//Y
		stride_memcpy(sdl_overlay->pixels[1],Uptr,movie_width/2,movie_height/2,sdl_overlay->pitches[1],movie_width/2);//U
		stride_memcpy(sdl_overlay->pixels[2],Vptr,movie_width/2,movie_height/2,sdl_overlay->pitches[2],movie_width/2);//V
	}

	SDL_UnlockYUVOverlay(sdl_overlay);
	SDL_DisplayYUVOverlay(sdl_overlay, &sdl_rect);
	SDL_LockYUVOverlay(sdl_overlay);

#if 0  // SDL - OSD test
	//	displaybox(&white, 10);
		displaytext("xjadeo!",&white,40);
#endif


}


void newsrc_sdl (void) {
	if(sdl_overlay) SDL_FreeYUVOverlay(sdl_overlay); 
	sdl_overlay = SDL_CreateYUVOverlay(movie_width, movie_height, SDL_YV12_OVERLAY, sdl_screen);
}

void handle_X_events_sdl (void) {
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		switch(ev.type){
			case SDL_VIDEOEXPOSE: // SDL render event
				render_sdl(buffer);
			//	printf("SDL render event.\n");
				break;
			case SDL_QUIT:
			//	printf("SDL quit event.\n");
				loop_flag=0;
				break;
			case SDL_KEYDOWN:
			//	printf("SDL key down event.");
				if(ev.key.keysym.sym==SDLK_ESCAPE) {
					loop_flag=0;
				}
				break;
			case SDL_VIDEORESIZE:
			        sdl_screen = SDL_SetVideoMode(ev.resize.w, ev.resize.h, 0, SDL_RESIZABLE | SDL_HWSURFACE);
				sdl_rect.w=ev.resize.w;
				sdl_rect.h=ev.resize.h;
				break;
			case SDL_MOUSEBUTTONUP:
				if(ev.button.button == SDL_BUTTON_LEFT) {
						resize_sdl(movie_width,movie_height);
				} else 
#if 0 // fix aspect only on right-button and scroll. 
					 if(ev.button.button == SDL_BUTTON_WHEELUP ||
						ev.button.button == SDL_BUTTON_WHEELDOWN ||
						ev.button.button == SDL_BUTTON_RIGHT)
#endif
					 { 

					unsigned int my_Width,my_Height;
					getsize_sdl(&my_Width,&my_Height);

					if(ev.button.button == SDL_BUTTON_WHEELUP) {
						float step=sqrt((float)my_Height);
						my_Width-=floor(step*((float)movie_width/(float)movie_height));
						my_Height-=step;
					}
					if(ev.button.button == SDL_BUTTON_WHEELDOWN) {
						float step=sqrt((float)my_Height);
						my_Width+=floor(step*((float)movie_width/(float)movie_height));
						my_Height+=step;
					} 
					// resize to match movie aspect ratio
					if( ((float)movie_width/(float)movie_height) < ((float)my_Width/(float)my_Height) )
						my_Width=floor((float)my_Height * (float)movie_width / (float)movie_height);
					else my_Height=floor((float)my_Width * (float)movie_height / (float)movie_width);

					resize_sdl(my_Width,my_Height);
				}
				break;
			default: /* unhandled event */
				;
		}
	}
	// TODO: SDL_Event SDL_Quit_Event -> loop_flag = 0;
}

#endif /* HAVE_SDL */
