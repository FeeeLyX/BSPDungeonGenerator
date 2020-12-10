#include <iostream>
#include "Windows.h"
#include <ctime>
#include <vector>
#include <chrono>
#include <algorithm>
#include <GL/glut.h>
#include <SOIL.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <audiere.h>
#include <sstream>

#define _USE_MATH_DEFINES
#include <math.h>

using namespace std;
using namespace audiere;

// audio
AudioDevicePtr device = OpenDevice();
vector<OutputStreamPtr> stepSounds;
OutputStreamPtr currentStepSound;

// dungeon generator parameters
static const int minSize = 4;						// min size of leaf
static const int maxMargin = 1;						// max room margin
static const int maxRatio = 2;						// max ratio of room leaf sides
static const int hallWidth = 1;						// width of halls between rooms
static const int mapWidth = 40;						// tilemap width
static const int mapHeight = 40;					// tilemap height

// ToDo: add interactives
float interactionDistance = 1.5f;

vector<vector<bool>> temp;
vector<GLuint> textures;

int screenWidth = GetSystemMetrics(SM_CXSCREEN);
int screenHeight = GetSystemMetrics(SM_CYSCREEN);

int scale = 12;										// minimap scale

float fFOV = M_PI * 70 / 180;						// Угол обзора (поле видимости)
float fDepth = 20.0f;								// Максимальная дистанция обзора

chrono::duration<float> elapsedTime;
float fElapsedTime;
auto tp1 = chrono::system_clock::now();				// Переменные для подсчета
auto tp2 = chrono::system_clock::now();				// пройденного времени

int minimapSize = 300;
int xOffset = 15;
int yOffset = 75;
int midX = xOffset + minimapSize / 2;
int midY = screenHeight - yOffset - minimapSize / 2;

string textureList;

float shakerV = 0.0f;
float shakerH = 0.0f;

class WallPart
{
	private:
		int x, y;
		GLuint texIDs[4];
		bool interactive;

	public:
		WallPart(int x, int y, GLuint texIDs[4], bool interactive)
		{
			this->x = x;
			this->y = y;
			for (int i = 0; i < 4; i++)
				this->texIDs[i] = texIDs[i];
			this->interactive = interactive;
		}

		GLuint GetTextureID(int index) { return texIDs[index]; }
		bool IsInteractive() { return interactive; }
};

vector<vector<WallPart*>> tiles;

class Character
{
	private:
		float x, y, angle, size;

	public:
		Character(float x, float y, float a, float s)
		{
			this->x = x;
			this->y = y;
			this->angle = a;
			this->size = s;
		}

		bool Move(float distance)
		{
			float deltaX = distance * cosf(angle);
			float deltaY = distance * sinf(angle);
			if (tiles[(int)(x + deltaX)][(int)(y + deltaY)] == nullptr)
			{
				this->x += deltaX;
				this->y += deltaY;
				return true;
			}
			return false;
		}

		void Rotate(float deltaA)
		{
			angle += deltaA;
			if (angle > 2 * M_PI){ angle -= 2 * M_PI; }
			else if (angle < 0) { angle += 2 * M_PI; }
		}

		bool Strafe(float amount)
		{
			float deltaX = amount * sinf(angle);
			float deltaY = amount * cosf(angle);
			if (tiles[(int)(x + deltaX)][(int)(y + deltaY)] == nullptr)
			{
				this->x += deltaX;
				this->y += deltaY;
				return true;
			}
			return false;
		}

		float GetX() { return x; }
		float GetY() { return y; }
		float GetAngle() { return angle; }
		float GetSize() { return size; }

		void SetX(float newX) { x = newX; }
		void SetY(float newY) { y = newY; }
};

Character player = Character(1.5f, 1.5f, 0.0f, 0.1f);

vector<vector<int>> roomMap;

class Chamber
{
	public:
		static int count;

		float r, g, b;
		int startX, startY;
		int id;
		vector<Chamber*> connections;

		Chamber(int x, int y)
		{
			Chamber::count++;
			this->id = count;

			this->r = rand() % 1000 / 2000.0f + 0.25f;
			this->g = rand() % 1000 / 2000.0f + 0.25f;
			this->b = rand() % 1000 / 2000.0f + 0.25f;
			
			this->startX = x;
			this->startY = y;

			Fill(x, y);
		}

