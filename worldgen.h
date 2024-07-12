//Worldgen Functions and Classes
#include <libnoise/noise.h>
#include <iostream>
#include <stdlib.h>
#include "player.h"
#include <SDL2/SDL.h>
#include <time.h>

using namespace noise;

//Screen dimension constants - square
const int SCREEN_WIDTH = 1000;
const int SCREEN_HEIGHT = SCREEN_WIDTH;
extern int localGrid;
extern int cellSize;
extern const size_t gridSizeDefault;
extern int seedDefault;

class Climate;
class Terrain;
class World;
class Vegetation;

class Vegetation{
  public:
  //Calculates wether there is a tree or not
  bool getTree(const World* territory, const Player* player, int i, int j) const;
};

class Terrain{
  public:
  int worldDepth = 4000;
  int worldHeight = 1000;
  int worldWidth = 1000;
  size_t gridSize = gridSizeDefault;

  Terrain(size_t gridSize);
  ~Terrain();

  //Terrain Parameters
  float* depthMap = nullptr;
  void genDepth(int seed);

  int* biomeMap = nullptr;
  void genBiome(const Climate& climate);

  //Erodes the Landscape for a number of years
  void erode(int seed, const Terrain* terrain, int years);

  //Local Area (100 Tiles)
  float* localMap = nullptr;
  void genLocal(int seed, const Player* player);
};

class Climate {
  public:
  Climate(size_t gridSize);
  ~Climate();

  //Curent Climate Maps
  float* TempMap = nullptr;
  float* HumidityMap = nullptr;
  bool* CloudMap = nullptr;
  bool* RainMap = nullptr;
  float* WindMap = nullptr;
  double WindDirection[2] = {1,1}; //from 0-1

  //Average Climate Maps
  float* AvgRainMap = nullptr;
  float* AvgWindMap = nullptr;
  float* AvgCloudMap = nullptr;
  float* AvgTempMap = nullptr;
  float* AvgHumidityMap = nullptr;

  size_t gridSize = gridSizeDefault;

  void init(int day, int seed, const Terrain* terrain);
  void initTempMap(const Terrain* terrain);
  void initHumidityMap(const Terrain* terrain);
  void initCloudMap();
  void initRainMap();

  void calcWind(int day, int seed, const Terrain* terrain);
  void calcTempMap(const Terrain* terrain);
  void calcHumidityMap(const Terrain* terrain);
  void calcDownfallMap();
  void calcWindMap(int day, int seed, const Terrain* terrain);

  void calcAverage(int seed, const Terrain* terrain);
};

class World{
  public:
  World(size_t gridSize, int seed);
  int xview = localGrid;
  int yview = localGrid;
  int seed = seedDefault;
  int day = 0;
  size_t gridSize = gridSizeDefault;
  Climate climate;
  Terrain terrain;
  Vegetation vegetation;

  void generate();
  void changePos(SDL_Event e);
};

bool Vegetation::getTree(const World* territory, const Player* player, int i, int j) const {
  //Code to Calculate wether or not we have a tree
  /* Ideally this generates a vegetation map, spitting out
  0 for nothing,
  1 from short grass,
  2 for shrub,
  3 for some herb
  4 for some bush
  5 for some flower
  6 for some tree

  and also gives a number for a variant (3-5 variants of everything per biome)
  every variant could then also have a texture variant if wanted

  For now it only spits out wether or not we have a tree, which it then draws

  We can one piece of vegetation per map

  You could also do this for other objects on the map
  (tents, rocks, other locations) and not place vegetation if there is something present
  */

  //Perlin Noise Module
  module::Perlin perlin = {};

  perlin.SetOctaveCount(20);
  perlin.SetFrequency(1000);
  perlin.SetPersistence(0.8);

  //Generate the Height Map with Perlin Noise
  float x = (float)(player->xTotal-localGrid/2+i)/100000;
  float y = (float)(player->yTotal-localGrid/2+j)/100000;

  //This is not an efficient tree generation method
  //But a reasonable distribution for a grassland area
  srand(x+y);
  int tree = ((int)(1/(perlin.GetValue(x, y, territory->seed+1)+1))*rand()%5)/4;

  return tree;
}

void World::changePos(SDL_Event e){
  switch( e.key.keysym.sym ){
    case SDLK_UP: yview -=localGrid;
      break;
    case SDLK_DOWN: yview +=localGrid;
      break;
    case SDLK_LEFT: xview -= localGrid;
      break;
    case SDLK_RIGHT: xview += localGrid;
      break;
    }
}

