#include "texture_cache.h"
#include "parameters.h"
#include "image_helper.h"
#include "misc.h"
#include <algorithm>
#include <gl/gl.h>
#include <string.h>

using namespace sf;

bool supports_rectangle_textures() {
	static bool supports = false;
	static bool launched = false;
	if (launched) return supports;
	
	launched = true;
	
	const char *ext = (const char*)glGetString(GL_EXTENSIONS);
	if (!ext) return false;
	char *newp = cstrdup(ext);
	if (!newp) return false;
	bool first=true;
	
	while (char *word=strtok((first?newp:NULL), " ")) {
		if (strcmp(word, "GL_ARB_texture_rectangle")==0 || strcmp(word, "GL_NV_texture_rectangle")==0)
			{supports=true;break;}
		first=false;
	}
	free(newp);
	return supports;
}

bool load_from_pixels(Image &img, const void *pixels, size_t width, size_t height, size_t channels) {
	img.create(width, height);
	const unsigned char *p0 = (const unsigned char*)pixels;
	for(size_t y=0; y < height; y++) {
		const unsigned char *p = p0+y*width*channels;
		for(size_t x=0; x < width; x++) {
			img.setPixel(x,y,Color(p[0],p[1],p[2],p[3]));
			p += channels;
		}
	}
	return true;
}
bool load_gl_image(Image &img, const char *path, Vector2u &disp_size) {
	Image simg;
	if (supports_rectangle_textures()) {
		if (!img.loadFromFile(path)) return false;
		disp_size = img.getSize();
	}
	if (!simg.loadFromFile(path)) return false;
	Vector2u size = simg.getSize();
	disp_size = size;
	size_t msz = Texture::getMaximumSize();
	
	size_t sqsize = std::max(size.x, size.y);
	size_t optsize = 1;
	while (optsize < sqsize) optsize *= 2;
	if (optsize > msz) optsize = msz;
	
	if (optsize == size.x && optsize == size.y) {
		img = simg;
		return true;
	}
	const size_t channels = 4;
	unsigned char *pixels = (unsigned char*)malloc(optsize*optsize*channels);
	if (!pixels) return false;
	if (!scale_image(simg.getPixelsPtr(), size.x, size.y, channels, pixels, optsize, optsize)) {
		free(pixels);
		return false;
	}
	bool ok = load_from_pixels(img, pixels, optsize, optsize, channels);
	free(pixels);
	return ok;
}

unsigned TextureCache::getTextureID(const char *name) {
	std::string path = parameters.spritesDirectory() + name + parameters.spritesExtension();
	MapIterator it = idmap.find(path);
	if (it == idmap.end()) {
		textures.push_back(Texture());
		sprites.push_back(Sprite());
		Image image;
		Vector2u disp_size;
		Vector2u img_size;
		load_gl_image(image, path.c_str(), disp_size);
		img_size = image.getSize();
		Texture &tex = textures[textures.size()-1];
		Sprite &spr = sprites[sprites.size()-1];
		tex.loadFromImage(image);
		Vector2f scal(double(disp_size.x) / img_size.x, double(disp_size.y) / img_size.y);
		spr.setScale(scal);
		spr.setTexture(tex);
		tex.setRepeated(false);
		tex.setSmooth(true);
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
