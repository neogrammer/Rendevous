#ifndef SOLUTIONHELPER_H__
#define SOLUTIONHELPER_H__
#include <stdint.h>
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector3.hpp>


class hlp
{
public:
	static std::pair<float, float> worldOrigin;
	static std::pair<float, float> worldSize;
	static std::pair<float, float> tileSize;

	/// <summary>
	/// What you need: tileW_ and tileH_ are the ground portion of the texture image in the usual case of full cubes as the texture image if the isometric tile, as only the top half is needed for calculations.  Theses are in pixels (the width and height) of the rectangular area surrounding that top half or ground portion of the square isometrix tile, usual case, if ur using tiles with the width fudged to exactly twice the height, that is the ratio,(or model, you may look at it as) that this calculation assumes it is working with.
	/// x_ and y_ are.. you guessed it!  The position of the coordinate you wish to convert to screenspace, where the isometric illusion magic fucks with their eyes.
	/// </summary>
	/// <param name="x_">cell space(where it is seemingly equivalent to the screen space, but) divided by the tile size, integer type, truncate any floating decimals you may get in calculation, this snaps the coordinate to hardcore values for precise placement</param>
	/// <param name="y_">cell space(where it is seemingly equivalent to the screen space, but) divided by the tile size, integer type, truncate any floating decimals you may get in calculation, this snaps the coordinate to hardcore values for precise placement</param>
	/// <param name="tileW_">in pixels.. the rectangular area of a tile that encompasses the diamond shaped graphic ur using for the tiles</param>
	/// <param name="tileH_">in pixels.. the rectangular area of a tile that encompasses the diamond shaped graphic ur using for the tiles</param>
	/// <returns></returns>
	static inline std::pair<uint32_t, uint32_t> ToScreen(float x_, float y_, uint32_t tileW_ = (uint32_t)tileSize.first, uint32_t tileH_ = tileSize.second)
	{
		// lets rotate the rectangular grid and draw it like that because fuck it, why not.   Dont fuck this up.  
		float isoPosx, isoPosy;
		isoPosx = (worldOrigin.first * tileSize.first) + float(x_ - y_) * ((float)tileW_ / 2.f);
		isoPosy = (worldOrigin.second * tileSize.second) + float(x_ + y_) * ((float)tileH_ / 2.f);
		//coord transformed to an isometric view with an orthographic projection, sike!! its just an illusion, the images are already cock-eyed, so we are just gonna plop em down where they would go, if every pixel was interpolated to give the effect of isometric
		//    , but we are cheating.. cuz we stupid cheater cheat cheats.. shhh.. don't tell me.. I'll get mad at me and ground myself again...
		return std::pair<uint32_t, uint32_t>{ (uint32_t)isoPosx, (uint32_t)isoPosy };
	};

	/// <summary>
	/// Pass in the cell position in worldspace that you want to revert back to cellSpace ( == raw world position / tilesize) ****> WHICH GIVES YOU THE TILE IN CELL SPACE FROM WORLD SPACE, IMPORTANT!  <**************
	/// </summary>
	/// <param name="x_">What cell in the world space, if you follow the isometric grid..'s x coordinate to be defined as what cell on the screen it is, then multiply by the tilesize and boom, you got your coords to run collisions with from isometric coords in world space on an isometric grid that the user sees</param>
	/// <param name="y_">What cell in the world space, if you follow the isometric grid..'s x coordinate to be defined as what cell on the screen it is, then multiply by the tilesize and boom, you got your coords to run collisions with from isometric coords in world space on an isometric grid that the user sees</param>
	/// <param name="tileW_">in pixels.. the rectangular area of a tile that encompasses the diamond shaped graphic ur using for the tiles</param>
	/// <param name="tileH_">in pixels.. the rectangular area of a tile that encompasses the diamond shaped graphic ur using for the tiles</param>
	/// <returns></returns>
	static inline std::pair<uint32_t, uint32_t> ToCart(uint32_t x_, uint32_t y_, uint32_t tileW_ = (uint32_t)tileSize.first, uint32_t tileH_ = tileSize.second)
	{

		float isoPosx{ (float)x_ }, isoPosy{ (float)y_ };

		float cartX = (isoPosx - ((float)worldOrigin.first * (float)tileSize.first)) / (float)tileSize.first;
		float cartY = ((isoPosy - ((float)worldOrigin.second * (float)tileSize.second)) / (float)tileSize.second) - ((isoPosx - ((float)worldOrigin.first * (float)tileSize.first)) / (float)tileSize.first);
		//coord transformed to an isometric view with an orthographic projection, sike!! its just an illusion, the images are already cock-eyed, so we are just gonna plop em down where they would go, if every pixel was interpolated to give the effect of isometric
		//    , but we are cheating.. cuz we stupid cheater cheat cheats.. shhh.. don't tell me.. I'll get mad at me and ground myself again...
		return std::pair{ (uint32_t)cartX, (uint32_t)cartY };
	};


