#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <iomanip>
#include <filesystem>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <SDL_net.h>
//#include <SDL_gpu.h>
//#include <SFML/Network.hpp>
//#include <SFML/Graphics.hpp>
#include <boost/locale.hpp>
#include <algorithm>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <locale>
#include <codecvt>
#ifdef __ANDROID__
#include <android/log.h> //__android_log_print(ANDROID_LOG_VERBOSE, "Input4", "Example number log: %d", number);
#include <jni.h>
#include "vendor/PUGIXML/src/pugixml.hpp"
#else
#include <pugixml.hpp>
#include <filesystem>
namespace fs = std::filesystem;
#endif

using namespace std::chrono_literals;

// NOTE: Remember to uncomment it on every release
//#define RELEASE

#if defined _MSC_VER && defined RELEASE
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

//240 x 240 (smart watch)
//240 x 320 (QVGA)
//360 x 640 (Galaxy S5)
//640 x 480 (480i - Smallest PC monitor)

int windowWidth = 240;
int windowHeight = 320;
SDL_Point mousePos;
SDL_Point realMousePos;
bool keys[SDL_NUM_SCANCODES];
bool buttons[SDL_BUTTON_X2 + 1];
SDL_Renderer* renderer;

void logOutputCallback(void* userdata, int category, SDL_LogPriority priority, const char* message)
{
	std::cout << message << std::endl;
}

int random(int min, int max)
{
	return min + rand() % ((max + 1) - min);
}

int SDL_QueryTextureF(SDL_Texture* texture, Uint32* format, int* access, float* w, float* h)
{
	int wi, hi;
	int result = SDL_QueryTexture(texture, format, access, &wi, &hi);
	if (w) {
		*w = wi;
	}
	if (h) {
		*h = hi;
	}
	return result;
}

SDL_bool SDL_PointInFRect(const SDL_Point* p, const SDL_FRect* r)
{
	return ((p->x >= r->x) && (p->x < (r->x + r->w)) &&
		(p->y >= r->y) && (p->y < (r->y + r->h))) ? SDL_TRUE : SDL_FALSE;
}

std::ostream& operator<<(std::ostream& os, SDL_FRect r)
{
	os << r.x << " " << r.y << " " << r.w << " " << r.h;
	return os;
}

std::ostream& operator<<(std::ostream& os, SDL_Rect r)
{
	SDL_FRect fR;
	fR.w = r.w;
	fR.h = r.h;
	fR.x = r.x;
	fR.y = r.y;
	os << fR;
	return os;
}

SDL_Texture* renderText(SDL_Texture* previousTexture, TTF_Font* font, SDL_Renderer* renderer, const std::string& text, SDL_Color color)
{
	if (previousTexture) {
		SDL_DestroyTexture(previousTexture);
	}
	SDL_Surface* surface;
	if (text.empty()) {
		surface = TTF_RenderUTF8_Blended(font, " ", color);
	}
	else {
		surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
	}
	if (surface) {
		SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_FreeSurface(surface);
		return texture;
	}
	else {
		return 0;
	}
}

struct Text {
	std::string text;
	SDL_Surface* surface = 0;
	SDL_Texture* t = 0;
	SDL_FRect dstR{};
	bool autoAdjustW = false;
	bool autoAdjustH = false;
	float wMultiplier = 1;
	float hMultiplier = 1;

	~Text()
	{
		if (surface) {
			SDL_FreeSurface(surface);
		}
		if (t) {
			SDL_DestroyTexture(t);
		}
	}

	Text()
	{

	}

	Text(const Text& rightText)
	{
		text = rightText.text;
		if (surface) {
			SDL_FreeSurface(surface);
		}
		if (t) {
			SDL_DestroyTexture(t);
		}
		if (rightText.surface) {
			surface = SDL_ConvertSurface(rightText.surface, rightText.surface->format, SDL_SWSURFACE);
		}
		if (rightText.t) {
			t = SDL_CreateTextureFromSurface(renderer, surface);
		}
		dstR = rightText.dstR;
		autoAdjustW = rightText.autoAdjustW;
		autoAdjustH = rightText.autoAdjustH;
		wMultiplier = rightText.wMultiplier;
		hMultiplier = rightText.hMultiplier;
	}

	Text& operator=(const Text& rightText)
	{
		text = rightText.text;
		if (surface) {
			SDL_FreeSurface(surface);
		}
		if (t) {
			SDL_DestroyTexture(t);
		}
		if (rightText.surface) {
			surface = SDL_ConvertSurface(rightText.surface, rightText.surface->format, SDL_SWSURFACE);
		}
		if (rightText.t) {
			t = SDL_CreateTextureFromSurface(renderer, surface);
		}
		dstR = rightText.dstR;
		autoAdjustW = rightText.autoAdjustW;
		autoAdjustH = rightText.autoAdjustH;
		wMultiplier = rightText.wMultiplier;
		hMultiplier = rightText.hMultiplier;
		return *this;
	}

