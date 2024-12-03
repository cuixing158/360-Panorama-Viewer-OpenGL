#include <stdio.h>
#include <string>
#include <iostream>
#include <cmath>
#include "glrenderer.h"

//Window initial dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 360;

bool is_fullscreen = false;
//Starts up SDL and creates window
bool init(bool is_fullscreen);

//For debugging purposes
void PrintEvent(const SDL_Event * event);

//Loads media
void loadMedia();

//Frees media and shuts down SDL
void close();

//Loads individual image as texture
SDL_Texture* loadTexture( std::string path );

//The window we'll be rendering to
SDL_Window* gWindow = NULL;


int screen_counter = 0;
int numdisplays = 0;

//current window size
int sizeX = 0;
int sizeY = 0;

bool is_spherical_grid = false;

void create_thread();
static int MyThread(void *ptr);
bool is_thread_running = true;
SDL_Thread *thread;
SDL_mutex *mutex;

//lock sdl mutex
bool lock_mutex(){
	if (SDL_LockMutex(mutex) == 0) {
		return true;
	} else {
		fprintf(stderr, "Couldn't lock mutex\n");
		return false;
	}
};

//unlock sdl mutex
bool unlock_mutex(){
	SDL_UnlockMutex(mutex);
	return true;///fix this latex to check if it succeeds
};

const double default_autorotate_step = 0.05;
//flags used to communicate from input event thread to the rendering thread
bool is_requested_reread_winsize = false;
bool is_fullscreenchange_requested = false;
bool is_displaychange_requested = false;
bool is_request_autorotate = true;
double autorotate_step=default_autorotate_step;
void request_reread_win_size();
void request_fulscreen_change();
void request_display_change();
void request_autorotate(double step);


//invalidate logics - invalidate = request for a redraw in a separate thread
bool is_screen_valid = true;
void invalidate_screen(){
	if(is_screen_valid){
		lock_mutex();
		is_screen_valid = false;
		unlock_mutex();
	}
}

//polar angles of where the center of the scene looks initially at the sphere
double default_center_teta = 90;//90 points to equator initially
double default_center_fi = 0;
int default_NviewX = 8;
int default_NviewY = 1;
bool is_request_update_polar_angles = false;
void update_polar_angles(double dteta, double dfi){
	lock_mutex();
	default_center_fi += dfi;
	default_center_teta += dteta;
	if(default_center_teta<0)//block at poles
		default_center_teta = 0;
	if(default_center_teta>180)
		default_center_teta = 180;
	if(default_center_fi>360)
		default_center_fi -= 360;
	if(default_center_fi<-360)
		default_center_fi += 360;
	is_request_update_polar_angles = true;
	unlock_mutex();
}

SDL_Surface* IMGSurf;

bool init(bool is_fullscreen)
{
	//gl sdl stuff
	printf("Started to initialize opengl context \n");
	glrenderer::gContext = SDL_GL_CreateContext( gWindow );
	if( glrenderer::gContext == NULL )
	{
		printf( "OpenGL context could not be created! SDL Error: %s\n", SDL_GetError() );
	}
	else
	{
		SDL_GetWindowSize(gWindow, &sizeX, &sizeY);
		printf("Got window size x: %d, y: %d \n", sizeX, sizeY);
		glrenderer::myworld.center_fi = default_center_fi;
		glrenderer::myworld.center_teta = default_center_teta;
		glrenderer::myworld.NviewX = default_NviewX;
		glrenderer::myworld.NviewY = default_NviewY;
		if( !glrenderer::initGL(sizeX, sizeY) )
		{
			printf( "Unable to initialize OpenGL!\n" );
		}
		//render gl
		glrenderer::renderGL(is_spherical_grid, sizeX, sizeY);
	}
	numdisplays = SDL_GetNumVideoDisplays();
	printf("Numdisplays %d \n", numdisplays);
	if(numdisplays <= 0)
	{
		printf("Error in the number of the displays \n");
		exit(0);
	}
	return true;
}