World::World(size_t gridSize, int seedIn) : seed(seedIn), climate(gridSize), terrain(gridSize) { }

void World::generate(){
  //Geography
  //Generate and save a heightmap for all Blocks, all Regions
  terrain.genDepth(seed);

  //Erode the Landscape based on iterative average climate
  terrain.erode(seed, &terrain, 1);

  //Calculate the climate system of the eroded landscape
  climate.init(day, seed, &terrain);
  climate.calcAverage(seed, &terrain);

  //Generate the Surface Composition
  terrain.genBiome(climate);
}

void Terrain::genBiome(const Climate& climate){
  /*
  Determine the Surface Biome:
  0: Water
  1: Sandy Beach
  2: Gravel Beach
  3: Stone Beach Cliffs
  4: Wet Plains (Grassland)
  5: Dry Plains (Shrubland)
  6: Rocky Hills
  7: Tempererate Forest
  8: Boreal Forest
  9: Mountain Tundra
  10: Mountain Peak

  Compare the Parameters and decide what kind of ground we have.
  */
  for(size_t i = 0; i<gridSize; i++){
    for(size_t j = 0; j<gridSize; j++){
      const size_t cell = i*gridSize+j;
      //0: Water
      if(depthMap[cell]<=200){
        biomeMap[cell] = 0;
      }
      //1: Sandy Beach
      else if(depthMap[cell]<=204){
        biomeMap[cell] = 1;
      }
      //2: Gravel Beach
      else if(depthMap[cell]<210){
        biomeMap[cell] = 2;
      }
      //3: Stony Beach Cliffs
      else if(depthMap[cell]<=220){
        biomeMap[cell] = 3;
      }
      //4: Wet Plains (Grassland)
      //5: Dry Plains (Shrubland)
      else if(depthMap[cell]<=600){
        if(climate.AvgRainMap[cell]>=0.02){
          biomeMap[cell] = 4;
        }
        else{
          biomeMap[cell] = 5;
        }
      }
      //6: Rocky Hills
      //7: Temperate Forest
      //8: Boreal Forest
      else if(depthMap[cell]<=1300){
        if(depthMap[cell]<=1100){
          biomeMap[cell] = 7;
        }
        else {
          biomeMap[cell] = 8;
        }
        if(climate.AvgRainMap[cell]<0.001 && i+rand()%4-2 > 5 && i+rand()%4-2 < 95 && j+rand()%4-2 > 5 && j+rand()%4-2 < 95){
          biomeMap[cell] = 6;
        }
      }
      else if(depthMap[cell]<=1500){
        biomeMap[cell] = 9;
      }
      //Otherwise just Temperate Forest
      else{
        biomeMap[cell] = 10;
      }
    }
  }
}

void Terrain::erode(int seed, const Terrain* terrain, int years){
  //Climate Simulation
  Climate* average = new Climate(gridSize);

  //Simulate the Years
  for(int i = 0; i<years; i++){
    //Initiate the Climate
    average->init(0, seed, terrain);

    //Simulate 1 Year for Average Weather Conditions
    average->calcAverage(seed, terrain);

    //Add Erosion of the Climate after 1 Year
    float erosion = 0;
    for(size_t j = 0; j<gridSize; j++){
      for(size_t k=0; k<gridSize; k++){
        const size_t cell = j*gridSize+k;
        erosion = (average->AvgRainMap[cell] + 0.5*average->AvgWindMap[cell]);
        depthMap[cell] = depthMap[cell] - 5*(depthMap[cell]/2000) * (1-depthMap[cell]/2000)*erosion;
      }
    }
  }
  delete average;
}

