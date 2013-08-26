#include "texture_cache.h"
#include "parameters.h"
#include "image_helper.h"
#include "misc.h"
#include <unistd.h>
#include <algorithm>
#include <GL/gl.h>
#include <string.h>

#ifdef WITH_FOG
#include <Fog/Core.h>
#include <Fog/G2d.h>
using Fog::StringW;
#endif

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

static bool load_from_pixels(Image &img, const void *pixels, size_t width, size_t height, size_t channels, bool rendian=false) {
	img.create(width, height);
	const unsigned char *p0 = (const unsigned char*)pixels;
	for(size_t y=0; y < height; y++) {
		const unsigned char *p = p0+y*width*channels;
		for(size_t x=0; x < width; x++) {
			img.setPixel(x,y,(rendian?Color(p[2],p[1],p[0],p[3]):Color(p[0],p[1],p[2],p[3])));
			p += channels;
		}
	}
	return true;
}
#ifdef WITH_FOG
bool loadSvgFile(Image &img, const char *path) {
	Fog::Logger::getGlobal()->setSeverity(100);
	Fog::Logger::getLocal()->setSeverity(100);
	
	size_t width = 400, height = 400;
	Fog::SvgDocument svg;
	unsigned err;
	if ((err=svg.readFromFile(StringW::fromAscii8(path)))) {
		fprintf(stderr, "Failed to load SVG %s error %05X ERR_XML_SAX_NO_DOCUMENT=%05X\n", path, err, Fog::ERR_XML_SAX_NO_DOCUMENT);
		return false;
	}
	Fog::SizeF size = svg.getDocumentSize();
	if (size.w >= 2 && size.w <= 2048) width = size.w;
	if (size.h >= 2 && size.h <= 2048) height = size.h;
	svg.getResourceManager()->loadQueuedResources();
	Fog::SizeI si;
	void *imgdata=malloc(width*height*4);
	if (!imgdata) return false;
	si.w = width; si.h = height;
	Fog::ImageBits imgbits(si, Fog::IMAGE_FORMAT_PRGB32, width*4, (unsigned char*)imgdata);
	Fog::Painter painter;
	if ((err=painter.begin(imgbits))) {
		fprintf(stderr, "Failed to select surface to SVG rendering %s error %05X\n", path, err);
	}
	
	painter.setSource(Fog::Argb32(0xFFFFFFFF));
	painter.fillAll();
	if ((err=svg.render(&painter))) {
		fprintf(stderr, "Failed to render SVG %s error %05X\n", path, err);
	}
	painter.flush(Fog::PAINTER_FLUSH_SYNC);
	painter.end();
	load_from_pixels(img, imgdata, width, height, 4, true);
	free(imgdata);
	return true;
}
#endif
bool load_gl_image(Image &img, const char *path, Vector2u &disp_size) {
#ifdef WITH_FOG
	if (strlen(path) >= 4 && strcmp(path+strlen(path)-4, ".svg")==0) {
		if (!loadSvgFile(img, path)) return false;
	} else
#endif
	       if (!img.loadFromFile(path)) return false;
	Vector2u osize;
	Vector2u size = img.getSize();
	size_t msz = Texture::getMaximumSize();
	disp_size = size;
	
	if (supports_rectangle_textures()) {
		osize = size;
	} else {
		size_t sqsize = std::max(size.x, size.y);
		size_t optsize = 1;
		while (optsize < sqsize) optsize *= 2;
		osize.x = osize.y = optsize;
	}
	if (osize.x > msz) osize.x = msz;
	if (osize.y > msz) osize.y = msz;
	
	if (osize.x == size.x && osize.y == size.y) {
		return true;
	}
	const size_t channels = 4;
	unsigned char *pixels = (unsigned char*)malloc(osize.x*osize.y*channels);
	if (!pixels) return false;
	if (!scale_image(img.getPixelsPtr(), size.x, size.y, channels, pixels, osize.x, osize.y)) {
		free(pixels);
		return false;
	}
	bool ok = load_from_pixels(img, pixels, osize.x, osize.y, channels);
	free(pixels);
	return ok;
}

static std::string texturePath(const std::string dir, const std::string name, const std::string ext) {
	std::string p1 = dir + name + ext;
	if (access(p1.c_str(), R_OK) != -1) return p1;
	p1 = dir + name + ".jpg";
	if (access(p1.c_str(), R_OK) != -1) return p1;
	p1 = dir + name + ".jpeg";
	if (access(p1.c_str(), R_OK) != -1) return p1;
#ifdef WITH_FOG
	p1 = dir + name + ".svg";
	if (access(p1.c_str(), R_OK) != -1) return p1;
#endif
	return "";
}

unsigned TextureCache::loadTexture(const char *name) {
	std::string path = texturePath(parameters.spritesDirectory(), name, parameters.spritesExtension());
	
	if (path == "") return (unsigned)-1;
	Image image;
	Vector2u disp_size;
	Vector2u img_size;
	if (!load_gl_image(image, path.c_str(), disp_size)) {
		return (unsigned)-1;
	}
	img_size = image.getSize();
	textures.push_back(Texture());
	Texture &tex = textures[textures.size()-1];
	if (!tex.loadFromImage(image)) {textures.pop_back();return (unsigned)-1;}
	tex.setRepeated(false);
	tex.setSmooth(true);
	
	sprites.push_back(Sprite());
	Sprite &spr = sprites[sprites.size()-1];
	Vector2f scal(double(disp_size.x) / img_size.x, double(disp_size.y) / img_size.y);
	spr.setScale(scal);
	spr.setTexture(tex);
	idmap.insert(MapType::value_type(path, textures.size()-1));
	return textures.size()-1;
}
unsigned TextureCache::getTextureID(const char *name) {
	MapIterator it = idmap.find(name);
	if (it != idmap.end()) return (*it).second;
	
	unsigned index = loadTexture(name);
	idmap.insert(MapType::value_type(name, index));
	return index;
}
sf::Texture *TextureCache::getTexture(unsigned ID) {
	if (ID == (unsigned)-1) return &textures[0];
	return &textures[ID];
}
sf::Sprite *TextureCache::getSprite(unsigned ID) {
	if (ID == (unsigned)-1) return &sprites[0];
	return &sprites[ID];
}
sf::Sprite *TextureCache::getSprite(const char *name) {
	unsigned ID=getTextureID(name);
	return getSprite(ID);
}
TextureCache::TextureCache() {
	/* create fallback texture */
	textures.push_back(Texture());
	sprites.push_back(Sprite());
	Texture &tex=textures[0];
	Sprite  &spr=sprites[0];
	Image img;

	img.create(2,2);
	img.setPixel(0,0,Color::Black);
	img.setPixel(1,1,Color::Black);
	img.setPixel(0,1,Color::White);
	img.setPixel(1,0,Color::White);
	tex.loadFromImage(img);
	
	spr.setTexture(tex);
	spr.setScale(32,32);
}
TextureCache::~TextureCache() {}
