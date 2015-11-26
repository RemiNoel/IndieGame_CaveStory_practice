#include "Slope.h"
#include "Level.h"
#include "Graphics.h"
#include "tinyxml2.h"
#include "Utils.h"
#include <SDL.h>

#include <sstream>
#include <algorithm>
#include <math.h>

using namespace tinyxml2;

Level::Level(){

}

Level::Level(std::string mapName, Vector2 spawnPoint, Graphics &graphics) :
	_mapName(mapName),
	_spawnPoint(spawnPoint),
	_size(Vector2(0, 0))
{
	this->loadMap(mapName, graphics);
}

void Level::loadMap(std::string mapName, Graphics& graphics){
	//Parse the .tmx file
	XMLDocument doc;
	std::stringstream ss;
	ss << "Content/maps/" << mapName << ".tmx"; // Pass in Map 1, we get maps/Map 1.tmx
	doc.LoadFile(ss.str().c_str());

	XMLElement* mapNode = doc.FirstChildElement("map");

	//Get the width and the height of the whole map and store it in _size
	int width, height;
	mapNode->QueryIntAttribute("width", &width);
	mapNode->QueryIntAttribute("height", &height);
	this->_size = Vector2(width, height);

	//Get the width and the height of the tiles and store it in _tileSize

	int tileWidth, tileHeight;
	mapNode->QueryIntAttribute("tilewidth", &tileWidth);
	mapNode->QueryIntAttribute("tileheight", &tileHeight);
	this->_tileSize = Vector2(tileWidth, tileHeight);

	//Loading the tile sets
	XMLElement* pTileset = mapNode->FirstChildElement("tileset");
	if (pTileset != NULL){
		while (pTileset){
			int firstgid;
			const char* source = pTileset->FirstChildElement("image")->Attribute("source");
			std::stringstream ss;
			ss << source;

			pTileset->QueryIntAttribute("firstgid", &firstgid);
			SDL_Texture* tex = SDL_CreateTextureFromSurface(graphics.getRenderer(), graphics.loadImage(ss.str()));

			this->_tilesets.push_back(Tileset(tex, firstgid));

			//Get all of the animations for that tileset
			XMLElement* pTileA = pTileset->FirstChildElement("tile");
			if (pTileA != NULL){
				while (pTileA){
					AnimatedTileInfo ati;
					ati.StartTileId = pTileA->IntAttribute("id") + firstgid;
					ati.TilesetsFirstGid = firstgid;
					XMLElement* pAnimation = pTileA->FirstChildElement("animation");

					if (pAnimation != NULL){
						while (pAnimation){

							XMLElement* pFrame = pAnimation->FirstChildElement("frame");
							if (pFrame != NULL){
								while (pFrame){
									ati.TileIds.push_back(pFrame->IntAttribute("tileid") + firstgid);
									ati.Duration = pFrame->IntAttribute("duration");

									pFrame = pFrame->NextSiblingElement("frame");
								}
							}
							pAnimation = pAnimation->NextSiblingElement("animation");
						}
					}
					this->_animatedTileInfos.push_back(ati);

					pTileA = pTileA->NextSiblingElement("tile");
				}
			}

			pTileset = pTileset->NextSiblingElement("tileset");
		}
	}

	XMLElement* pLayer = mapNode->FirstChildElement("layer");
	if (pLayer != NULL){
		while (pLayer){
			//Loading the data element
			XMLElement* pData = pLayer->FirstChildElement("data");

			if (pData != NULL){
				while (pData){
					//Loading the tile element
					XMLElement* pTile = pData->FirstChildElement("tile");

					if (pTile != NULL){
						int tileCounter = 0;

						while (pTile){
							//Build each individual tile here
							//If gid is 0, no tile should be drawn. Continue loop
							if (pTile->IntAttribute("gid") == 0){
								tileCounter++;
								if (pTile->NextSiblingElement("tile")){
									pTile = pTile->NextSiblingElement("tile");
									continue;
								}
								else{
									break;
								}
							}

							//get the tileset for this specific gid
							int gid = pTile->IntAttribute("gid");
							Tileset tls;
							int closest = 0;

							for (int i = 0; i < (this->_tilesets.size()); i++){
								if (this->_tilesets[i].FirstGid <= gid){
									if (this->_tilesets[i].FirstGid > closest){
										closest = this->_tilesets[i].FirstGid;
										//This is the tileset we want
										tls = this->_tilesets.at(i);
									}
								}
							}

							if (tls.FirstGid == -1){
								//No tileset was found for this gid
								tileCounter++;
								if (pTile->NextSiblingElement("tile")){
									pTile = pTileset->NextSiblingElement("tile");
									continue;
								}
								else{
									break;
								}
							}

							//Get the position of the tile in the level
							int xx = 0;
							int yy = 0;
							xx = tileCounter % width;
							xx *= tileWidth;
							yy += tileHeight * (tileCounter / width);
							Vector2 finalTilePosition = Vector2(xx, yy);

							// Calculate the position of the tile in the tileset
							Vector2 finalTilesetPosition = this->getTilesetPosition(tls, gid, tileWidth, tileHeight);

							//Build the actual tile and add it to the level's tile list
							bool isAnimatedTile = false;
							AnimatedTileInfo ati; 

							for (int i = 0; i < this->_animatedTileInfos.size(); i++){
								if (this->_animatedTileInfos.at(i).StartTileId == gid){
									ati = this->_animatedTileInfos.at(i);
									isAnimatedTile = true;
								}
							}

							if (isAnimatedTile == true){
								std::vector<Vector2> tilesetPositions; 
								for (int i = 0; i < ati.TileIds.size(); i++){
									tilesetPositions.push_back(this->getTilesetPosition(tls, ati.TileIds.at(i), tileWidth, tileHeight));
								}
								
								AnimatedTile tile(tilesetPositions, ati.Duration, tls.Texture, Vector2(tileWidth, tileHeight), finalTilePosition);
								this->_animatedTileList.push_back(tile);
							}
							else{
								Tile tile(tls.Texture, Vector2(tileWidth, tileHeight),
									finalTilesetPosition, finalTilePosition);
								this->_tileList.push_back(tile);
							}
							tileCounter++;
							pTile = pTile->NextSiblingElement("tile");
						}
					}

					pData = pData->NextSiblingElement("data");
				}
			}

			pLayer = pLayer->NextSiblingElement("layer");
		}
	}

	//Parse out the collisions from .tmx file
	XMLElement* pObjectGroup = mapNode->FirstChildElement("objectgroup");
	if (pObjectGroup != NULL){
		while (pObjectGroup){
			const char* name = pObjectGroup->Attribute("name");
			std::stringstream ss;
			ss << name;

			if (ss.str() == "Collisions"){
				XMLElement* pObject = pObjectGroup->FirstChildElement("object");
				if (pObject != NULL){
					while (pObject){
						float x, y, width, height;
						
						x = pObject->FloatAttribute("x");
						y = pObject->FloatAttribute("y");
						width = pObject->FloatAttribute("width");
						height = pObject->FloatAttribute("height");

						this->_collisionRects.push_back(Rectangle(
							std::ceil(x) * globals::SPRITE_SCALE,
							std::ceil(y) * globals::SPRITE_SCALE,
							std::ceil(width) * globals::SPRITE_SCALE,
							std::ceil(height) * globals::SPRITE_SCALE));

						pObject = pObject->NextSiblingElement("object");
					}
				}
			}

			//Parse slopes from .tmx file
			else if (ss.str() == "Slopes"){
				XMLElement* pObject = pObjectGroup->FirstChildElement("object");
				if (pObject != NULL){
					while (pObject){
						std::vector<Vector2> points;
						Vector2 p1, p2;
						p1 = Vector2(std::ceil(pObject->FloatAttribute("x")), std::ceil(pObject->FloatAttribute("y")));

						XMLElement* pPolyLine = pObject->FirstChildElement("polyline");
						if (pPolyLine != NULL){
							std::vector<std::string> pairs;
							const char* pointString = pPolyLine->Attribute("points");

							std::stringstream ss;
							ss << pointString;
							Utils::split(ss.str(), pairs, ' ');
							
							//Now we have each of the pairs. Loop through the list of pairs
							// and split them into Vector2s and then store them in our points vector
							for (int i = 0; i < pairs.size(); i++){
								std::vector<std::string> ps;
								Utils::split(pairs.at(i), ps, ',');
								points.push_back(Vector2(std::stoi(ps.at(0)), std::stoi(ps.at(1))));
							}
						}

						for (int i = 0; i < points.size(); i += 2){
							this->_slopes.push_back(Slope(
								Vector2((p1.x + points.at(i < 2 ? i : i - 1).x) * globals::SPRITE_SCALE,
										(p1.y + points.at(i < 2 ? i : i - 1).y) * globals::SPRITE_SCALE),
								Vector2((p1.x + points.at(i < 2 ? i + 1 : i).x) * globals::SPRITE_SCALE,
										(p1.y + points.at(i < 2 ? i + 1 : i).y) * globals::SPRITE_SCALE)
								));
						}

						pObject = pObject->NextSiblingElement("object");
					}
				}
			}

			//Parse spawn points from .tmx file
			else if (ss.str() == "Spawn points"){
				XMLElement* pObject = pObjectGroup->FirstChildElement("object");
				if (pObject != NULL){
					while (pObject){
						float x = pObject->FloatAttribute("x");
						float y = pObject->FloatAttribute("y");
						const char* name = pObject->Attribute("name");
						std::stringstream ss;
						ss << name;
						if (ss.str() == "Player"){
							this->_spawnPoint = Vector2(std::ceil(x) * globals::SPRITE_SCALE, std::ceil(y) * globals::SPRITE_SCALE);
						}

						pObject = pObject->NextSiblingElement("object");
					}
				}
			}

			pObjectGroup = pObjectGroup->NextSiblingElement("objectgroup");
		}
	}
}