	/// <summary>
	/// take a coordinate in the world space as pixels, spits out screen position of that world coord to be drawn on screen at the illusioned placement in cell space.. which is cell wrt the screen laid out in a 2d grid of tile size cells.
	///  The third component comes back as encoded cell offset within that cell.. soo 20 pixels to the right and 15 down from the top of the cell would bring back a z value of 20.15f, respectively so you get that for free  
	/// </summary>
	/// <param name="pos_"></param>
	/// <returns></returns>
	static inline sf::Vector3f ToScreenIso_wOffset(sf::Vector2f pos_)
	{
		sf::Vector2i cell = { (int)(pos_.x / tileSize.first),(int)(pos_.y / tileSize.second) };
		sf::Vector2i offset = { ((int)pos_.x % (int)tileSize.first),((int)pos_.y % (int)tileSize.second) };
		sf::Vector3f out{};
		std::string front{ std::to_string(offset.x) };
		std::string back{ std::to_string(offset.y) };
		front.append(".");
		front.append(back);
		std::string full{ front };
		out.x = (cell.x - cell.y) * (tileSize.first / 2.f);
		out.y = (cell.x + cell.y) * (tileSize.second / 2.f);
		out.z = (float)std::atof(full.c_str());

		sf::Vector2f out2 = sf::Vector2f{ (worldOrigin.first * tileSize.first), (worldOrigin.second * tileSize.second) } + sf::Vector2f{ out.x, out.y };
		return { out2.x, out2.y, out.z };
	};

	/// <summary>
    /// take a coordinate in the world space as pixels, spits out screen position of that world coord to be drawn on screen at the illusioned placement in cell space.. which is cell wrt the screen laid out in a 2d grid of tile size cells.
    ///  The third component comes back as encoded cell offset within that cell.. soo 20 pixels to the right and 15 down from the top of the cell would bring back a z value of 20.15f, respectively so you get that for free  
    /// </summary>
    /// <param name="pos_"></param>
    /// <returns></returns>
	static inline sf::Vector3f ToScreenIso_wOffset(float posx_, float posy_)
	{
		sf::Vector2i cell = { (int)(posx_ / tileSize.first),(int)(posy_ / tileSize.second)};
		sf::Vector2i offset = { ((int)posx_ % (int)tileSize.first),((int)posy_ % (int)tileSize.second) };
		sf::Vector3f out{};
		std::string front{ std::to_string(offset.x) };
		std::string back{ std::to_string(offset.y) };
		front.append(".");
		front.append(back);
		std::string full{ front };
		out.x = (cell.x - cell.y) * (tileSize.first / 2.f);
		out.y = (cell.x + cell.y) * (tileSize.second / 2.f);
		out.z = (float)std::atof(full.c_str());

		sf::Vector2f out2 = sf::Vector2f{ (worldOrigin.first * tileSize.first), (worldOrigin.second * tileSize.second) } + sf::Vector2f{ out.x, out.y };
		return { out2.x, out2.y, out.z };
	};

	/// <summary>
	/// take a coordinate in the world space as pixels, spits out screen position of that world coord to be drawn on screen at the illusioned placement
	/// </summary>
	/// <param name="pos_"></param>
	/// <returns></returns>
	static inline sf::Vector2f ToScreenIso(sf::Vector2f pos_)
	{
		sf::Vector2i cell = { (int)(pos_.x / tileSize.first),(int)(pos_.y / tileSize.second) };


		float isox{}, isoy{};
		isox = (float)(cell.x - cell.y) * (tileSize.first / 2.f);
		isoy = (float)(cell.x + cell.y) * (tileSize.second / 2.f);

		sf::Vector2f out = sf::Vector2f{ (worldOrigin.first * tileSize.first), (worldOrigin.second * tileSize.second) } + sf::Vector2f{ isox, isoy };
		return out;
	};

	/// <summary>
	/// take a coordinate in the world space as pixels, spits out screen position of that world coord to be drawn on screen at the illusioned placement
	/// </summary>
	/// <param name="pos_"></param>
	/// <returns></returns>
	static inline sf::Vector2f ToScreenIso(float posx_, float posy_)
	{
		sf::Vector2i cell = { (int)(posx_ / tileSize.first),(int)(posy_ / tileSize.second) };

		float isox{}, isoy{};
		isox = float(cell.x - cell.y) * (tileSize.first / 2.f);
		isoy = float(cell.x + cell.y) * (tileSize.second / 2.f);

		sf::Vector2f out = sf::Vector2f{(worldOrigin.first * tileSize.first),(worldOrigin.second * tileSize.second) } + sf::Vector2f{ isox, isoy };
		return out;
	};

	/// <summary>
	/// take a coordinate in cell space, spits out screen position in pixels of that world coord to be drawn on screen at the illusioned placement
	/// </summary>
	/// <param name="pos_"></param>
	/// <returns></returns>
	static inline sf::Vector2f ToScreenIso_4mCell(sf::Vector2i pos_)
	{
		int isox{}, isoy{};
		isox = (pos_.x - pos_.y) * ((int)tileSize.first / 2);
		isoy = (pos_.x + pos_.y) * ((int)tileSize.second / 2);

		sf::Vector2f out = sf::Vector2f{ (worldOrigin.first * tileSize.first), (worldOrigin.second * tileSize.second) } + sf::Vector2f{ (float)isox, (float)isoy };
		return out;
	};

	/// <summary>
	/// take a coordinate in the world space in cell space, spits out screen position of that world coord to be drawn on screen at the illusioned placement
	/// </summary>
	/// <param name="pos_"></param>
	/// <returns></returns>
	static inline sf::Vector2f ToScreenIso_4mCell(int posx_, int posy_)
	{

		int isox{}, isoy{};
		isox = (posx_ - posy_) * ((int)tileSize.first / 2);
		isoy = (posx_ + posy_) * ((int)tileSize.second / 2);

		sf::Vector2f out = sf::Vector2f{ (worldOrigin.first * tileSize.first), (worldOrigin.second * tileSize.second) } + sf::Vector2f{ (float)isox, (float)isoy };
		return out;

	};



};
#endif