Climate::Climate(size_t gridSizeIn) : gridSize(gridSizeIn) {
  const size_t gridSizeSq = gridSize*gridSize;
  TempMap     = new float[gridSizeSq];
  HumidityMap = new float[gridSizeSq];
  CloudMap    = new bool [gridSizeSq];
  RainMap     = new bool [gridSizeSq];
  WindMap     = new float[gridSizeSq];

  AvgRainMap     = new float[gridSizeSq];
  AvgWindMap     = new float[gridSizeSq];
  AvgCloudMap    = new float[gridSizeSq];
  AvgTempMap     = new float[gridSizeSq];
  AvgHumidityMap = new float[gridSizeSq];

  memset(TempMap       ,0, sizeof(float)*gridSizeSq);
  memset(HumidityMap   ,0, sizeof(float)*gridSizeSq);
  memset(WindMap       ,0, sizeof(float)*gridSizeSq);

  memset(AvgRainMap    ,0, sizeof(float)*gridSizeSq);
  memset(AvgWindMap    ,0, sizeof(float)*gridSizeSq);
  memset(AvgCloudMap   ,0, sizeof(float)*gridSizeSq);
  memset(AvgTempMap    ,0, sizeof(float)*gridSizeSq);
  memset(AvgHumidityMap,0, sizeof(float)*gridSizeSq);
}

Climate::~Climate(){
  delete TempMap;
  delete HumidityMap;
  delete CloudMap;
  delete RainMap;
  delete WindMap;

  delete AvgRainMap;
  delete AvgWindMap;
  delete AvgCloudMap;
  delete AvgTempMap;
  delete AvgHumidityMap;
}

void Climate::init(int day, int seed, const Terrain* terrain){
  calcWind(day, seed, terrain);
  initTempMap(terrain);
  initHumidityMap(terrain);
  initRainMap();
  initCloudMap();
}

void Climate::calcAverage(int seed, const Terrain* terrain){
  //Climate Simulation over n years
  int years = 1;
  int startDay = 0;

  //Initiate Simulation at a starting point
  Climate* simulation = new Climate(gridSize);
  simulation->init(startDay, seed, terrain);

  //Simulate every day for n years
  for(int i = 0; i<years*365; i++){
    //Calculate new Climate
    simulation->calcWind(i, seed, terrain);
    simulation->calcTempMap(terrain);
    simulation->calcHumidityMap(terrain);
    simulation->calcDownfallMap();

    //Average
    for(size_t j = 0; j<gridSize; j++){
      for(size_t k = 0; k<gridSize; k++){
        const size_t cell = j*gridSize+k;
        AvgWindMap[cell] = (AvgWindMap[cell]*i+simulation->WindMap[cell])/(i+1);
        AvgRainMap[cell] = (AvgRainMap[cell]*i+simulation->RainMap[cell])/(i+1);
        AvgCloudMap[cell] = (AvgCloudMap[cell]*i+simulation->CloudMap[cell])/(i+1);
        AvgTempMap[cell] = (AvgTempMap[cell]*i+simulation->TempMap[cell])/(i+1);
        AvgHumidityMap[cell] = (AvgHumidityMap[cell]*i+simulation->HumidityMap[cell])/(i+1);
      }
    }
  }
  delete simulation;
}

Terrain::Terrain(size_t gridSizeIn) : gridSize(gridSizeIn){
  const size_t gridSizeSq = gridSize*gridSize;
  depthMap = new float[gridSizeSq];
  biomeMap = new int[gridSizeSq];
  localMap = new float[localGrid*localGrid];
}

Terrain::~Terrain(){
  delete depthMap;
  delete biomeMap;
}

void Terrain::genDepth(int seed){
  //Perlin Noise Module

  //Global Depth Map is Fine, unaffected by rivers.
  module::Perlin perlin = {};

  perlin.SetOctaveCount(12);
  perlin.SetFrequency(2);
  perlin.SetPersistence(0.6);

  //Generate the Perlin Noise World Map
  for(size_t i = 0; i<gridSize; i++){
    for(size_t j = 0; j<gridSize; j++){
      const size_t cell = i*gridSize+j;

      //Generate the Height Map with Perlin Noise
      float x = (float)i / gridSize;
      float y = (float)j / gridSize;
      depthMap[cell] = (perlin.GetValue(x, y, seed))/5+0.25;

      //Multiply with the Height Factor
      depthMap[cell] *= worldDepth;
    }
  }
}

void Terrain::genLocal(int seed, const Player* player){
  //Perlin Noise Module
  module::Perlin perlin = {};

  perlin.SetOctaveCount(12);
  perlin.SetFrequency(2);
  perlin.SetPersistence(0.6);

  //Generate the Perlin Noise World Map
  for(int i = 0; i<localGrid; i++){
    for(int j = 0; j<localGrid; j++){
      //Generate the Height Map with Perlin Noise
      float x = float(player->xTotal-localGrid/2+i)/100000.0f;
      float y = float(player->yTotal-localGrid/2+j)/100000.0f;
      const size_t currLocalCell = i*localGrid+j;
      localMap[currLocalCell] = (perlin.GetValue(x, y, seed))/5+0.25;
      //Multiply with the Height Factor
      localMap[currLocalCell] = localMap[currLocalCell]*worldDepth;
    }
  }
}

