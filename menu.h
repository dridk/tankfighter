#ifndef __MENU_H__
#define __MENU_H__
#include "coretypes.h"
#include <vector>
#include "entity.h"
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

class Menu:public Entity
{
	public:
	Menu(Engine *engine);
	virtual ~Menu();
	virtual Vector2d getSize() const;

	virtual void draw(sf::RenderTarget &target) const;
	virtual Vector2d movement(sf::Int64 tm);
	virtual void event_received(EngineEvent *event);

	void addItem(const char *item_string, void *user_data = NULL, int beforeItem=-1);
	int getSelected(void **puser_data = NULL) const;
	size_t getItemCount(void) const;

	void selectNext(void);
	void selectPrevious(void);
	void selectByIndex(int itemIndex);
	void selectFirst(void);
	void selectLast(void);
	bool selectionValidated(void) const;
	void escape(void);


	private:
	struct MenuItem {
		std::string title;
		void *user_data;
	};
	struct MenuMetrics {
		sf::Vector2u wsize;
		unsigned hmargin, wmargin, tmargin;
		float hspacing;

		sf::FloatRect clr;
		int font_size;
		int start, end;
		float texth;
	};
	std::vector<MenuItem> items;
	int selectedItem;
	sf::Font font;
	sf::Clock key_repeat;
	unsigned char irep;
	bool validated;
	void draw_string(sf::RenderTarget &target, const char *str, Vector2d pos, int font_size, sf::Color color) const;
	Vector2d text_size(unsigned font_size) const;
	void controllerFeedback(void);
	MenuMetrics getMetrics(void) const;
	int getItemFromPosition(const sf::Vector2f &pos) const;
	Vector2d ompos;
	bool mouseLeftWasPressed;
};
#endif