void close()
{
	//Destroy window	
	SDL_DestroyWindow( gWindow );
	gWindow = NULL;
	//Quit SDL subsystems
	SDL_Quit();
}

int draw_frame(){
	lock_mutex();
	SDL_GetWindowSize(gWindow, &sizeX, &sizeY);
	glrenderer::set_viewport_size(sizeX, sizeY);
	glrenderer::renderGL(is_spherical_grid, sizeX, sizeY);
	SDL_GL_SwapWindow( gWindow );
	unlock_mutex();
	return 0;
}

SDL_Event event;

int main( int argc, char* args[] )
{
	//Initialization flag
	bool success = true;
	//Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
		success = false;
	}
	//Create window
	gWindow = SDL_CreateWindow( "GL_RENDERER", SDL_WINDOWPOS_CENTERED_DISPLAY(screen_counter),
	SDL_WINDOWPOS_CENTERED_DISPLAY(screen_counter),
	SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE );
	if( gWindow == NULL )
	{
		printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
		success = false;
	}
	create_thread();
	lock_mutex();
	unlock_mutex();
	//Main loop flag
	bool quit = false;

	//Event handler
	SDL_Event e;
	//While application is running
	while( !quit )
	{
		//Handle events on queue
		while( SDL_PollEvent( &e ) != 0 )
		{
			PrintEvent(&e);
			switch (e.type)
			{
				case SDL_WINDOWEVENT:
					printf("Window event \n");
					invalidate_screen();
					break;
				case SDL_MOUSEMOTION:
					break;
				case SDL_QUIT:
					quit = true;
				case SDL_KEYDOWN:
					if(e.key.keysym.sym == SDLK_ESCAPE)
						quit = true; //quit
					if(e.key.keysym.sym == SDLK_a)
					{
						request_autorotate(default_autorotate_step);
					}
					if(e.key.keysym.sym == SDLK_f)
					{
						is_fullscreen = !is_fullscreen;
						SDL_SetWindowFullscreen(gWindow, is_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
						SDL_GetWindowSize(gWindow, &sizeX, &sizeY);
						printf("Got window size x: %d, y: %d \n", sizeX, sizeY);
						request_fulscreen_change();
					}
					if(e.key.keysym.sym == SDLK_d)
					{
						request_display_change();//dummy for now - does not currently do any job
					}
					if(e.key.keysym.sym == SDLK_s)
					{
						printf("Grid toggled \n");
						is_spherical_grid = !is_spherical_grid;
						invalidate_screen();
					}
					if(e.key.keysym.sym == SDLK_RIGHT)
					{
						update_polar_angles(0.0, -1.0);
						invalidate_screen();
					}
					if(e.key.keysym.sym == SDLK_LEFT)
					{
						update_polar_angles(0.0, 1.0);
						invalidate_screen();
					}
					if(e.key.keysym.sym == SDLK_UP)
					{
						update_polar_angles(-1.0, 0.0);
						invalidate_screen();
					}
					if(e.key.keysym.sym == SDLK_DOWN)
					{
						update_polar_angles(1.0, 0.0);
						invalidate_screen();
					}
					break;
			}
		}
	}
	is_thread_running = false;
	int threadReturnValue;
	//wait for thread to finish - join thread
	SDL_WaitThread(thread, &threadReturnValue);
	printf("Thread returned value: %d \n", threadReturnValue);
	SDL_DestroyMutex(mutex);
	//Free resources and close SDL
	close();
	return 0;
}

void create_thread(){
	mutex = SDL_CreateMutex();
	if (!mutex) {
		printf( "Couldn't create mutex\n");
		return;
	}
	//create a thread, pass it a thread pointer function
	thread = SDL_CreateThread(MyThread, "MyThread", (void *)NULL);
	if (NULL == thread) {
		printf("\nSDL_CreateThread failed: %s\n", SDL_GetError());
	}
}