	void setText(SDL_Renderer* renderer, TTF_Font* font, std::string text, SDL_Color c = { 255,255,255 })
	{
		this->text = text;
#if 1 // NOTE: renderText
		if (surface) {
			SDL_FreeSurface(surface);
		}
		if (t) {
			SDL_DestroyTexture(t);
		}
		if (text.empty()) {
			surface = TTF_RenderUTF8_Blended(font, " ", c);
		}
		else {
			surface = TTF_RenderUTF8_Blended(font, text.c_str(), c);
		}
		if (surface) {
			t = SDL_CreateTextureFromSurface(renderer, surface);
		}
#endif
		if (autoAdjustW) {
			SDL_QueryTextureF(t, 0, 0, &dstR.w, 0);
		}
		if (autoAdjustH) {
			SDL_QueryTextureF(t, 0, 0, 0, &dstR.h);
		}
		dstR.w *= wMultiplier;
		dstR.h *= hMultiplier;
	}

	void setText(SDL_Renderer* renderer, TTF_Font* font, int value, SDL_Color c = { 255,255,255 })
	{
		setText(renderer, font, std::to_string(value), c);
	}

	void draw(SDL_Renderer* renderer)
	{
		if (t) {
			SDL_RenderCopyF(renderer, t, 0, &dstR);
		}
	}
};

int SDL_RenderDrawCircle(SDL_Renderer* renderer, int x, int y, int radius)
{
	int offsetx, offsety, d;
	int status;


	offsetx = 0;
	offsety = radius;
	d = radius - 1;
	status = 0;

	while (offsety >= offsetx) {
		status += SDL_RenderDrawPoint(renderer, x + offsetx, y + offsety);
		status += SDL_RenderDrawPoint(renderer, x + offsety, y + offsetx);
		status += SDL_RenderDrawPoint(renderer, x - offsetx, y + offsety);
		status += SDL_RenderDrawPoint(renderer, x - offsety, y + offsetx);
		status += SDL_RenderDrawPoint(renderer, x + offsetx, y - offsety);
		status += SDL_RenderDrawPoint(renderer, x + offsety, y - offsetx);
		status += SDL_RenderDrawPoint(renderer, x - offsetx, y - offsety);
		status += SDL_RenderDrawPoint(renderer, x - offsety, y - offsetx);

		if (status < 0) {
			status = -1;
			break;
		}

		if (d >= 2 * offsetx) {
			d -= 2 * offsetx + 1;
			offsetx += 1;
		}
		else if (d < 2 * (radius - offsety)) {
			d += 2 * offsety - 1;
			offsety -= 1;
		}
		else {
			d += 2 * (offsety - offsetx - 1);
			offsety -= 1;
			offsetx += 1;
		}
	}

	return status;
}


int SDL_RenderFillCircle(SDL_Renderer* renderer, int x, int y, int radius)
{
	int offsetx, offsety, d;
	int status;


	offsetx = 0;
	offsety = radius;
	d = radius - 1;
	status = 0;

	while (offsety >= offsetx) {

		status += SDL_RenderDrawLine(renderer, x - offsety, y + offsetx,
			x + offsety, y + offsetx);
		status += SDL_RenderDrawLine(renderer, x - offsetx, y + offsety,
			x + offsetx, y + offsety);
		status += SDL_RenderDrawLine(renderer, x - offsetx, y - offsety,
			x + offsetx, y - offsety);
		status += SDL_RenderDrawLine(renderer, x - offsety, y - offsetx,
			x + offsety, y - offsetx);

		if (status < 0) {
			status = -1;
			break;
		}

		if (d >= 2 * offsetx) {
			d -= 2 * offsetx + 1;
			offsetx += 1;
		}
		else if (d < 2 * (radius - offsety)) {
			d += 2 * offsety - 1;
			offsety -= 1;
		}
		else {
			d += 2 * (offsety - offsetx - 1);
			offsety -= 1;
			offsetx += 1;
		}
	}

	return status;
}

int eventWatch(void* userdata, SDL_Event* event)
{
	// WARNING: Be very careful of what you do in the function, as it may run in a different thread
	if (event->type == SDL_APP_TERMINATING || event->type == SDL_APP_WILLENTERBACKGROUND) {

	}
	return 0;
}

std::string ucs4ToUtf8(const std::u32string& in)
{
	return boost::locale::conv::utf_to_utf<char>(in);
}

std::u32string utf8ToUcs4(const std::string& in)
{
	return boost::locale::conv::utf_to_utf<char32_t>(in);
}