void Climate::calcWind(int day, int seed, const Terrain* terrain){
  //Perlin Noise Module

  module::Perlin perlin = {};
  perlin.SetOctaveCount(2);
  perlin.SetFrequency(4);

  float timeInterval = (float)day/365;

  //winddirection shifts every Day
  //One Dimensional Perlin Noise
  WindDirection[0] = (perlin.GetValue(timeInterval, seed, seed));
  WindDirection[1] = (perlin.GetValue(timeInterval, seed+timeInterval, seed));

  for(size_t i=0; i<gridSize; i++){
    for(size_t j=0; j<gridSize; j++){
      //Previous Tiles
      size_t k = i+cellSize*(WindDirection[0]);
      if(k > gridSize-1){k = i;};
      size_t l = j+cellSize*(WindDirection[1]);
      if(l > gridSize-1){l = j;};

      const size_t cell = i*gridSize+j;
      const size_t fromCell = k*gridSize+l;
      WindMap[cell]=5*(1-(terrain->depthMap[cell]-terrain->depthMap[fromCell])/1000);
    }
  }
}

void Climate::initTempMap(const Terrain* terrain){
  for(size_t i=0; i<gridSize; i++){
    for(size_t j=0; j<gridSize; j++){
      const size_t cell = i*gridSize+j;

      //Sea Temperature
      TempMap[cell]=0.7; //In Degrees Celsius

      //Add for Height
      if(terrain->depthMap[cell]>200){
        //In Degrees Celsius
        TempMap[cell]=1-terrain->depthMap[cell]/2000;
      }
    }
  }
}

void Climate::initHumidityMap(const Terrain* terrain){
  //Calculate the Humidity Grid
  for(size_t i=0; i<gridSize; i++){
    for(size_t j=0; j<gridSize; j++){
      const size_t cell = i*gridSize+j;

      //Sea Level Temperature
      HumidityMap[cell]=0; //In Degrees Celsius

      //Humidty Increases for
      if(terrain->depthMap[cell]<200){
        //In Degrees Celsius
        HumidityMap[cell]=0.4;
      }
      else{
        HumidityMap[cell]=0.2;
      }
    }
  }
}

void Climate::initCloudMap(){
  const size_t gridSizeSq = gridSize*gridSize;
  memset(CloudMap,0,gridSizeSq*sizeof(bool));
}

void Climate::initRainMap(){
  const size_t gridSizeSq = gridSize*gridSize;
  memset(RainMap,0,gridSizeSq);
}

void Climate::calcHumidityMap(const Terrain* terrain){
  const size_t gridSizeSq = gridSize*gridSize;
  float* oldHumidMap = new float[gridSizeSq];
  memcpy(oldHumidMap,HumidityMap,gridSizeSq*sizeof(float));

  for(size_t i=1; i<gridSize-1; i++){
    for(size_t j=1; j<gridSize-1; j++){
      const size_t cell = i*gridSize+j;
      const size_t prevRow  = (i-1)*gridSize;
      const size_t nextRow  = (i+1)*gridSize;

      //Get New Map from Wind Direction
      //Indices of Previous Tile
      //Assumption: Wind Blows Despite Obstacles
      size_t k = i+2*WindMap[cell]*(WindDirection[0]);
      if(k > gridSize-1){k = i;};
      size_t l = j+2*WindMap[cell]*(WindDirection[1]);
      if(l > gridSize-1){l = j;};

      const size_t fromCell = k*gridSize+l;
      
      //Transfer to New Tile
      HumidityMap[cell]=oldHumidMap[fromCell];

      //Average (with all surrounding cells)
      HumidityMap[cell] = 
        (HumidityMap[prevRow+j-1]+HumidityMap[prevRow+j]+HumidityMap[prevRow+j+1]+
         HumidityMap[cell -1]+HumidityMap[cell] +HumidityMap[cell+1] +
         HumidityMap[nextRow+j-1]+HumidityMap[nextRow+j]+HumidityMap[nextRow+j+1]
        )/9;

      //We are over a body of water, temperature accelerates
      float addHumidity=0;
      if(CloudMap[cell]==0){
        addHumidity=0.01;
        if(terrain->depthMap[cell]<=200){
          addHumidity = 0.05*TempMap[cell];
        }
      }

      //Raining
      float addRain=0;
      if(RainMap[cell]==1){
        addRain = -(HumidityMap[cell])*0.8;
      }

      HumidityMap[cell]+=(HumidityMap[cell])*addRain+(1-HumidityMap[cell])*(addHumidity);
      if(HumidityMap[cell]>1){HumidityMap[cell]=1;}
      if(HumidityMap[cell]<0){HumidityMap[cell]=0;}
    }
  }
  delete oldHumidMap;
}