		Chamber* FindNext(int x, int y, float dir)
		{
			if (x<1 || x>mapWidth - 2 || y<1 || y>mapHeight - 2)
			{
				return nullptr;
			}
			if (tiles[x][y] != nullptr)
				return nullptr;
			roomMap[x][y] = 0;
			float left = dir - M_PI / 2;
			float right = dir + M_PI / 2;
			int leftX = x + (int)round(cos(left));
			int rightX = x + (int)round(cos(right));
			int leftY = y + (int)round(sin(left));
			int rightY = y + (int)round(sin(right));
			if (tiles[leftX][leftY] != nullptr && tiles[rightX][rightY] != nullptr)
				return FindNext(x + (int)round(cos(dir)), y + (int)round(sin(dir)), dir);
			else
				return new Chamber(x, y);
		}

		void Fill(int x, int y)
		{
			roomMap[x][y] = id;
			int nextX, nextY;
			for (float dir = 0.0f; dir <= 2 * M_PI; dir += M_PI / 2)
			{
				nextX = x + (int)round(cosf(dir));
				nextY = y + (int)round(sinf(dir));
				if (tiles[nextX][nextY] == nullptr && roomMap[nextX][nextY] == -1)
				{
					if (tiles[nextX - 1][nextY] != nullptr && tiles[nextX + 1][nextY] != nullptr)
					{
						Chamber* connection = FindNext(nextX, nextY, dir);
						if (connection != nullptr)
							connections.push_back(connection);
					}
					else if (tiles[nextX][nextY - 1] != nullptr && tiles[nextX][nextY + 1] != nullptr)
					{
						Chamber* connection = FindNext(nextX, nextY, dir);
						if (connection != nullptr)
							connections.push_back(connection);
					}
					else
						Fill(nextX, nextY);
				}
			}
		}
};

int Chamber::count;

vector<Chamber*> chambers;

class Rect
{
	public:
		int x, y, h, w;
		Rect()
		{
			x = 0;
			y = 0;
			w = 0;
			h = 0;
		}

		Rect(int X, int Y, int W, int H)
		{
			this->x = X;
			this->y = Y;
			this->w = W;
			this->h = H;
		}

		void Draw()
		{
			for (int i = x; i < x + w; i++)
				for (int j = y; j < y + h; j++)
					tiles[i][j] = nullptr;
		}
};

class Leaf
{
	private:
		int x, y, h, w;
	public:
		Leaf* left;
		Leaf* right;
		Rect* room;
		vector<Rect*> halls;

		Leaf(int X, int Y, int W, int H)
		{
			this->x = X;
			this->y = Y;
			this->h = H;
			this->w = W;
			left = NULL;
			right = NULL;
			room = NULL;

			if (Split())
			{
				if ((this->left != NULL) && (this->right != NULL))
				{
					CreateHall(this->left->GetRoom(), this->right->GetRoom());
				}
			}
			else
			{
				int leftmargin = rand() % maxMargin + 1;
				int rightmargin = rand() % maxMargin + 1;
				int topmargin = rand() % maxMargin + 1;
				int bottommargin = rand() % maxMargin + 1;
				int x = this->x + leftmargin;
				int y = this->y + topmargin;
				int w = this->w - leftmargin - rightmargin;
				int h = this->h - topmargin - bottommargin;
				room = new Rect(x, y, w, h);
			}
		}

		void DrawHalls()
		{
			if ((this->left == NULL) && (this->right == NULL))
				for (int i = 0; i < halls.size(); i++)
					halls[i]->Draw();
			else
			{
				this->left->DrawHalls();
				this->right->DrawHalls();
			}
		}

		void DrawRoom()
		{
			if (this->room != NULL)
				this->room->Draw();
			else
			{
				this->left->DrawRoom();
				this->right->DrawRoom();
			}
		}

