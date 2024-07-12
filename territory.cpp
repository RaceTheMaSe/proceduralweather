//Territory Main File

//Using SDL and standard IO
#include "view.h"
#include <stdio.h>
#include <array>
#include <iomanip>

/*
Add Player Sprite
Add Game Mode Switch Button
Add Smarter Local Tiling, No edge, more efficiency
Add Rotate Map Option?
Fix Coordinate System
Improve Local Terrain Generator
	Fix Water Level
Add Randomized Floor Sprites for more interesting Floor
Add Basic Grass and Shrub Foliage
Attempt to add Trees?

- Save Global Depth Map to File
*/

//Function Definitions
void drawWorldMap(World* terrain, SDL_Renderer* gRenderer, Player* player, size_t gridSize);
void drawWorldOverlay(World* terrain, SDL_Renderer* gRenderer, int a, size_t gridSize);

bool loadMedia();

std::array<std::string,10> modeStrings {
		"Windmap",
		"Cloudmap",
		"Rainmap",
		"Humditymap",
		"Tempmap",
		"Average Windmap",
		"Average Cloudmap",
		"Average Rainmap",
		"Average Humiditymap",
		"Average Tempmap"
};

const size_t gridSizeDefault = 100;
int cellSize = SCREEN_WIDTH / gridSizeDefault;
int localGrid = 50;
int seedDefault = 15;
int seed = seedDefault;

int main( int argc, char** args ) {
	//The window we'll be rendering to
	SDL_Window* gWindow = NULL;
	SDL_Renderer* gRenderer = NULL;

	TTF_Init();

	size_t gridSize = gridSizeDefault;
	if(argc>1)
		gridSize = (size_t)atoi(args[1]);
	if(argc>2)
		localGrid = (size_t)atoi(args[2]);
	if(argc>3)
		seed = (size_t)atoi(args[3]);
	gridSize=std::min(std::max(50ul,gridSize),1000ul);
	localGrid=std::min(std::max(10ul,gridSize),100ul);
	cellSize = SCREEN_WIDTH / gridSize;

	//Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
	}
	else
	{
		//Create window
		gWindow = SDL_CreateWindow( "Territory", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
		if( gWindow == NULL )
		{
			printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
		}
		else
		{
			//Prepare the Renderer
		  gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);

			//Tiling Logic
			View view(gridSize);
			if(!view.loadTilemap(*&gRenderer)){
				std::cout<<"Couldn't load file."<<std::endl;
			}

			//World Generation
			World* territory = new World(gridSize, seed);
			Player* player = new Player();

			territory->generate();
			//Clear the Screen
			SDL_SetRenderDrawBlendMode(gRenderer,SDL_BLENDMODE_BLEND);

			//Game Loop
			bool quit = false;
			SDL_Event e;
			int overlayMode = 0;
			int delayMS = 100;

			while(!quit){
				//Check for Quit
				while( SDL_PollEvent( &e ) != 0 ) {
					//User requests quit
					if( e.type == SDL_QUIT ) { quit = true; }
					else if( e.type == SDL_KEYDOWN ) {
						if(e.key.keysym.sym == SDLK_ESCAPE ){
							quit = true;
						}
						else if (e.key.keysym.sym == SDLK_SPACE){
							view.switchView();
						}
						else if (e.key.keysym.sym == SDLK_UP){
							delayMS -= 10;
							delayMS = std::min(std::max(delayMS,10),1000);
							std::cout << "Speed " << std::fixed << std::setprecision(2) << 1000.0f/(float)delayMS << std::endl;
						}
						else if (e.key.keysym.sym == SDLK_DOWN){
							delayMS += 10;
							delayMS = std::min(std::max(delayMS,10),1000);
							std::cout << "Speed " << std::fixed << std::setprecision(2) << 1000.0f/(float)delayMS << std::endl;
						}
						else if (e.key.keysym.sym == SDLK_RIGHT){
							delayMS = 100;
							std::cout << "Speed " << std::fixed << std::setprecision(2) << 1000.0f/(float)delayMS  << " (default)" << std::endl;
						}
						else if (e.key.keysym.sym == SDLK_r){
							view.rotateView();
						}
						else if (e.key.keysym.sym >= SDLK_0 && e.key.keysym.sym <= SDLK_9){
							overlayMode = e.key.keysym.sym-SDLK_0;
							std::cout << "Overlay " << overlayMode << " " << modeStrings[overlayMode] << std::endl;
						}
						if(view.viewMode == 1){
							territory->changePos(e);
						}
						else if(view.viewMode == 2){
							player->changePos(e);
						}
					}
				}

				SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 0);
				SDL_RenderClear(gRenderer);
				if(view.viewMode == 0){
					territory->day+=1;
					territory->climate.calcWind(territory->day, territory->seed, &territory->terrain);
					territory->climate.calcTempMap(&territory->terrain);
					territory->climate.calcHumidityMap(&territory->terrain);
					territory->climate.calcDownfallMap();

					//I don't know why this works
					drawWorldMap(territory, gRenderer, player, gridSize);
					drawWorldOverlay(territory, gRenderer,overlayMode, gridSize);
					if(overlayMode==1) // wind and clouds drawn together
						drawWorldOverlay(territory, gRenderer,overlayMode+1, gridSize);

					//Wait for day development
					//view.calcFPS();
					SDL_Delay(delayMS);
				}

				else if(view.viewMode == 1){
					view.renderMap(territory, gRenderer, territory->xview, territory->yview);
					//view.calcFPS();
				}

				else if(view.viewMode == 2){
					view.renderLocal(territory, gRenderer, player);
					//view.calcFPS();
					SDL_Delay(10);
				}
				//Draw Everything
				SDL_RenderPresent(gRenderer);
			}

			delete player;
			delete territory;
		}
	}
	//Destroy window
	SDL_DestroyWindow( gWindow );
	SDL_DestroyRenderer (gRenderer);

	//Quit SDL subsystems
  TTF_Quit();
	SDL_Quit();

	return 0;
}