void Climate::calcTempMap(const Terrain* terrain){
  const size_t gridSizeSq = gridSize*gridSize;
  float* oldTempMap = new float[gridSizeSq];
  memcpy(oldTempMap,TempMap,gridSizeSq*sizeof(float));

  for(size_t i=1; i<gridSize-1; i++){
    for(size_t j=1; j<gridSize-1; j++){
      const size_t cell = i*gridSize+j;
      
      //Get New Map from Wind Direction
      //Indices of Previous Tile
      size_t k = i+2*WindMap[cell]*(WindDirection[0]);
      if(k > gridSize-1){k = i;};
      size_t l = j+2*WindMap[cell]*(WindDirection[1]);
      if(l > gridSize-1){l = j;};

      const size_t fromCell = k*gridSize+l;

      //Transfer to New Tile
      TempMap[cell]=oldTempMap[fromCell];

      //Average (from corners to this cell)
      TempMap[cell] = (TempMap[(i-1)*gridSize+j-1]+TempMap[(i+1)*gridSize+j-1]+TempMap[(i+1)*gridSize+j+1]+TempMap[(i-1)*gridSize+j+1])/4;

      //Various Contributions to the TempMap
      //Rising Air Cools

      float addCool = 0.5*(WindMap[cell]-5);


      //Sunlight on Surface
      float addSun = 0;
      if(CloudMap[cell]==0){
        addSun = (1-terrain->depthMap[cell]/2000)*0.008;
      }

      float addRain = 0;
      if(RainMap[cell]==1 && TempMap[cell]>0){
        //Rain Reduces Temperature
        addRain = -0.01;
      }

      //Add Contributions

      TempMap[cell]+=0.8*(1-TempMap[cell])*(addSun)+0.6*(TempMap[cell])*(addRain+addCool);
      if(TempMap[cell]>1){TempMap[cell]=1;}
      if(TempMap[cell]<0){TempMap[cell]=0;}
    }
  }
  delete oldTempMap;
}

void Climate::calcDownfallMap(){
  const size_t gridSizeSq = gridSize*gridSize;
  float* oldCloudMap = new float[gridSizeSq];
  float* oldRainMap  = new float[gridSizeSq];
  memcpy(oldRainMap ,RainMap ,gridSizeSq*sizeof(float));
  memcpy(oldCloudMap,CloudMap,gridSizeSq*sizeof(float));
  memset(CloudMap,0,gridSizeSq*sizeof(bool));
  memset(RainMap ,0,gridSizeSq*sizeof(bool));

  for(size_t i=1; i<gridSize-1; i++){
    for(size_t j=1; j<gridSize-1; j++){
      const size_t cell = i*gridSize+j;
      
      //Old Coordinates
      size_t k = i+2*WindMap[cell]*(WindDirection[0]);
      if(k > gridSize-1){k = i;};
      size_t l = j+2*WindMap[cell]*(WindDirection[1]);
      if(l > gridSize-1){l = j;};
      
      const size_t fromCell = k*gridSize+l;
      
      //Transfer to New Tile
      CloudMap[cell]=oldCloudMap[fromCell];
      RainMap [cell]=oldRainMap [fromCell];

      //Rain Condition
      if(HumidityMap[cell]>=0.35+0.5*TempMap[cell]){
        RainMap[cell]=1;
      }
      else if(HumidityMap[cell]>=0.3+0.3*TempMap[cell]){
        CloudMap[cell]=1;
      }
      else{
        CloudMap[cell]=0;
        RainMap[cell]=0;
      }
    }
  }
  delete oldCloudMap;
  delete oldRainMap;
}