void Level::update(int elapsedTime){
	for (int i = 0; i < this->_animatedTileList.size(); i++){
		this->_animatedTileList.at(i).update(elapsedTime);
	}
}

void Level::draw(Graphics &graphics){
	for (int i = 0; i < (this->_tileList.size()); i++){
		this->_tileList.at(i).draw(graphics);
	}

	for (int i = 0; i < this->_animatedTileList.size(); i++){
		this->_animatedTileList.at(i).draw(graphics);
	}
}

std::vector<Rectangle> Level::checkTileCollisions(const Rectangle &other){
	std::vector<Rectangle> others;
	for (int i = 0; i < this->_collisionRects.size(); i++){
		if (this->_collisionRects.at(i).collidesWith(other)){
			others.push_back(this->_collisionRects.at(i));
		}
	}
	return others;
}

std::vector<Slope> Level::checkSlopeCollisions(const Rectangle &other){
	std::vector<Slope> others;
	for (int i = 0; i < this->_slopes.size(); i++){
		if (this->_slopes.at(i).collidesWith(other)){
			others.push_back(this->_slopes.at(i));
		}
	}
	return others;
}


const Vector2 Level::getPlayerSpawnPoint() const {
	return this->_spawnPoint;
}

Vector2 Level::getTilesetPosition(Tileset tls, int gid, int tileWidth, int tileHeight){
	int tilesetWidth, tilesetHeight;
	SDL_QueryTexture(tls.Texture, NULL, NULL, &tilesetWidth, &tilesetHeight);
	int tsxx = gid % (tilesetWidth / tileWidth) - 1;
	tsxx *= tileWidth;

	int tsyy = 0; //39.27
	int amount = ((gid - tls.FirstGid) / (tilesetWidth / tileWidth));
	tsyy = tileHeight * amount;
	Vector2 finalTilesetPosition = Vector2(tsxx, tsyy);

	return finalTilesetPosition;
}