		void CreateHall(Rect* l, Rect* r)
		{
			int x1 = rand() % l->w + l->x;
			int x2 = rand() % r->w + r->x;
			int y1 = rand() % l->h + l->y;
			int y2 = rand() % r->h + r->y;
			w = x2 - x1;
			h = y2 - y1;
			if (w < 0)
			{
				if (h < 0)
				{
					if (rand() % 2 == 0)
					{
						halls.push_back(new Rect(x2, y1, abs(w), hallWidth));
						halls.push_back(new Rect(x2, y2, hallWidth, abs(h) + hallWidth));
					}
					else
					{
						halls.push_back(new Rect(x2, y2, abs(w) + hallWidth, hallWidth));
						halls.push_back(new Rect(x1, y2, hallWidth, abs(h)));
					}
				}
				else if (h > 0)
				{
					if (rand() % 2 == 0)
					{
						halls.push_back(new Rect(x2, y1, abs(w), hallWidth));
						halls.push_back(new Rect(x2, y1, hallWidth, abs(h) + hallWidth));
					}
					else
					{
						halls.push_back(new Rect(x2, y2, abs(w) + hallWidth, hallWidth));
						halls.push_back(new Rect(x1, y1, hallWidth, abs(h)));
					}
				}
				else // if (h == 0)
				{
					halls.push_back(new Rect(x2, y2, abs(w), hallWidth));
				}
			}
			else if (w > 0)
			{
				if (h < 0)
				{
					if (rand() % 2 == 0)
					{
						halls.push_back(new Rect(x1, y2, abs(w), hallWidth));
						halls.push_back(new Rect(x1, y2, hallWidth, abs(h)));
					}
					else
					{
						halls.push_back(new Rect(x1, y1, abs(w) + hallWidth, hallWidth));
						halls.push_back(new Rect(x2, y2, hallWidth, abs(h) + hallWidth));
					}
				}
				else if (h > 0)
				{
					if (rand() % 2 == 0)
					{
						halls.push_back(new Rect(x1, y1, abs(w) + hallWidth, hallWidth));
						halls.push_back(new Rect(x2, y1, hallWidth, abs(h)));
					}
					else
					{
						halls.push_back(new Rect(x1, y2, abs(w), hallWidth));
						halls.push_back(new Rect(x1, y1, hallWidth, abs(h) + hallWidth));
					}
				}
				else // if (h == 0)
				{
					halls.push_back(new Rect(x1, y1, abs(w), hallWidth));
				}
			}
			else // if (w == 0)
			{
				if (h < 0)
				{
					halls.push_back(new Rect(x2, y2, hallWidth, abs(h)));
				}
				else if (h > 0)
				{
					halls.push_back(new Rect(x1, y1, hallWidth, abs(h)));
				}
			}

			for (int i = 0; i < halls.size(); i++)
			{
				halls[i]->Draw();
			}
		}

		Rect* GetRoom()
		{
			if (this->room != NULL)
			{
				return this->room;
			}
			else
			{
				Rect* leftroom = NULL;
				Rect* rightroom = NULL;
				if (this->left != NULL)
				{
					leftroom = this->left->GetRoom();
				}
				if (this->right != NULL)
				{
					rightroom = this->right->GetRoom();
				}
				if (leftroom == NULL && rightroom == NULL)
					return NULL;
				else if (rightroom == NULL)
					return leftroom;
				else if (leftroom == NULL)
					return rightroom;
				else if (rand() % 2 == 0)
					return leftroom;
				else
					return rightroom;
			}
		}

		bool Split()
		{
			int s = rand() % 21;
			if (s < 10)
			{
				if ((this->w > 2 * minSize) && ((2 * this->h) / this->w <= maxRatio))
				{
					Horizontal();
					return true;
				}
				else if ((this->h > 2 * minSize) && ((2 * this->w) / this->h <= maxRatio))
				{
					Vertical();
					return true;
				}
				else return false;
			}
			else if (s < 20)
			{
				if ((this->h > 2 * minSize) && ((2 * this->w) / this->h <= maxRatio))
				{
					Vertical();
					return true;
				}
				else if ((this->w > 2 * minSize) && ((2 * this->h) / this->w <= maxRatio))
				{
					Horizontal();
					return true;
				}
				else return false;
			}
			else
			{
				if (this->h > 4 * minSize)
				{
					Vertical();
					return true;
				}
				else if (this->w > 4 * minSize)
				{
					Horizontal();
					return true;
				}
				else return false;
			}
		}

	private:
		void Horizontal()
		{
			int w1 = rand() % (this->w - 2 * minSize) + minSize;
			int w2 = this->w - w1 - 1;
			int x2 = this->x + w1 + 1;
			this->left = new Leaf(this->x, this->y, w1, this->h);
			this->right = new Leaf(x2, this->y, w2, this->h);
		}

		void Vertical()
		{
			int h1 = rand() % (this->h - 2 * minSize) + minSize;
			int h2 = this->h - h1 - 1;
			int y2 = this->y + h1 + 1;
			this->left = new Leaf(this->x, this->y, this->w, h1);
			this->right = new Leaf(this->x, y2, this->w, h2);
		}
};