void utf8PopBack(std::string& buffer)
{
	std::size_t textlen = SDL_strlen(buffer.c_str());
	do {
		if (textlen == 0) {
			break;
		}
		// NOTE: If first bit is not set
		if ((buffer[textlen - 1] & 0x80) == 0x00) {
			/* One byte */
			buffer.erase(textlen - 1);
			break;
		}
		// NOTE: If only first bit is set
		if ((buffer[textlen - 1] & 0xC0) == 0x80) {
			/* Byte from the multibyte sequence */
			buffer.erase(textlen - 1);
			textlen--;
			if (textlen == 0) { break; } // invalid character
		}
		// NOTE: If first and second bits are set
		if ((buffer[textlen - 1] & 0xC0) == 0xC0) {
			/* First byte of multibyte sequence */
			buffer.erase(textlen - 1);
			break;
		}
	} while (true);
}

std::string utf8Substr(const std::string& str, unsigned int start, unsigned int leng)
{
	if (leng == 0) { return ""; }
	unsigned int c, i, ix, q, min = std::string::npos, max = std::string::npos;
	for (q = 0, i = 0, ix = str.length(); i < ix; i++, q++) {
		if (q == start) { min = i; }
		if (q <= start + leng || leng == std::string::npos) { max = i; }

		c = (unsigned char)str[i];
		if (
			//c>=0   &&
			c <= 127) i += 0;
		else if ((c & 0xE0) == 0xC0) i += 1;
		else if ((c & 0xF0) == 0xE0) i += 2;
		else if ((c & 0xF8) == 0xF0) i += 3;
		//else if (($c & 0xFC) == 0xF8) i+=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
		//else if (($c & 0xFE) == 0xFC) i+=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
		else return "";//invalid utf8
	}
	if (q <= start + leng || leng == std::string::npos) { max = i; }
	if (min == std::string::npos || max == std::string::npos) { return ""; }
	return str.substr(min, max);
}

void utf8Erase(std::string& buffer, int index)
{
	buffer = utf8Substr(buffer, 0, index) + utf8Substr(buffer, index + 1, std::string::npos);
}

void utf8Insert(std::string& buffer, int index, std::string text)
{
	std::u32string u32Buffer = utf8ToUcs4(buffer);
	if (index > u32Buffer.size()) {
		index = u32Buffer.size();
	}
	u32Buffer.insert(index, utf8ToUcs4(text));
	buffer = ucs4ToUtf8(u32Buffer);
}

#if 0
struct Input {
	SDL_Rect r{};
	Text text;
	SDL_Rect cursorR{};
	int currentLetter = 0;
	std::string str;

	void handleEvent(SDL_Event event, TTF_Font* font)
	{
		if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE) {
				if (currentLetter) {
					utf8Erase(str, currentLetter - 1);
					text.setText(renderer, font, str, {});
					Text t = text;
					--currentLetter;
					t.text = utf8Substr(t.text, 0, currentLetter);
					t.setText(renderer, font, t.text, {});
					if (currentLetter) {
						cursorR.x = t.dstR.x + t.dstR.w;
					}
					else {
						cursorR.x = t.dstR.x;
					}
				}
			}
			else if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
				if (currentLetter) {
					Text t = text;
					--currentLetter;
					t.text = utf8Substr(t.text, 0, currentLetter);
					t.setText(renderer, font, t.text, {});
					if (currentLetter) {
						cursorR.x = t.dstR.x + t.dstR.w;
					}
					else {
						cursorR.x = t.dstR.x;
					}
				}
				else {
					// TODO: Add letter in front and erase from back
					utf8Erase(text.text, utf8ToUcs4(str).size() - 1);
					utf8Insert(text.text, 0, ucs4ToUtf8(utf8ToUcs4(str).substr(0, 1))); // TODO: Replace str.front() with something else
					text.setText(renderer, font, text.text, {});
				}
			}
			else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
				if (currentLetter != utf8ToUcs4(str).size()) {
					Text t = text;
					++currentLetter;
					t.text = utf8Substr(t.text, 0, currentLetter);
					t.setText(renderer, font, t.text, {});
					cursorR.x = t.dstR.x + t.dstR.w;
				}
			}
		}
		else if (event.type == SDL_TEXTINPUT) {
			utf8Insert(str, currentLetter, event.text.text);
			text.setText(renderer, font, str, {});
			Text t = text;
			++currentLetter;
			t.text = utf8Substr(t.text, 0, currentLetter);
			t.setText(renderer, font, t.text, {});
			cursorR.x = t.dstR.x + t.dstR.w;
			while (text.dstR.x + text.dstR.w > r.x + r.w) {
				--currentLetter; // TODO: It can't be lower than 0
				utf8Erase(str, 0);
				text.setText(renderer, font, str, {});
			}
		}
	}

	void draw(SDL_Renderer* renderer)
	{
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
		SDL_RenderFillRect(renderer, &r);
		text.draw(renderer);
		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 0);
		SDL_RenderFillRect(renderer, &cursorR);
	}
};
#endif

