#include "texture_cache.h"
#include "parameters.h"

using namespace sf;
unsigned TextureCache::getTextureID(const char *name) {
	std::string path = parameters.spritesDirectory() + name + parameters.spritesExtension();
	MapIterator it = idmap.find(path);
	if (it == idmap.end()) {
		textures.push_back(Texture());
		sprites.push_back(Sprite());
		Texture &tex = textures[textures.size()-1];
		Sprite &spr = sprites[sprites.size()-1];
		tex.loadFromFile(path);
		spr.setTexture(tex);
		tex.setRepeated(true);
		idmap.insert(MapType::value_type(path, textures.size()-1));
		return textures.size()-1;
	}
	return (*it).second;
}
sf::Texture *TextureCache::getTexture(unsigned ID) {
	return &textures[ID];
}
sf::Sprite *TextureCache::getSprite(unsigned ID) {
	return &sprites[ID];
}
sf::Sprite *TextureCache::getSprite(const char *name) {
	unsigned ID=getTextureID(name);
	return getSprite(ID);
}
TextureCache::TextureCache() {}
TextureCache::~TextureCache() {}