Leaf* dungeon;

void reshape(int w, int h)
{
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, w, 0, h);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void Wave(int x, int y)
{
	if (temp[x][y])
		return;
	if (tiles[x][y] != nullptr)
	{
		temp[x][y] = true;
		if (x - 1 >= 0)
			Wave(x - 1, y);
		if (y - 1 >= 0)
			Wave(x, y - 1);
		if (y + 1 < mapHeight)
			Wave(x, y + 1);
		if (x + 1 < mapWidth)
			Wave(x + 1, y);
	}
	else return;
}

void AddChambers(Chamber* c)
{
	chambers.push_back(c);
	for (int i = 0; i < c->connections.size(); i++)
	{
		AddChambers(c->connections[i]);
	}
}

void UpdateChambers()
{
	chambers.clear();
	Chamber::count = 0;

	for (int i = 0; i < mapWidth; i++)
		for (int j = 0; j < mapHeight; j++)
			roomMap[i][j] = -1;

	int startX = 1;
	int startY = 1;
	while (tiles[startX][startY] != nullptr)
	{
		startX = rand() % (mapWidth - 1) + 1;
		startY = rand() % (mapHeight - 1) + 1;
		
	}
	player.SetX(startX + 0.5f);
	player.SetY(startY + 0.5f);

	AddChambers(new Chamber(startX, startY));
}

