#ifndef __TEXTURE_CACHE_H__
#define __TEXTURE_CACHE_H__
#include "coretypes.h"
#include <SFML/Graphics.hpp>
#include <map>
#include <deque>

class TextureCache {
	public:
	TextureCache();
	~TextureCache();
	unsigned getTextureID(const char *name);
	sf::Texture *getTexture(unsigned ID);
	sf::Sprite  *getSprite(unsigned ID);
	sf::Sprite  *getSprite(const char *name);
	private:
	typedef std::map<std::string, unsigned> MapType;
	typedef MapType::iterator MapIterator;

	MapType idmap;
	std::deque<sf::Texture> textures;
	std::deque<sf::Sprite>  sprites;
};
#endif