struct Input {
	SDL_Rect r{};
	Text text;
	SDL_Rect cursorR{};
	int currentLetter = 0;

	void handleEvent(SDL_Event event, TTF_Font* font)
	{
		if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE) {
				if (currentLetter) {
					utf8Erase(text.text, currentLetter - 1);
					text.setText(renderer, font, text.text, {});
					--currentLetter;
				}
			}
			else if (event.key.keysym.scancode == SDL_SCANCODE_LEFT) {
				if (currentLetter) {
					--currentLetter;
				}
				else {
					// TODO: Add letter in front and erase from back
					utf8Erase(text.text, utf8ToUcs4(text.text).size() - 1);
					utf8Insert(text.text, 0, ucs4ToUtf8(utf8ToUcs4(text.text).substr(0, 1))); // TODO: Replace str.front() with something else
					text.setText(renderer, font, text.text, {});
				}
			}
			else if (event.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
				if (currentLetter != utf8ToUcs4(text.text).size()) {
					++currentLetter;
				}
			}
		}
		else if (event.type == SDL_TEXTINPUT) {
			utf8Insert(text.text, currentLetter, event.text.text);
			text.setText(renderer, font, text.text, {});
			++currentLetter;
			while (text.dstR.x + text.dstR.w > r.x + r.w) {
				--currentLetter; // TODO: It can't be lower than 0
				utf8Erase(text.text, 0);
				text.setText(renderer, font, text.text, {});
			}
		}
	}

	void draw(SDL_Renderer* renderer, TTF_Font* font)
	{
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 0);
		SDL_RenderFillRect(renderer, &r);
		text.draw(renderer);
		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 0);
		Text t = text;
		t.text = utf8Substr(t.text, 0, currentLetter);
		t.setText(renderer, font, t.text, {});
		if (currentLetter) {
			cursorR.x = t.dstR.x + t.dstR.w;
		}
		else {
			cursorR.x = t.dstR.x;
		}
		SDL_RenderFillRect(renderer, &cursorR);
	}
};

int main(int argc, char* argv[])
{
	std::srand(std::time(0));
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
	SDL_LogSetOutputFunction(logOutputCallback, 0);
	SDL_Init(SDL_INIT_EVERYTHING);
	TTF_Init();
	SDL_GetMouseState(&mousePos.x, &mousePos.y);
	SDL_Window* window = SDL_CreateWindow("Input4", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	TTF_Font* robotoF = TTF_OpenFont("res/roboto.ttf", 72);
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	SDL_RenderSetScale(renderer, w / (float)windowWidth, h / (float)windowHeight);
	SDL_AddEventWatch(eventWatch, 0);
	bool running = true;
	Input input;
	input.r.w = 32;
	input.r.h = 12;
	input.r.x = windowWidth / 2 - input.r.w / 2;
	input.r.y = windowHeight / 2 - input.r.h / 2;
	input.text.dstR.w = input.r.w;
	input.text.dstR.h = input.r.h;
	input.text.dstR.x = input.r.x;
	input.text.dstR.y = input.r.y;
	input.text.autoAdjustW = true;
	input.text.wMultiplier = 0.08;
	input.cursorR.w = 1;
	input.cursorR.h = input.r.h - 2;
	input.cursorR.x = input.r.x;
	input.cursorR.y = input.r.y + input.r.h / 2 - input.cursorR.h / 2;
	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			input.handleEvent(event, robotoF);
			if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
				running = false;
				// TODO: On mobile remember to use eventWatch function (it doesn't reach this code when terminating)
			}
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
				SDL_RenderSetScale(renderer, event.window.data1 / (float)windowWidth, event.window.data2 / (float)windowHeight);
			}
			if (event.type == SDL_KEYDOWN) {
				keys[event.key.keysym.scancode] = true;
			}
			if (event.type == SDL_KEYUP) {
				keys[event.key.keysym.scancode] = false;
			}
			if (event.type == SDL_MOUSEBUTTONDOWN) {
				buttons[event.button.button] = true;
			}
			if (event.type == SDL_MOUSEBUTTONUP) {
				buttons[event.button.button] = false;
			}
			if (event.type == SDL_MOUSEMOTION) {
				float scaleX, scaleY;
				SDL_RenderGetScale(renderer, &scaleX, &scaleY);
				mousePos.x = event.motion.x / scaleX;
				mousePos.y = event.motion.y / scaleY;
				realMousePos.x = event.motion.x;
				realMousePos.y = event.motion.y;
			}
		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
		SDL_RenderClear(renderer);
		input.draw(renderer,robotoF);
		SDL_RenderPresent(renderer);
	}
	// TODO: On mobile remember to use eventWatch function (it doesn't reach this code when terminating)
	return 0;
}