vector<string> split(const string &s, char delim)
{
	vector<string> elems;
	stringstream ss(s);
	string item;
	while (getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

bool LoadMap(string filename)
{
	ifstream in;
	in.open(filename);
	if (in.is_open())
	{
		string line;
		for (int i = 0; i < mapWidth; i++)
		{
			for (int j = 0; j < mapHeight; j++)
			{
				getline(in, line);
				if (line != " ")
				{
					vector<string> splt = split(line, ' ');
					bool intrctv = splt[4] == "1";
					GLuint tex[4];
					for (int k = 0; k < 4; k++)
						tex[k] = splt[k][0] - '0';
					tiles[i][j] = new WallPart(i, j, tex, intrctv);
				}
				else
					tiles[i][j] = nullptr;
			}
		}

		UpdateChambers();

		return true;
	}
	return false;
}

bool SaveMap(string filename)
{
	ofstream out;
	out.open(filename);
	if (out.is_open())
	{
		for (int i = 0; i < mapWidth; i++)
		{
			for (int j = 0; j < mapHeight; j++)
			{
				if (tiles[i][j] != nullptr)
				{
					for (int k = 0; k < 4; k++)
					{
						out << tiles[i][j]->GetTextureID(k) << ' ';
					}
					out << tiles[i][j]->IsInteractive() ? '1' : '0';
					out << endl;
				}
				else
					out << ' ' << endl;
			}
		}
		out.close();
	}
	return false;
}

void GenerateMap()
{
	for (int i = 0; i < mapWidth; i++)
	{
		for (int j = 0; j < mapHeight; j++)
		{
			GLuint texIDs[4];
			for (int k = 0; k < 4; k++)
				texIDs[k] = rand() % textures.size() + 1;
			tiles[i][j] = new WallPart(i, j, texIDs, false);
			temp[i][j] = false;
		}
	}

	dungeon = new Leaf(0, 0, mapWidth, mapHeight);
	dungeon->DrawHalls();
	dungeon->DrawRoom();

	Wave(0, 0);

	for (int i = 0; i < mapWidth; i++)
	{
		for (int j = 0; j < mapHeight; j++)
		{
			if (!temp[i][j])
				tiles[i][j] = nullptr;
		}
	}

	UpdateChambers();
}

void Smooth()
{
	for (int i = 1; i < mapWidth-1; i++)
	{
		for (int j = 1; j < mapHeight-1; j++)
		{
			int neighbours = 0;
			for (int x = i - 1; x <= i + 1; x++)
				for (int y = j - 1; y <= j + 1; y++)
					if ((x != i || y != j) && tiles[x][y] != nullptr)
						neighbours++;
			if (neighbours > 4)
			{
				GLuint texIDs[4];
				for (int k = 0; k < 4; k++)
					texIDs[k] = rand() % textures.size() + 1;
				tiles[i][j] = new WallPart(i, j, texIDs, false);
			}
			else if (neighbours < 4)
				tiles[i][j] = nullptr;
		}
	}
}

void GenerateCave()
{
	int randomFillPercentage = 45;
	for (int i = 0; i < mapWidth; i++)
		for (int j = 0; j < mapHeight; j++)
		{
			if (i == 0 || j == 0 || i == mapWidth - 1 || j == mapHeight - 1 || (rand() % 100) < randomFillPercentage)
			{
				GLuint texIDs[4];
				for (int k = 0; k < 4; k++)
					texIDs[k] = rand() % textures.size() + 1;
				tiles[i][j] = new WallPart(i, j, texIDs, false);
			}
			else
				tiles[i][j] = nullptr;
		}

	int smoothingStagesCount = 4;
	for (int i = 0; i < smoothingStagesCount; i++)
		Smooth();

	UpdateChambers();
}

void Tick()
{
	tp2 = chrono::system_clock::now();
	elapsedTime = tp2 - tp1;
	fElapsedTime = elapsedTime.count();
	tp1 = tp2;
}

bool LoadTextures(string filename)
{
	ifstream in;
	in.open(filename);
	if (in.is_open())
	{
		string line;
		textures.clear();
		while (getline(in, line))
		{
			textures.push_back(SOIL_load_OGL_texture(line.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y));
		}

		return true;
	}
	return false;
}

bool LoadStepSounds(string filename)
{
	ifstream in;
	in.open(filename);
	if (in.is_open())
	{
		string line;
		stepSounds.clear();
		while (getline(in, line))
		{
			stepSounds.push_back(OpenSound(device, line.c_str(), false));
		}

		return true;
	}
	return false;
}

void renderBitmapString(float x, float y, float z, void *font, string s)
{
	glRasterPos3f(x, y, z);
	for (int i = 0; i < s.length(); i++)
		glutBitmapCharacter(font, s[i]);
}

time_t prevt;
int FPS = 0;

void displayTiles()
{
	// initial ray coords
	float a = player.GetAngle();
	float pointX = player.GetX() + shakerH*sinf(a);
	float pointY = player.GetY() + shakerH*cosf(a);

	if (currentStepSound->isPlaying())
	{
		int l = currentStepSound->getLength();
		int f = currentStepSound->getPosition();
		float c = abs(cosf(f * M_PI / (float)l)) - 1;
		shakerV = 5 * c * c;
	}

	bool isInteractive = false;

	// tracing ray for every pixel on x axis
	for (int x = 0; x < screenWidth; x++)
	{
		// angle of current ray
		float fRayAngle = (a + fFOV / 2.0f) - ((float)x / (float)screenWidth) * fFOV;
		if (fRayAngle > 2 * M_PI){ fRayAngle -= 2 * M_PI; }
		else if (fRayAngle < 0){ fRayAngle += 2 * M_PI; }

		bool check = false;
		if (abs(fRayAngle-a) <0.05)
			check = true;

		// initial distance
		float distX = 100;
		float distY = 100;

		float texDeltaX, texDeltaY;
		float xiX, yiX, xiY, yiY;

		int indexX, indexY;

		// vertical lines
		if (fRayAngle<0.5f*M_PI || fRayAngle>1.5f*M_PI)
		{
			float deltaX = ceilf(pointX) - pointX;
			float tga = tanf(fRayAngle);
			float deltaY = deltaX * tga;
			xiX = ceilf(pointX);
			yiX = pointY + deltaY;
			while (xiX < mapWidth && yiX < mapHeight && yiX>0)
			{
				if (tiles[(int)xiX][(int)yiX] != nullptr)
				{
					distX = sqrt(deltaX*deltaX + deltaY*deltaY);
					texDeltaX = 1 - yiX + (int)yiX;
					indexX = 0;
					break;
				}
				xiX += 1;
				yiX += tga;
				deltaX += 1;
				deltaY += tga;
			}
		}
		else if (fRayAngle>0.5*M_PI && fRayAngle < 1.5f*M_PI)
		{
			float deltaX = pointX - (int)pointX;
			float tga = tanf(fRayAngle);
			float deltaY = -deltaX*tga;
			xiX = (int)pointX;
			yiX = pointY + deltaY;
			while (xiX >= 1 && yiX < mapHeight && yiX>0)
			{
				if (tiles[(int)xiX - 1][(int)yiX] != nullptr)
				{
					xiX -= 1;
					distX = sqrt(deltaX*deltaX + deltaY*deltaY);
					texDeltaX = yiX - (int)yiX;
					indexX = 2;
					break;
				}
				xiX -= 1;
				yiX -= tga;
				deltaX += 1;
				deltaY -= tga;
			}
		}

		// horizontal lines
		if (fRayAngle < M_PI)
		{
			float deltaY = ceilf(pointY) - pointY;
			float tga = tanf(fRayAngle - 0.5f*M_PI);
			float deltaX = -deltaY * tga;
			yiY = ceilf(pointY);
			xiY = pointX + deltaX;
			while (yiY < mapHeight && xiY < mapWidth && xiY>0)
			{
				if (tiles[(int)xiY][(int)yiY] != nullptr)
				{
					distY = sqrt(deltaX*deltaX + deltaY*deltaY);
					texDeltaY = xiY - (int)xiY;
					indexY = 1;
					break;
				}
				yiY += 1;
				xiY -= tga;
				deltaY += 1;
				deltaX -= tga;
			}
		}
		else if (fRayAngle>M_PI)
		{
			float deltaY = pointY - (int)pointY;
			float tga = tanf(fRayAngle - 0.5f*M_PI);
			float deltaX = deltaY * tga;
			yiY = (int)pointY;
			xiY = pointX + deltaX;
			while (yiY >= 1 && xiY < mapWidth && xiY>0)
			{
				if (tiles[(int)xiY][(int)yiY - 1] != nullptr)
				{
					yiY -= 1;
					distY = sqrt(deltaX*deltaX + deltaY*deltaY);
					texDeltaY = 1 - xiY + (int)xiY;
					indexY = 3;
					break;
				}
				yiY -= 1;
				xiY += tga;
				deltaY += 1;
				deltaX += tga;
			}
		}

		float fDistanceToWall;
		float texDelta;
		int wallX;
		int wallY;
		int index;
		if (distX <= distY)
		{
			fDistanceToWall = distX;
			texDelta = texDeltaX;
			wallX = (int)xiX;
			wallY = (int)yiX;
			index = indexX;
		}
		else
		{
			fDistanceToWall = distY;
			texDelta = texDeltaY;
			wallX = (int)xiY;
			wallY = (int)yiY;
			index = indexY;
		}

		if (check)
		{
			if (tiles[wallX][wallY]->IsInteractive() && fDistanceToWall<interactionDistance)
			{
				isInteractive = true;
			}
		}

		// calculating wall height (with fixing fisheye effect)
		fDistanceToWall *= cosf(fRayAngle - a);
		float wallHeight = screenHeight / fDistanceToWall;
		float fCeiling = screenHeight / 2.0f - wallHeight;
		float fFloor = screenHeight / 2.0f + wallHeight;

		float shade = 0;
		if (fDistanceToWall < fDepth)
			shade = 0.75 * (1 - fDistanceToWall / fDepth);

		glEnable(GL_TEXTURE_2D);
		UINT wallID = tiles[wallX][wallY]->GetTextureID(index)-1;
		glBindTexture(GL_TEXTURE_2D, textures[wallID]);
		glBegin(GL_QUADS);
		glColor3d(shade, shade, shade);
		glColor3d(shade, shade, shade);
		glTexCoord2f(texDelta, 1.0f);
		glVertex2f(x, screenHeight - fCeiling + (10 * shakerV) / fDistanceToWall);
		glTexCoord2f(texDelta + 1 / 512.0f, 1.0f);
		glVertex2f(x + 1, screenHeight - fCeiling + (10 * shakerV) / fDistanceToWall);
		glTexCoord2f(texDelta + 1 / 512.0f, 0.0f);
		glVertex2f(x + 1, screenHeight - fFloor + (10 * shakerV) / fDistanceToWall);
		glTexCoord2f(texDelta, 0.0f);
		glVertex2f(x, screenHeight - fFloor + (10 * shakerV) / fDistanceToWall);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}

	if (isInteractive)
		renderBitmapString(screenWidth / 2 - 50, 5 * yOffset, -0.1f, GLUT_BITMAP_TIMES_ROMAN_24, "press E to interact");
}

void displayBackground()
{
	glBegin(GL_QUADS);
	float maxfloorshade = 0.5f;
	glColor3f(maxfloorshade, maxfloorshade, maxfloorshade);
	glVertex2f(0, 0);
	glVertex2f(screenWidth, 0);
	glColor3f(0, 0, 0);
	glVertex2f(screenWidth, screenHeight / 2.0f);
	glVertex2f(0, screenHeight / 2.0f);
	glVertex2f(0, screenHeight / 2.0f);
	glVertex2f(screenWidth, screenHeight / 2.0f);
	glColor3f(maxfloorshade, maxfloorshade, maxfloorshade);
	glVertex2f(screenWidth, screenHeight);
	glVertex2f(0, screenHeight);
	glEnd();
}

void displayMap()
{
	float a = player.GetAngle();

	glBegin(GL_QUADS);
	// main square
	glColor3f(0.0f, 0.0f, 0.0f);
	glVertex2f(xOffset, screenHeight - yOffset);
	glVertex2f(xOffset, screenHeight - yOffset - minimapSize);
	glVertex2f(xOffset + minimapSize, screenHeight - yOffset - minimapSize);
	glVertex2f(xOffset + minimapSize, screenHeight - yOffset);

	int nPlayerX = (int)player.GetX();
	int nPlayerY = (int)player.GetY();
	int minX = max(nPlayerX - scale - 1, 0);
	int maxX = min(nPlayerX + scale + 1, mapWidth - 1);
	int minY = max(nPlayerY - scale - 1, 0);
	int maxY = min(nPlayerY + scale + 1, mapHeight - 1);
	float minimapTileSize = minimapSize / (2.0f * scale + 1.0f);

	for (int x = minX; x <= maxX; x++)
	{
		for (int y = minY; y <= maxY; y++)
		{
			if (tiles[x][y] != nullptr)
			{
				glColor3f(1.0f, 1.0f, 1.0f);

				float left = xOffset + (x - player.GetX() + scale + 0.5f) * minimapTileSize;
				float top = screenHeight - yOffset - minimapSize + (y - player.GetY() + scale + 1.5f) * minimapTileSize;

				glVertex2f(max(left, xOffset), min(top, screenHeight - yOffset));
				glVertex2f(min(left + minimapTileSize, xOffset + minimapSize), min(top, screenHeight - yOffset));
				glVertex2f(min(left + minimapTileSize, xOffset + minimapSize), max(top - minimapTileSize, screenHeight - yOffset - minimapSize));
				glVertex2f(max(left, xOffset), max(top - minimapTileSize, screenHeight - yOffset - minimapSize));
			}
			else if (roomMap[x][y] > 0)
			{
				float r = chambers[roomMap[x][y] - 1]->r;
				float g = chambers[roomMap[x][y] - 1]->g;
				float b = chambers[roomMap[x][y] - 1]->b;
				glColor3f(r, g, b);

				float left = xOffset + (x - player.GetX() + scale + 0.5f) * minimapTileSize;
				float top = screenHeight - yOffset - minimapSize + (y - player.GetY() + scale + 1.5f) * minimapTileSize;

				glVertex2f(max(left, xOffset), min(top, screenHeight - yOffset));
				glVertex2f(min(left + minimapTileSize, xOffset + minimapSize), min(top, screenHeight - yOffset));
				glVertex2f(min(left + minimapTileSize, xOffset + minimapSize), max(top - minimapTileSize, screenHeight - yOffset - minimapSize));
				glVertex2f(max(left, xOffset), max(top - minimapTileSize, screenHeight - yOffset - minimapSize));
			}
		}
	}

	// player
	glColor3f(0.0f, 1.0f, 0.0f);
	int pointerSize = 10;
	// нос
	glVertex2f(midX + pointerSize * cosf(a), midY + pointerSize * sinf(a));
	// левый хвост
	glVertex2f(midX + pointerSize * cosf(a - M_PI / 1.5f), midY + pointerSize * sinf(a - M_PI / 1.5f));
	// жопа
	glVertex2f(midX, midY);
	// правый хвост
	glVertex2f(midX + pointerSize * cosf(a + M_PI / 1.5f), midY + pointerSize * sinf(a + M_PI / 1.5f));

	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);
	float x = player.GetX();
	float y = player.GetY();
	string s = "X: " + to_string(x) + " Y: " + to_string(y) + " Room " + to_string(roomMap[x][y]);
	renderBitmapString(xOffset, screenHeight - yOffset - minimapSize - 20, 0, GLUT_BITMAP_TIMES_ROMAN_24, s);
	s = "Angle: " + to_string(a * 180 / M_PI);
	renderBitmapString(xOffset, screenHeight - yOffset - minimapSize - 40, 0, GLUT_BITMAP_TIMES_ROMAN_24, s);

	//time_t t;
	//if (time(&t) > prevt)
	//{
		FPS = 1 / fElapsedTime;
	//}
	s = "FPS: " + to_string(FPS);
	renderBitmapString(xOffset, screenHeight - yOffset - minimapSize - 60, 0, GLUT_BITMAP_TIMES_ROMAN_24, s);
	//prevt = t;
}

void displayAll()
{
	Tick();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	displayBackground();

	//display(-2*screenHeight);
	displayTiles();
	//display(2*screenHeight);

	displayMap();

	glutSwapBuffers();
}

/*
bool pressed = false;
void Mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN && !pressed)
		{
			pressed = true;
			if (x >= xOffset && x < xOffset + minimapSize && y >= yOffset-24 && y < yOffset + minimapSize-24)
			{
				y = screenHeight - y + 24;
				int tileX = (x - midX + minimapSize / (4 * scale + 2)) / scale + (int)player.GetX();
				int tileY = (y - midY) / scale + (int)player.GetY() - 4;
				if (tileX > 0 && tileX < mapWidth - 1 && tileY > 0 && tileY < mapHeight - 1)
				{
					tiles[tileX][tileY] = !tiles[tileX][tileY];
					UpdateChambers();
				}
			}
		}
		else
			pressed = false;
	}
}
*/

void Action(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'a':
		for (int i = 0; i < 10; i++)
		{
			player.Rotate(0.5f * M_PI / 180.0f);
			displayAll();
		}

		break;
	case 'd':
		for (int i = 0; i < 10; i++)
		{
			player.Rotate(-0.5f * M_PI / 180.0f);
			displayAll();
		}
		break;
	case 'w':
		if (!currentStepSound->isPlaying())
		{
			currentStepSound = stepSounds[rand() % stepSounds.size()];
			currentStepSound->play();
		}
		for (int i = 0; i < 10; i++)
		{
			player.Move(0.02f);
			displayAll();
		}
		break;
	case 's':
		if (!currentStepSound->isPlaying())
		{
			currentStepSound = stepSounds[rand() % stepSounds.size()];
			currentStepSound->play();
		}
		for (int i = 0; i < 10; i++)
		{
			player.Move(-0.02f);
			displayAll();
		}
		break;
	case '-':
		scale += 1;
		break;
	case '=':
		scale = max(scale - 1, 1);
		break;
	case 'g':
		GenerateMap();
		break;
	case 'f':
		LoadMap("map.txt");
		break;
	case 'v':
		SaveMap("map.txt");
		break;
	case 'c':
		GenerateCave();
		break;
	case 'l':
		LoadTextures(textureList);
		break;
	}
}

