//Tilemapping Class
#include <iostream>
#include <stdlib.h>
#include "worldgen.h"
#include "game.h"
#include <time.h>

//Texture wrapper class
class View {
 public:
   View(size_t gridSize);
   size_t gridSize = gridSizeDefault;
   //For FPS Counter
   int ticks= 0;
   int FPS = 0;

   //Game Viewmode
   int viewMode = 0;
   int viewRotation = 0;
   const int renderDistance = 15;

   //Drawing Functions
	 bool loadTilemap(SDL_Renderer* gRenderer);
   void writeText(SDL_Renderer* gRenderer);
   void calcFPS();

   //Overlay Rendering
   void renderMap(const World* territory, SDL_Renderer* gRenderer, int xview, int yview);
   void renderLocal(World* territory, SDL_Renderer* gRenderer, const Player* player);
   void renderVegetation(const World* territory, SDL_Renderer* gRenderer, const Player* player, int i, int j, int tileScale);
   void renderPlayer(const World* territory, SDL_Renderer* gRenderer, const Player* player);

   //View altering Functions
   void switchView();
   void rotateView();

 private:
   SDL_Texture* mTexture = NULL;
   SDL_Texture* treeTexture = NULL;
   TTF_Font * gFont = NULL;
};

View::View(size_t gridSizeIn) : gridSize(gridSizeIn) {}

void View::switchView(){
  viewMode = (viewMode+1)%3;
}

void View::rotateView(){
  viewRotation = (viewRotation+1)%2;
}

void View::calcFPS(){
  FPS = 1000/(SDL_GetTicks()-ticks);
  ticks = SDL_GetTicks();
  std::cout<< "FPS: "<<FPS<<std::endl;
}

void View::writeText(SDL_Renderer* gRenderer){
  //Load the Font in the desired Size
  gFont = TTF_OpenFont("font.ttf", 50);
  //Set textcolor
  SDL_Color color = { 255, 255, 255 , 255 };
  //Render the Text
  SDL_Surface * surface = TTF_RenderText_Solid(gFont, "100", color);
  SDL_Texture * texture = SDL_CreateTextureFromSurface(gRenderer, surface);
  SDL_Rect rect;
    //Replace this with logic based on territory->terrain.surfaceMap[i][j];
    rect.x=0;
    rect.y=0;
    SDL_QueryTexture(texture,NULL,NULL,&rect.w,&rect.h);
  SDL_RenderCopy(gRenderer, texture, NULL, &rect);
}

bool View::loadTilemap(SDL_Renderer* gRenderer){
  mTexture = IMG_LoadTexture( gRenderer, "tiles.png" );
  treeTexture = IMG_LoadTexture( gRenderer, "trunk2.png" );
  if(mTexture != NULL){
    return 1;
  }
  else return 0;
}

void View::renderMap(const World* territory, SDL_Renderer* gRenderer, int /*xview*/, int /*yview*/) {
	//Set rendering space and render to screen
  //Isometric Tiling Logic Based on Height and Surface Map
  int tileScale = 5;
  for(size_t i=0; i<gridSize; i++){
    for(size_t j=0; j<gridSize; j++){
      //For the Depth
      for(int k =0; k<40; k++){
        //Take Sourcequad from Territory Surface Tile
        SDL_Rect sourceQuad;
          //Replace this with logic based on territory->terrain.surfaceMap[i][j];
          sourceQuad.x=0;
          sourceQuad.y=territory->terrain.biomeMap[i*gridSize+j]*11;
          sourceQuad.w=11;
          sourceQuad.h=11;
        //Take Renderquad from current i and j numbers
        //Height is in Intervals of 100
        if((int)territory->terrain.depthMap[i*gridSize+j]/100>=k){
          SDL_Rect renderQuad;
            renderQuad.x=((territory->terrain.worldWidth-tileScale*10)/2)+j*tileScale*5-i*tileScale*5-territory->xview;
            renderQuad.y=j*3*tileScale+i*3*tileScale-territory->yview-k*5*tileScale;
            renderQuad.w=tileScale*11;
            renderQuad.h=tileScale*11;
          //Render
          if(renderQuad.x > -10 && renderQuad.x < SCREEN_WIDTH && renderQuad.y < SCREEN_HEIGHT && renderQuad.y > -10){
            SDL_RenderCopy( gRenderer, mTexture, &sourceQuad, &renderQuad);
          }
        }
      }
    }
  }
  // black outside map
  SDL_Rect rect;
  rect.x=1000;
  rect.y=0;
  rect.w=400;
  rect.h=1000;
  SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 100);
  SDL_RenderFillRect(gRenderer, &rect);
}