void drawWorldMap(World* territory, SDL_Renderer* gRenderer, Player* player, size_t gridSize){
	//Draw the Map with Overlays
	for (size_t i=0; i<gridSize; i++){
		for (size_t j=0; j<gridSize; j++){
			int a = territory->terrain.biomeMap[i*gridSize+j];
			SDL_Rect rect;
			rect.x=i*cellSize;
			rect.y=j*cellSize;
			rect.w=cellSize;
			rect.h=cellSize;
			switch(a){
				//Water
				case 0: SDL_SetRenderDrawColor(gRenderer, 0x2d, 0x56, 0x85, 255);
				break;
				//Sandy Beach
				case 1: SDL_SetRenderDrawColor(gRenderer, 0xea, 0xdf, 0x9e, 255);
				break;
				//Gravel Beach
				case 2: SDL_SetRenderDrawColor(gRenderer, 0xcc, 0xcc, 0xcc, 255);
				break;
				//Stoney Beach Cliff
				case 3: SDL_SetRenderDrawColor(gRenderer, 0xa7, 0xa5, 0x9b, 255);
				break;
				//Wet Plains (Grassland)
				case 4: SDL_SetRenderDrawColor(gRenderer, 0x9e, 0xc1, 0x6d, 255);
				break;
				//Dry Plains (Shrubland)
				case 5: SDL_SetRenderDrawColor(gRenderer, 0xbc, 0xc1, 0x6d, 255);
				break;
				//Rocky Hills
				case 6: SDL_SetRenderDrawColor(gRenderer, 0xaa, 0xaa, 0xaa, 255);
				break;
				//Temperate Forest
				case 7: SDL_SetRenderDrawColor(gRenderer, 0x3d, 0xab, 0x50, 255);
				break;
				//Boreal Forest
				case 8: SDL_SetRenderDrawColor(gRenderer, 0x30, 0x7a, 0x3c, 255);
				break;
				//Mountain Tundra
				case 9: SDL_SetRenderDrawColor(gRenderer, 0x77, 0x77, 0x77, 255);
				break;
				//Mountain Peak
				case 10: SDL_SetRenderDrawColor(gRenderer, 0xee, 0xee, 0xee, 255);
			}
			SDL_RenderFillRect(gRenderer, &rect);
		}
	}
	SDL_SetRenderDrawColor(gRenderer, 0xee, 0x11, 0x11, 255);
	SDL_Rect rect;
	rect.x=player->xGlobal*cellSize;
	rect.y=player->yGlobal*cellSize;
	rect.w=cellSize;
	rect.h=cellSize;
	SDL_RenderFillRect(gRenderer, &rect);
}

void drawWorldOverlay(World* territory, SDL_Renderer* gRenderer, int a, size_t gridSize){
	//Draw the Climate Map
	for(size_t i = 0; i<gridSize; i++){
		for(size_t j = 0; j<gridSize; j++){
			const size_t cell = i*gridSize+j;
			
			//std::cout<<territory.climate.AvgRainMap[i][j]<<std::endl;
			//Drawing Rectangle
			SDL_Rect rect;
			rect.x=i*cellSize;
			rect.y=j*cellSize;
			rect.w=cellSize;
			rect.h=cellSize;
			//Render the Overlays
			switch(a){
				//Wind Map
				case 0: SDL_SetRenderDrawColor(gRenderer, territory->climate.WindMap[cell]*25, territory->climate.WindMap[cell]*25, territory->climate.WindMap[cell]*25, 100);
					break;
				//Cloud Map
				case 1: SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 100*territory->climate.CloudMap[cell]);
					break;
				//Rain Map
				case 2: SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255*territory->climate.RainMap[cell]);
					break;
				//Temperature Map
				case 3: SDL_SetRenderDrawColor(gRenderer, territory->climate.TempMap[cell]*255, 150, 150, 100);
					break;
				//Humidity Map
				case 4: SDL_SetRenderDrawColor(gRenderer, 50, 50, territory->climate.HumidityMap[cell]*255, 220);
					break;
				//Average Wind Map
				case 5: SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, ((5-territory->climate.AvgWindMap[cell])+2)*60);
					break;
				//Average Cloud Map
				case 6: SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255*territory->climate.AvgCloudMap[cell]);
					break;
				//Average Rain Map
				case 7: SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255*10*territory->climate.AvgRainMap[cell]);
					break;
				//Average Temperature Map
				case 8: SDL_SetRenderDrawColor(gRenderer, territory->climate.AvgTempMap[cell]*255, 150, 150, 100);
					break;
				//Average Humidity Map
				case 9: SDL_SetRenderDrawColor(gRenderer, 50, 50, territory->climate.AvgHumidityMap[cell]*255, 220);
					break;

			}
			SDL_RenderFillRect(gRenderer, &rect);
		}
	}
}