int main(int argc, char* argv[])
{
	LoadStepSounds("./stepsounds/stepsounds.txt");
	currentStepSound = stepSounds[0];

	OutputStreamPtr ambient = OpenSound(device, "RPG_Dungeon_Ambient.mp3", true);
	ambient->setRepeat(true);
	ambient->play();

	srand(time(0));

	Chamber::count = 0;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(screenWidth, screenHeight);
	
	glutCreateWindow("Test");

	//textureList = "./12tiles289/textures.txt";
	//textureList = "textures.txt";
	textureList = "./minecraft/textures.txt";
	//textureList = "./tex512/textures.txt";
	//textureList = "./tridoors/textures.txt";
	LoadTextures(textureList);

	for (int i = 0; i < mapWidth; i++)
	{
		tiles.push_back(vector<WallPart*>());
		temp.push_back(vector<bool>());
		roomMap.push_back(vector<int>());
		for (int j = 0; j < mapHeight; j++)
		{
			GLuint texIDs[4];
			for (int k = 0; k < 4; k++)
				texIDs[k] = rand() % textures.size() + 1;
			tiles[i].push_back(new WallPart(i, j, texIDs, false));
			temp[i].push_back(false);
			roomMap[i].push_back(-1);
		}
	}

	GenerateMap();

	glutReshapeFunc(reshape);
	glutDisplayFunc(displayAll);

	glutKeyboardFunc(Action);
	glutIdleFunc(displayAll);

	//glutMouseFunc(Mouse);

	SetCursorPos(screenWidth / 2, screenHeight / 2);

	glutMainLoop();
	
	return 0;
}