void View::renderPlayer(const World* territory, SDL_Renderer* gRenderer, const Player* /*player*/){
  treeTexture = IMG_LoadTexture( gRenderer, "trunk.png" );
  SDL_Rect sourceQuad;
  int tileScale = 5;
    sourceQuad.x=0;
    sourceQuad.y=0;
    sourceQuad.w=11;
    sourceQuad.h=22;

    SDL_Rect renderQuad;
    int i = localGrid/2-1;
    int j = localGrid/2-1;
    const size_t localCell = i*localGrid+j;
    renderQuad.w=tileScale*11;
    renderQuad.h=tileScale*22;
    renderQuad.x=territory->terrain.worldWidth/2+tileScale*5*(-1);
    renderQuad.y=territory->terrain.worldHeight/2-tileScale*5-tileScale*2*11-((int)territory->terrain.localMap[localCell]-(int)territory->terrain.localMap[localCell])*5*tileScale;
    SDL_RenderCopy( gRenderer, treeTexture, &sourceQuad, &renderQuad);
}

void View::renderVegetation(const World* territory, SDL_Renderer* gRenderer, const Player* player, int i, int j, int tileScale){
  //If there is a tree present at given location
  if(territory->vegetation.getTree(territory, player, i, j)){
    SDL_Rect sourceQuad;
      sourceQuad.x=0;
      sourceQuad.y=0;
      sourceQuad.w=11;
      sourceQuad.h=22;

      int hs = localGrid/2;
      int i = hs-1;
      int j = hs-1;
      const size_t localCell = i*localGrid+j;

      SDL_Rect renderQuad;
      renderQuad.w=tileScale*11;
      renderQuad.h=tileScale*22;
      renderQuad.x=territory->terrain.worldWidth/2+tileScale*5*(-1+j-i);
      renderQuad.y=territory->terrain.worldHeight/2-tileScale*5-tileScale*17+3*tileScale*((j-hs)+(i-hs))-((int)territory->terrain.localMap[localCell]-(int)territory->terrain.localMap[localCell])*5*tileScale;
      SDL_RenderCopy( gRenderer, treeTexture, &sourceQuad, &renderQuad);
  }
}

void View::renderLocal(World* territory, SDL_Renderer* gRenderer, const Player* player){
  //Set rendering space and render to screen
  //Generate the Local Area
  //Isometric Tiling Logic Based on Height and Surface Map
  int tileScale = 6;
  territory->terrain.genLocal(territory->seed, player);

  int hs = localGrid/2;
  int lc = hs-1;
  int uc = hs-1;

  for(int i=0; i<localGrid; i++){
    for(int j=0; j<localGrid; j++){
        const size_t localCell = i*localGrid+j;

        //Take Sourcequad from Territory Surface Tile
        SDL_Rect sourceQuad;
          //Replace this with logic based on territory->terrain.surfaceMap[localCell];
          sourceQuad.x=(int)(territory->terrain.localMap[localCell]*4)%3*11;
          sourceQuad.y=territory->terrain.biomeMap[((player->xTotal+i-hs)/territory->terrain.worldWidth)*gridSize+((player->yTotal+j-hs)/territory->terrain.worldHeight)]*11;
          sourceQuad.w=11;
          sourceQuad.h=11;
        //Take Renderquad from current i and j numbers
        //Height is in Intervals of 100
          SDL_Rect renderQuad;
          renderQuad.w=tileScale*11;
          renderQuad.h=tileScale*11;
          renderQuad.x=territory->terrain.worldWidth/2+tileScale*5*((j-hs)-(i-hs)-1);
          renderQuad.y=territory->terrain.worldHeight/2-tileScale*5+3*tileScale*((j-hs)+(i-hs))-((int)territory->terrain.localMap[localCell]-(int)territory->terrain.localMap[lc*uc])*5*tileScale;
          //Render
          //Render the Vegetation on the Map
          SDL_RenderCopy( gRenderer, mTexture, &sourceQuad, &renderQuad);
          renderVegetation(territory, gRenderer, player, i, j, tileScale);
    }
  }
}