int MyThread(void *ptr)
{
	int cnt = 0;
	lock_mutex();
	loadMedia();
	if(!init(false))
	{
		printf( "Failed to initialize!\n" );
	}
	unlock_mutex();
	draw_frame();
	while(is_thread_running) {
		lock_mutex();
		if(is_requested_reread_winsize)
		{
			SDL_GetWindowSize(gWindow, &sizeX, &sizeY);
			if( !glrenderer::initGL(sizeX, sizeY) )
			{
				printf( "Unable to initialize OpenGL!\n" );
			}

			draw_frame();
		}
		if(is_fullscreenchange_requested)
		{
			draw_frame();
			is_fullscreenchange_requested = false;
		}
		unlock_mutex();

		lock_mutex();
		if(is_displaychange_requested)
		{
			numdisplays = SDL_GetNumVideoDisplays();
			screen_counter++;
			if (screen_counter >= numdisplays)
				screen_counter = 0;
			is_displaychange_requested = false;
		}
		unlock_mutex();

		lock_mutex();
		if(is_request_update_polar_angles)
		{
			glrenderer::myworld.center_fi = default_center_fi;
			glrenderer::myworld.center_teta = default_center_teta;
			printf("teta : %f , fi : %f \n",glrenderer::myworld.center_teta, glrenderer::myworld.center_fi);
			draw_frame();
			is_request_update_polar_angles = false;
		}
		unlock_mutex();

		lock_mutex();
		if(is_request_autorotate){
			default_center_fi+=autorotate_step;
			glrenderer::myworld.center_fi = default_center_fi;
			is_screen_valid = false;
		}
		unlock_mutex();
		//printf("\nThread counter: %d", cnt);
		SDL_Delay(1);
		if (!is_screen_valid){
			draw_frame();
			is_screen_valid = true;
		}
		cnt++;
	}
	return cnt;
}


void request_reread_win_size(){
	lock_mutex();
	is_requested_reread_winsize = true;
	unlock_mutex();
}

void request_fulscreen_change(){
	lock_mutex();
	is_fullscreenchange_requested = true;
	unlock_mutex();
}


void request_display_change(){
	lock_mutex();
	is_displaychange_requested = true;
	unlock_mutex();
}

void request_autorotate(double step){
	lock_mutex();
	autorotate_step = step;
	is_request_autorotate = !is_request_autorotate;
	unlock_mutex();
}

void loadMedia(){
    glrenderer::surf = IMG_Load("imgbig.jpg");
    printf("Img loaded , w : %d, h : %d \n",glrenderer::surf->w,glrenderer::surf->h);
    glrenderer::setup_textures();
}

void PrintEvent(const SDL_Event * event)
{
	if (event->type == SDL_WINDOWEVENT) {
		switch (event->window.event) {
		case SDL_WINDOWEVENT_SHOWN:
			printf("Window %d shown \n", event->window.windowID);
			 break;
		case SDL_WINDOWEVENT_HIDDEN:
			printf("Window %d hidden \n", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_EXPOSED:
			printf("Window %d exposed \n", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_MOVED:
			printf("Window %d moved to %d,%d \n",
				event->window.windowID, event->window.data1,
				event->window.data2);
			break;
		case SDL_WINDOWEVENT_RESIZED:
			printf("Window %d resized to %dx%d \n",
				event->window.windowID, event->window.data1,
				event->window.data2);
			break;
		case SDL_WINDOWEVENT_MINIMIZED:
			printf("Window %d minimized \n", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_MAXIMIZED:
			printf("Window %d maximized \n", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_RESTORED:
			printf("Window %d restored \n", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_ENTER:
			printf("Mouse entered window %d \n",
				event->window.windowID);
			break;
		case SDL_WINDOWEVENT_LEAVE:
			printf("Mouse left window %d \n", event->window.windowID);
			break;
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			printf("Window %d gained keyboard focus \n",
				event->window.windowID);
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			printf("Window %d lost keyboard focus \n",
				event->window.windowID);
			break;
		case SDL_WINDOWEVENT_CLOSE:
			printf("Window %d closed \n", event->window.windowID);
			break;
		default:
			printf("Window %d got unknown event %d \n",
				event->window.windowID, event->window.event);
			break;
		}
	}
}
